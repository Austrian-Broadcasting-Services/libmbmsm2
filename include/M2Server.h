#pragma once
#include <usrsctp.h>
#include "M2ServerConnection.h"
#include <vector>
#include <memory>

namespace MbmsIfs {
  class M2Server {
    public:
      M2Server(const char* interface, unsigned short port);
      virtual ~M2Server();

      void add_connection(std::shared_ptr<M2ServerConnection> conn) { _connections.push_back(conn); }

    private:
      struct socket* _sock;
      std::vector<std::shared_ptr<M2ServerConnection>> _connections;

  };
}
