#include "sdk_structs.h"
#include "ieee80211_structs.h"


// https://github.com/n0w/esp8266-simple-sniffer/blob/master/src/string_utils.cpp

// Uncomment to enable MAC address masking
#define MASKED

//Returns a human-readable string from a binary MAC address.
//If MASKED is defined, it masks the output with XX
void mac2str(const uint8_t* ptr, char* string)
{
  #ifdef MASKED
  sprintf(string, "XX:XX:XX:%02x:%02x:XX", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  #else
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  #endif
  return;
}

//Parses 802.11 packet type-subtype pair into a human-readable string
const char* wifi_pkt_type2str(wifi_promiscuous_pkt_type_t type, wifi_mgmt_subtypes_t subtype)
{
  switch(type)
  {
    case WIFI_PKT_MGMT:
      switch(subtype)
      {
    	   case ASSOCIATION_REQ:
         return "Management (Association Request)";
         case ASSOCIATION_RES:
         return "Management (Association Response)";
         case REASSOCIATION_REQ:
         return "Management (Reassociation Request)";
         case REASSOCIATION_RES:
         return "Management (Reassociation Response)";
         case PROBE_REQ:
         return "Management (Probe Request)";
         case PROBE_RES:
         return "Management (Probe Response)";
         case BEACON:
         return "Management (Beacon)";
         case ATIM:
         return "Management (ATIM";
         case DISASSOCIATION:
         return "Management (Dissasociation)";
         case AUTHENTICATION:
         return "Management (Authentication)";
         case DEAUTHENTICATION:
         return "Management (Deauthentication)";
         case ACTION:
         return "Management (Action)";
         case ACTION_NACK:
         return "Management (Action No Ack)";
    	default:
        return "Management (Unsupported)";
      }

    case WIFI_PKT_CTRL:
    return "Control";

    case WIFI_PKT_DATA:
    return "Data";

    default:
      return "Unsupported/error";
  }
}

byte getFrameType(wifi_promiscuous_pkt_type_t type)
{
  switch(type)
  {
    case WIFI_PKT_MGMT:
    return 0;

    case WIFI_PKT_CTRL:
    return 1;

    case WIFI_PKT_DATA:
    return 2;
  }
}
