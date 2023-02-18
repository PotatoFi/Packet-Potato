
#include <ESP8266WiFi.h>
#include <string>
#include "sevensegment.h"
#include "sdk_structs.h"
#include "ieee80211_structs.h"
#include "string_utils.h"

extern "C" {
#include "user_interface.h"
}

// Define Arduino pins (board v1.02, v1.03, and v1.04)
const int pinOFDM = 4;          // LEDs
const int pinDSSS = 0;          // LEDs
const int pinMGMT = 5;          // LED
const int pinCTRL = 1;          // LED
const int pinDATA = 3;          // LED
const int plusButton = 12;      // Button
const int minusButton = 16;     // Button
sevensegment Display(14, 15, 13, 2);

/*
// Define Ardunio pins (Beta board)
const int pinOFDM = 1;          // LEDs
const int pinDSSS = 3;          // LEDs
const int pinMGMT = 5;          // LED
const int pinCTRL = 4;          // LED
const int pinDATA = 0;          // LED
const int plusButton = 2;       // Button
const int minusButton = 16;     // Button
sevensegment Display(14, 15, 13, 2);
*/

byte scanChannel = 6;            // Current channel variable, and set it to start on channel 6
unsigned frameRate = 0;
unsigned frameRSSI = 0;
byte frameType = 0;
unsigned frameDisplay = 0;      // Used to write special frame data to the display

// Define intervals and durations
const int blinkDuration = 10;          // Amount of time to keep LEDs lit, 10 is good
const int minBlinkInterval = 60;       // Minimum amount of time between starting blinks, 60 is good
const int shortPressInterval = 200;
const int longPressInterval = 1000;
const int minChannelDisplayInterval = 2000;

// Define timers
unsigned long whenPlusPressed = 0;
unsigned long whenMinusPressed = 0;
unsigned long whenBlinked = 0;
unsigned long whenChannelDisplay = 0;

// State variables
boolean plusButtonState = false;
boolean minusButtonState = false;
boolean plusButtonEvent = false;
boolean minusButtonEvent = false;
boolean blinkingState = false;

boolean serialEnable = false;
boolean modeRate = false;
boolean modeRSSI = false;
byte displayMode = 0; // 0 is normal, 1 is RSSI, 2 is rate/MCS

void setup() {

  // Define pin modes for inputs and outputs
  pinMode(pinOFDM, OUTPUT);
  pinMode(pinDSSS, OUTPUT);
  pinMode(pinMGMT, OUTPUT);
  pinMode(pinCTRL, OUTPUT);
  pinMode(pinDATA, OUTPUT);
  pinMode(plusButton, INPUT);
  pinMode(minusButton, INPUT);

  Display.Begin(); //Initialize display

  // Loop through numbers 0-99
  for (int initDisplay = 0 ; initDisplay <= 99 ; initDisplay++) {
      Display.Update(initDisplay);
      if (initDisplay >= 25) { digitalWrite(pinDSSS, HIGH); }
      if (initDisplay >= 40) { digitalWrite(pinOFDM, HIGH); }
      if (initDisplay >= 55) { digitalWrite(pinDATA, HIGH); }
      if (initDisplay >= 70) { digitalWrite(pinCTRL, HIGH); }
      if (initDisplay >= 85) { digitalWrite(pinMGMT, HIGH); }
      delay(25);
  }

  delay(125);
  serialEnable = digitalRead(minusButton);
  delay(125);
  
  // Loop through numbers 0-99
  for (int initDisplay = 99 ; initDisplay >= scanChannel ; initDisplay--) {
      Display.Update(initDisplay);
      if (initDisplay <= 25) { digitalWrite(pinDSSS, LOW); }
      if (initDisplay <= 40) { digitalWrite(pinOFDM, LOW); }
      if (initDisplay <= 55) { digitalWrite(pinDATA, LOW); }
      if (initDisplay <= 70) { digitalWrite(pinCTRL, LOW); }
      if (initDisplay <= 85) { digitalWrite(pinMGMT, LOW); }
      delay(10);
  }

  if (serialEnable) {
    Serial.begin(115200);
    Serial.print("\n");
    Serial.print("Welcome to the Packet Potato. Serial output is enabled at 115200 baud.");
    Serial.print("\n");
  }

  // Set display to current channel
  Display.Update(scanChannel);

  //Set station mode, callback, then cycle promiscuous mode
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  wifi_set_channel(scanChannel);
  //   wifi_set_promiscuous_rx_cb(capture);
  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
}

struct RxControl {
  signed rssi: 8; // signal intensity of packet
  unsigned rate: 4;
  // signed rate: 8; 
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;         // 0:is 11n packet; 1:is not 11n packet;
  unsigned legacy_length: 12;   // If not 11n packet, shows length of packet.
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;              // If is 11n packet, shows the modulation and code used (range from 0 to 76)
  unsigned CWB: 1;              // If is 11n packet, shows if is HT40 packet or not
  unsigned HT_length: 16;       // If is 11n packet, shows length of packet.
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;      // If is 11n packet, shows if is LDPC packet or not.
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;        // Which channel this packet in.
  unsigned: 12;
};

struct sniffer_buf2 {         // I have absolutely no idea what is going on here, is it creating a struct?
  struct RxControl rx_ctrl;   // This is creating a structure or something...?
  u8 buf[112];                // Only mention of "buf" I can find. 112 refers to the length of the frame?
  u16 cnt;
  u16 len;                    // Length of packet
};

// This is commented out because it's not used at the moment. Might be more useful than the string operations
// later so I haven't deleted it yet.
/*
  wifi_promiscuous_pkt_type_t packet_type_parser(uint16_t len)
  {
  switch (len)
  {
    // If only rx_ctrl is returned, this is an unsupported packet
    case sizeof(wifi_pkt_rx_ctrl_t):
      return WIFI_PKT_MISC;

    // Management packet
    case sizeof(wifi_pkt_mgmt_t):
      return WIFI_PKT_MGMT;

    // Data packet
    default:
      return WIFI_PKT_DATA;
  }
  }
*/

// This is the new handler, stolen mostly from https://github.com/n0w/esp8266-simple-sniffer/blob/master/src/main.cpp
void wifi_sniffer_packet_handler(uint8_t *buff, uint16_t len) {
    
  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

  // Third layer: define pointers to the 802.11 packet header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the packet header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // This seems to have something to do with the frame subtype
  String type = wifi_pkt_type2str((wifi_promiscuous_pkt_type_t)frame_ctrl->type, (wifi_mgmt_subtypes_t)frame_ctrl->subtype);

  if ((millis() - whenBlinked >= minBlinkInterval) && blinkingState == false) {
    blinkingState = true;

    // Show RSSI on screen, if enabled
    if (displayMode == 1) {
      frameDisplay = ppkt->rx_ctrl.rssi * -1;       // Convert RSSI to a positive number
    }

    // Show rate or MCS on screen, if enabled
    if (displayMode == 2) {
      if (ppkt->rx_ctrl.rate == 0) {frameDisplay = ppkt->rx_ctrl.mcs; }
      else {frameDisplay = ppkt->rx_ctrl.rate; }
    }

    String mgmt = "Mgmt:";
    // This is bad and could be improved by just checking the type and not the subtype like the called function does
    if (type == "Data") {
      frameType = 2;
    }
    
    else if (type == "Control") {
      frameType = 1;
    }
    
    else if (strstr(type.c_str(), mgmt.c_str())) { // See if the string "Mgmt:" is a substring of the type
      frameType = 0;
    }
    
    switch ( ppkt->rx_ctrl.rate) {
      case 1:
        blinkOn(0,frameType,frameDisplay);
        break;
      case 2:
        blinkOn(0,frameType,frameDisplay);
        break;
      case 5.5:
        blinkOn(0,frameType,frameDisplay);
        break;
      case 11:
        blinkOn(0,frameType,frameDisplay);
        break;
      default:
        blinkOn(1,frameType,frameDisplay);
        break;
    }

    if (serialEnable) {
      Serial.print("Frame type: ");
      Serial.print(type);
      Serial.print("\n");
      Serial.print("Data Rate: ");
      Serial.print(ppkt->rx_ctrl.rate);
      Serial.print("\n");
      Serial.print("MCS: ");
      Serial.print(ppkt->rx_ctrl.mcs);
      Serial.print("\n");
      Serial.print("RSSI: ");
      Serial.print(ppkt->rx_ctrl.rssi);
      Serial.print("\n");
      Serial.print("\n");
    }
     
  }
}

void capture(uint8_t *buff, uint16_t len) {      // This seems to callback whenever a packet is heard?
  sniffer_buf2 *data = (sniffer_buf2 *)buff;     // No idea what this does (but *data is a pointer, right?)
}

void loop() {

  if ((millis() - whenBlinked >= blinkDuration)) {
    allOff();
    blinkingState = false;
  }

  // Check to see when the right button is pressed, and start a timer
  plusButtonState = digitalRead(plusButton); // Check button state
  if ((plusButtonState == HIGH) && (plusButtonEvent == false) && (millis() - whenPlusPressed >= shortPressInterval)) { 
    whenPlusPressed = millis();
    plusButtonEvent = true;
  }

  // Check to see if the button was released before the long press threshold
  if ((plusButtonState == LOW) && (plusButtonEvent == true) && (millis() - whenPlusPressed <= longPressInterval)) {
      scanChannel++; // Increase the channel by 1
      if (scanChannel == 14) {
        scanChannel = 1;
      }
      Display.Update(scanChannel); // Write new channel to display 
      whenChannelDisplay = millis();
      resetScanning();            // Reset scanning so the new channel is used
      plusButtonEvent = false;
  }


  // If the long press threshold is exceeded, register a long button press
  if ((plusButtonState == HIGH) && (millis() - whenPlusPressed >= longPressInterval)) {
      displayMode++;
      if (displayMode == 3) {
        displayMode = 0;
      }
      indicateDisplayMode();
      plusButtonEvent = false;
  }

  // Check to see when the left button was pressed, and start a timer
  minusButtonState = digitalRead(minusButton); // Check button state
  if ((minusButtonState == HIGH) && (minusButtonEvent == false) && (millis() - whenMinusPressed >= shortPressInterval)) { 
    whenMinusPressed = millis();
    minusButtonEvent = true;
  }

  // Check to see if the button was released before the long press threshold
  if ((minusButtonState == LOW) && (minusButtonEvent == true) && (millis() - whenMinusPressed <= longPressInterval)) {
    scanChannel--;              // Decrease the channel by 1
    if (scanChannel == 0) {    // Write new channel to display
      scanChannel = 13;
    }
    Display.Update(scanChannel); // Write new channel to display
    whenChannelDisplay = millis();
    resetScanning();            // Reset scanning so the new channel is used
    minusButtonEvent = false;
  }

}

// Reset scanning, used whenever we change channels with the buttons
void resetScanning() {
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
  wifi_set_channel(scanChannel);
}

void blinkOn(boolean modulationType, byte frameType, unsigned frameDisplay) {
  if (modulationType == 0) {
    digitalWrite(pinDSSS, HIGH);
  }
  if (modulationType == 1) {
    digitalWrite(pinOFDM, HIGH);
  }
  if (frameType == 0) {
    digitalWrite(pinMGMT, HIGH);
  }
  if (frameType == 1) {
    digitalWrite(pinCTRL, HIGH);
  }
  if (frameType == 2) {
    digitalWrite(pinDATA, HIGH);
  }
  if ((displayMode != 0) && (millis() - whenChannelDisplay >= minChannelDisplayInterval)) {
    Display.Update(frameDisplay);
  }
  whenBlinked = millis(); 
}

void allOff() {
  digitalWrite(pinDSSS, LOW);
  digitalWrite(pinOFDM, LOW);
  digitalWrite(pinMGMT, LOW);
  digitalWrite(pinCTRL, LOW);
  digitalWrite(pinDATA, LOW);
}

void indicateDisplayMode() {
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  allOff();

  if (displayMode == 0) {
    delay(100);
    digitalWrite(pinDATA, HIGH);
    delay(100);
    digitalWrite(pinCTRL, HIGH);
    delay(100);
    digitalWrite(pinMGMT, HIGH);
    delay(100);
    digitalWrite(pinDATA, LOW);
    delay(100);
    digitalWrite(pinCTRL, LOW);
    delay(100);
    digitalWrite(pinMGMT, LOW);
    delay(100);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinDATA, HIGH);
        delay(100);
        digitalWrite(pinDATA, LOW);
        delay(100);
      }
    Display.Update(scanChannel);
    }

  if (displayMode == 1) {
    delay(100);
    digitalWrite(pinDATA, HIGH);
    delay(100);
    digitalWrite(pinCTRL, HIGH);
    delay(100);
    digitalWrite(pinMGMT, HIGH);
    delay(100);
    digitalWrite(pinDATA, LOW);
    delay(100);
    digitalWrite(pinCTRL, LOW);
    delay(100);
    digitalWrite(pinMGMT, LOW);
    delay(100);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinCTRL, HIGH);
        delay(100);
        digitalWrite(pinCTRL, LOW);
        delay(100);
      }
  }

  if (displayMode == 2) {
    delay(100);
    digitalWrite(pinDATA, HIGH);
    delay(100);
    digitalWrite(pinCTRL, HIGH);
    delay(100);
    digitalWrite(pinMGMT, HIGH);
    delay(100);
    digitalWrite(pinDATA, LOW);
    delay(100);
    digitalWrite(pinCTRL, LOW);
    delay(100);
    digitalWrite(pinMGMT, LOW);
    delay(100);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinMGMT, HIGH);
        delay(100);
        digitalWrite(pinMGMT, LOW);
        delay(100);
      }
  }
  
  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
  wifi_set_channel(scanChannel);
  whenChannelDisplay = 0;
}
