#include "M2ServerConnection.h"
#include "spdlog/spdlog.h"
#include "M2AP-PDU.h" 

#define BUFFERSIZE 512
void MbmsIfs::M2ServerConnection::handle_receive(struct socket* sock, int flags)
{
        char* buf = (char*)malloc(BUFFERSIZE);
        struct sockaddr_storage addr;
        socklen_t len = (socklen_t)sizeof(struct sockaddr_storage);
        unsigned int infotype = 0;
        socklen_t infolen = sizeof(struct sctp_recvv_rn);
        struct sctp_recvv_rn rn;
        memset(&rn, 0, sizeof(struct sctp_recvv_rn));
        int recv_flags = 0;

        auto recvd_bytes = usrsctp_recvv(
            sock, buf, BUFFERSIZE, 
            (struct sockaddr *) &addr, &len, (void *)&rn,
            &infolen, &infotype, &recv_flags);


        if (recvd_bytes > 0) {
        spdlog::info("M2ServerConnection has received: ");
          M2AP_PDU_t* pdu = 0;
          auto ret = aper_decode_complete(
              0,
              &asn_DEF_M2AP_PDU,
              (void**)&pdu,
              buf,
              recvd_bytes);

          if (ret.code == RC_OK) {
            asn_fprint(stdout, &asn_DEF_M2AP_PDU, pdu);
          }
        }
        free(buf);
}
