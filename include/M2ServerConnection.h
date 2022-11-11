#pragma once
#include <usrsctp.h>

namespace MbmsIfs {
  class M2ServerConnection {
    public:
      M2ServerConnection(struct socket* sock)
        : _sock(sock) 
        {};
      virtual ~M2ServerConnection() = default;

      void handle_receive(struct socket* sock, int flags);

    private:
      struct socket* _sock;

  };
}
