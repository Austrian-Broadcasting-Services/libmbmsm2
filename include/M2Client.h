#pragma once
#include <usrsctp.h>
#include <vector>
#include <string>

namespace MbmsIfs {
  class M2Client {
    public:
      M2Client(const char* target, unsigned short port);
      virtual ~M2Client();

      bool send_setup_request(std::string mcc, std::string mnc, uint32_t enb_id, std::string enb_name,
          uint32_t mbsfn_sync_area_id, std::vector<std::string> mbsfn_service_areas);
    private:
      struct socket* _sock;
  };
}
