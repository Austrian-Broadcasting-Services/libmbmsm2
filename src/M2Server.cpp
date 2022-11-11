
#include "M2Server.h"
#include <stdexcept>
#include <usrsctp.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "spdlog/spdlog.h"
#include <boost/bind.hpp>

MbmsIfs::M2Server::M2Server(const char* interface, unsigned short port)
{
  usrsctp_init(0, nullptr, nullptr);
  usrsctp_sysctl_set_sctp_blackhole(2);
  usrsctp_sysctl_set_sctp_no_csum_on_loopback(0);

	if ((_sock = usrsctp_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP, NULL, NULL, 0, NULL)) == NULL) {
    throw std::runtime_error("SCTP socket could not be created");
	}
	usrsctp_set_non_blocking(_sock, 1);

	struct sockaddr_in addr;
  struct in_addr ina;
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len = sizeof(struct sockaddr_in);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
  if (0 == inet_aton(interface, &addr.sin_addr)) {
    throw std::runtime_error(std::string("Invalid address: ") + interface);
  }

	if (usrsctp_bind(_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    throw std::runtime_error("SCTP socket could not be bound");
	}
	if (usrsctp_listen(_sock, 1) < 0) {
    throw std::runtime_error("SCTP socket listen() failed ");
	}

	usrsctp_set_upcall(_sock, 
      [](struct socket *sock, void *data, int flags) {
        auto self = reinterpret_cast<MbmsIfs::M2Server*>(data);
        spdlog::info("M2Server accepting a new connection");

        struct socket* client_sock;
        if (((client_sock = usrsctp_accept(sock, NULL, NULL)) == NULL)
            && (errno != EINPROGRESS)) {
          spdlog::error("accept() failed with error {}", strerror(errno));
          return;
        }

        auto conn = std::make_shared<MbmsIfs::M2ServerConnection>(client_sock);
	      usrsctp_set_upcall(client_sock,
            [](struct socket *sock, void *data, int flags) {
              auto conn = reinterpret_cast<MbmsIfs::M2ServerConnection*>(data);
              conn->handle_receive(sock, flags);
            }, 
            conn.get());
        self->add_connection(conn);
      }, 
      this);
}

MbmsIfs::M2Server::~M2Server()
{
    usrsctp_finish();
}

