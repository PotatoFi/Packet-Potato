#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include "sdk_structs.h"
#include "ieee80211_structs.h"

// https://github.com/n0w/esp8266-simple-sniffer/blob/master/src/string_utils.h

void mac2str(const uint8_t* ptr, char* string);
const char* wifi_pkt_type2str(wifi_promiscuous_pkt_type_t type, wifi_mgmt_subtypes_t subtype);

#endif
