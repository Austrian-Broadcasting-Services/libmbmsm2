
#include "M2Client.h"
#include <stdexcept>
#include <usrsctp.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "spdlog/spdlog.h"

#include "M2AP-PDU.h" 
#include "GlobalENB-ID.h" 
#include "ProtocolIE-Field.h" 

bool str_to_bcd(const std::string& str, uint16_t* res) 
{
  *res = 0x0000;
  if (str.length() == 3) {
    *res |= ((uint8_t)(str[0] - '0') << 8);
    *res |= ((uint8_t)(str[1] - '0') << 4);
    *res |= ((uint8_t)(str[2] - '0'));
  } else if (str.length() == 2) {  
    *res |= ((uint8_t)(str[0] - '0') << 4);
    *res |= ((uint8_t)(str[1] - '0'));
  } else {
    return false;
  }

  return true;
}

MbmsIfs::M2Client::M2Client(const char* target, unsigned short port)
{
  usrsctp_init(0, nullptr, nullptr);
	usrsctp_sysctl_set_sctp_blackhole(2);
	usrsctp_sysctl_set_sctp_no_csum_on_loopback(0);

	if ((_sock = usrsctp_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP, NULL, NULL, 0, NULL)) == NULL) {
    throw std::runtime_error("SCTP socket could not be created");
	}
	struct sockaddr_in addr;
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len = sizeof(struct sockaddr_in);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
  if (0 == inet_aton(target, &addr.sin_addr)) {
    throw std::runtime_error(std::string("Invalid address: ") + target);
  }
  if (usrsctp_connect(_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    throw std::runtime_error(std::string("SCTP socket failed to connect to ") + target + ":" + std::to_string(port));
  }

}

MbmsIfs::M2Client::~M2Client()
{
    usrsctp_finish();
}

bool MbmsIfs::M2Client::send_setup_request(std::string mcc, std::string mnc, uint32_t enb_id, std::string enb_name, uint32_t mbsfn_sync_area_id, std::vector<std::string> mbsfn_service_areas)
{
  M2SetupRequest_t setupRequest = {};

  // Global eNB ID IE
  GlobalENB_ID_t enb_id_s = {};

  uint16_t mcc_bcd;
  if (!str_to_bcd(mcc, &mcc_bcd)) {
    spdlog::error("Cannot convert MCC {} to BCD", mcc); 
    return false;
  }

  uint16_t mnc_bcd;
  if (!str_to_bcd(mnc, &mnc_bcd)) {
    spdlog::error("Cannot convert MNC {} to BCD", mnc); 
    return false;
  }

  uint8_t plmn_nibbles[6];
  plmn_nibbles[1] = (mcc_bcd & 0x0F00) >> 8; // MCC digit 1
  plmn_nibbles[0] = (mcc_bcd & 0x00F0) >> 4; // MCC digit 2
  plmn_nibbles[3] = (mcc_bcd & 0x000F);      // MCC digit 3

  if (mnc.length() == 2) {
    plmn_nibbles[2] = 0x0F;                // MNC digit 1
    plmn_nibbles[5] = (mnc_bcd & 0x00F0) >> 4; // MNC digit 2
    plmn_nibbles[4] = (mnc_bcd & 0x000F);      // MNC digit 3
  } else {
    plmn_nibbles[2] = (mnc_bcd & 0x0F00) >> 8; // MNC digit 1
    plmn_nibbles[5] = (mnc_bcd & 0x00F0) >> 4; // MNC digit 2
    plmn_nibbles[4] = (mnc_bcd & 0x000F);      // MNC digit 3
  }

  char* plmn = (char*)calloc(1,3);
  plmn[0] = (plmn_nibbles[0] << 4) | plmn_nibbles[1];
  plmn[1] = (plmn_nibbles[2] << 4) | plmn_nibbles[3];
  plmn[2] = (plmn_nibbles[4] << 4) | plmn_nibbles[5];
  OCTET_STRING_fromString( &enb_id_s.pLMN_Identity, plmn);

  enb_id_s.eNB_ID.present = ENB_ID_PR_macro_eNB_ID;
  enb_id_s.eNB_ID.choice.macro_eNB_ID.size = 3;
  enb_id_s.eNB_ID.choice.macro_eNB_ID.bits_unused = 4;
  enb_id_s.eNB_ID.choice.macro_eNB_ID.buf = (uint8_t*)calloc(sizeof(uint8_t), 3);
  enb_id_s.eNB_ID.choice.macro_eNB_ID.buf[2] = (enb_id & 0x0000000F) << 4;
  enb_id_s.eNB_ID.choice.macro_eNB_ID.buf[1] = (enb_id & 0x00000FF0) >> 4;
  enb_id_s.eNB_ID.choice.macro_eNB_ID.buf[0] = (enb_id & 0x000FF000) >> 16;
  
  M2SetupRequest_Ies_t globalenb_id_ie = {};
  globalenb_id_ie.id = ProtocolIE_ID_id_GlobalENB_ID;
  globalenb_id_ie.criticality = Criticality_reject;
  globalenb_id_ie.value.present = M2SetupRequest_Ies__value_PR_GlobalENB_ID;
  globalenb_id_ie.value.choice.GlobalENB_ID = enb_id_s;
  ASN_SEQUENCE_ADD(&setupRequest.protocolIEs, &globalenb_id_ie);


  // eNB Name IE - optional
  M2SetupRequest_Ies_t enb_name_ie = {};
  if (enb_name.length()) {
    enb_name_ie.id = ProtocolIE_ID_id_ENBname;
    enb_name_ie.criticality = Criticality_ignore;
    enb_name_ie.value.present = M2SetupRequest_Ies__value_PR_ENBname;
    OCTET_STRING_fromString( &enb_name_ie.value.choice.ENBname, enb_name.c_str());
    ASN_SEQUENCE_ADD(&setupRequest.protocolIEs, &enb_name_ie);
  }

  // eNB MBMS Configuration Data per Cell - exactly one
  M2SetupRequest_Ies_t enb_mbms_cfg_list_ie = {};
  enb_mbms_cfg_list_ie.id = ProtocolIE_ID_id_ENB_MBMS_Configuration_data_List;
  enb_mbms_cfg_list_ie.criticality = Criticality_reject;
  enb_mbms_cfg_list_ie.value.present = M2SetupRequest_Ies__value_PR_ENB_MBMS_Configuration_data_List;

  ENB_MBMS_Configuration_data_ItemIEs_t mbms_cfg_item_ie = {};
  mbms_cfg_item_ie.id = ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item;
  mbms_cfg_item_ie.criticality = Criticality_reject;
  mbms_cfg_item_ie.value.present =  ENB_MBMS_Configuration_data_ItemIEs__value_PR_ENB_MBMS_Configuration_data_Item;
  OCTET_STRING_fromString( &mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.pLMN_Identity, plmn);
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.size = 4;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.bits_unused = 4;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf = (uint8_t*)calloc(sizeof(uint8_t), 3);
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf[3] = (enb_id & 0x0000000F) << 4;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf[2] = (enb_id & 0x00000FF0) >> 4;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf[1] = (enb_id & 0x000FF000) >> 12;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf[0] = (enb_id & 0x0FF00000) >> 20;
  mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.mbsfnSynchronisationArea = 12;

  std::vector<OCTET_STRING> service_areas;
  for (auto& area : mbsfn_service_areas) {
    auto& sa_octet_string = service_areas.emplace_back();
    OCTET_STRING_fromString( &sa_octet_string, area.c_str() );
    ASN_SEQUENCE_ADD(&mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.mbmsServiceAreaList, &sa_octet_string);
  }
  ASN_SEQUENCE_ADD(&enb_mbms_cfg_list_ie.value.choice.ENB_MBMS_Configuration_data_List.list, &mbms_cfg_item_ie);
 ASN_SEQUENCE_ADD(&setupRequest.protocolIEs, &enb_mbms_cfg_list_ie);

  InitiatingMessage_t msg = {};
  msg.procedureCode = ProcedureCode_id_m2Setup;
  msg.criticality = Criticality_reject;
  msg.value.present = InitiatingMessage__value_PR_M2SetupRequest;
  msg.value.choice.M2SetupRequest = setupRequest;

  M2AP_PDU_t pdu = {};
  pdu.present = M2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = msg;

	char buffer[80];

  auto ret = aper_encode_to_buffer(
      &asn_DEF_M2AP_PDU,
      NULL,
      &pdu,
      buffer,
      80
      );

  free(enb_id_s.eNB_ID.choice.macro_eNB_ID.buf);
  free(mbms_cfg_item_ie.value.choice.ENB_MBMS_Configuration_data_Item.eCGI.eUTRANcellIdentifier.buf);
  free(plmn);
  for (auto& area : service_areas) {
    free(area.buf);
  }

  if(ret.encoded == -1) {
    spdlog::error("Cannot encode {}: {}", ret.failed_type->name, strerror(errno));
    return false;
  } else {
    int bytes = (ret.encoded + 7)/8;
    spdlog::info("Encoded {} bits, {} bytes", ret.encoded, bytes);
    usrsctp_sendv(_sock, buffer, bytes, NULL, 0, NULL, 0, SCTP_SENDV_NOINFO, 0);
    return true;
  }
}

