
#include <ESP8266WiFi.h>
#include <string>
#include "sevensegment.h"
#include "sdk_structs.h"
#include "ieee80211_structs.h"
#include "string_utils.h"

extern "C" {
#include "user_interface.h"
}

/*
  WARNING: This code is super messy! It works, but there's a LOT of stuff commented out and it's very far from clean. Planned updates:
  - Configurable RSSI threshold
  - Blink only DSSS or OFDM, but never both simultaneously
  - DATA, MGMT, and CTRL LEDs depending on the received frame type
  - Disable Wi-Fi scanning during blinks to save battery power
  - Enable serial data on booth with button press
  - This is a test of git
*/

int scanChannel = 6;                    // Current channel variable, and set it to start on channel 6
// In the future, we could save this value to the EEPROM to persist after poweroffs

// Define intervals and durations

const int minBlinkInterval = 10;        // Minimum amount of time between blinks (not sure this works right now)
const int blinkDuration = 100;          // Amount of time to keep LED's lit (not sure this works right now)
const int minButtonInterval = 200;      // Minimum amount of time between button presses

// Define tickers to keep track of time

unsigned long currentTime = 0;          // Not currently used
unsigned long previousTime = 0;         // Not currently used
unsigned long currentTimeDSSS = 0;
unsigned long currentTimeOFDM = 0;
unsigned long currentTimeMGMT = 0;
unsigned long currentTimeDATA = 0;
unsigned long currentTimeCTRL = 0;
unsigned long currentTimeButton = 0;
unsigned long previousTimeDSSS = 0;
unsigned long previousTimeOFDM = 0;
unsigned long previousTimeMGMT = 0;
unsigned long previousTimeDATA = 0;
unsigned long previousTimeCTRL = 0;
unsigned long previousTimeButton = 0;

// Create state variables, these track whether the given LED is off (0) or on (1)
// These should probably be true/false, idk lol!

int stateOFDM = 0;                      // Current state of the OFDM LEDs
int stateDSSS = 0;                      // Current state of the DSSS LEDs
int stateMGMT = 0;                      // Current state of the MGMT LED
int stateCTRL = 0;                      // Current state of the CTRL LED
int stateDATA = 0;                      // Current state of the DATA LED
int plusButtonState = 0;                // Current state of plus button
int minusButtonState = 0;               // Current state of minus button

// Create scheduling variables, these ensure that only one one thing happens at a time

int processingFrame = 0;           // 0 is not processing a frame, 1 is processing a frame

// Create trigger variables
// These should probably be true/false, idk lol!

byte triggerOFDM = 0;               // Triggers the OFDM LED, 1 is triggered, 0 is untriggered
byte triggerDSSS = 0;               // Triggers the DSSS LED, 1 is triggered, 0 is untriggered
byte triggerMGMT = 0;               // Triggers the MGMT LED, 1 is triggered, 0 is untriggered
byte triggerCTRL = 0;               // Triggers the CTRL LED, 1 is triggered, 0 is untriggered
byte triggerDATA = 0;               // Triggers the DATA LED, 1 is triggered, 0 is untriggered

// Define Arduino pins (Beta board)

/*
  const int OFDMPin = 1;          // LEDs
  const int DSSSPin = 3;          // LEDs
  const int pinMGMT = 5;          // LED
  const int pinCTRL = 4;          // LED
  const int pinDATA = 0;          // LED
  const int plusButton = 2;       // Button
  const int minusButton = 16;     // Button
*/

// Define Arduino pins (board v1.2)

const int OFDMPin = 4;          // LEDs
const int DSSSPin = 0;          // LEDs
const int pinMGMT = 5;          // LED
const int pinCTRL = 1;          // LED
const int pinDATA = 3;          // LED
const int plusButton = 12;      // Button
const int minusButton = 16;     // Button

// Define display pins (Same on Beta and v1.2)

sevensegment Display(14, 15, 13, 2); //CLK, LOAD/CS, DIN, Num of Digits

byte serialEnable = HIGH;          // Controls whether serial output is enabled or not

void setup()
{

  // Define pin modes for inputs and outputs
  pinMode(OFDMPin, OUTPUT);
  pinMode(DSSSPin, OUTPUT);
  pinMode(pinMGMT, OUTPUT);
  pinMode(pinCTRL, OUTPUT);
  pinMode(pinDATA, OUTPUT);
  pinMode(plusButton, INPUT);
  pinMode(minusButton, INPUT);


  //Test LED's

  digitalWrite(pinMGMT, HIGH); // Green LED ON
  delay(400);
  digitalWrite(pinCTRL, HIGH); // Green LED ON
  delay(400);
  digitalWrite(pinDATA, HIGH); // Green LED ON
  delay(400);
  digitalWrite(DSSSPin, HIGH); // Green LED ON
  delay(400);
  digitalWrite(OFDMPin, HIGH); // Green LED ON
  delay(1000);

  //Initialize display and test
  Display.Begin();
  Display.Update(88);

  digitalWrite(OFDMPin, LOW);
  delay(400);
  digitalWrite(DSSSPin, LOW);
  delay(400);
  digitalWrite(pinDATA, LOW);
  delay(400);
  digitalWrite(pinCTRL, LOW);
  delay(400);
  digitalWrite(pinMGMT, LOW);
  delay(400);

  // Set display to current channel
  Display.Update(scanChannel);

  serialEnable = digitalRead(minusButton);

  if (serialEnable == HIGH) {
    Serial.begin(115200);
    Serial.print("Starting serial output!");
  }

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
void wifi_sniffer_packet_handler(uint8_t *buff, uint16_t len)
{
  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

  // Third layer: define pointers to the 802.11 packet header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the packet header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // I get a ton of MCS 0 rate packets. Not sure if this is my environment or something is broken, but it seems strange.
  /*
    if (serialEnable == HIGH) {
       if (ppkt->rx_ctrl.rate == 0){
         Serial.print("Rate=MCS ");
         Serial.print(ppkt->rx_ctrl.mcs);
       } else {
         Serial.print("Rate=");
         Serial.print(ppkt->rx_ctrl.rate);
       }
     }
  */

  // This is a fun thing to flash the current rate on the display instead of the channel but it's so fast it's mostly useless
  /*
    if (ppkt->rx_ctrl.rate == 0) {
      Display.Update(ppkt->rx_ctrl.mcs);
    } else {
      Display.Update(ppkt->rx_ctrl.rate);
    }
  */


  // rate = 0 when an MCS encoding is used. This logic still works, but be wary of using the rate variable alone.
  switch ( ppkt->rx_ctrl.rate) {
    case 1:
      triggerDSSS = 1;
      break;
    case 2:
      triggerDSSS = 1;
      break;
    case 5.5:
      triggerDSSS = 1;
      break;
    case 11:
      triggerDSSS = 1;
      break;
    default:
      triggerOFDM = 1;
      break;
  }

  String type = wifi_pkt_type2str((wifi_promiscuous_pkt_type_t)frame_ctrl->type, (wifi_mgmt_subtypes_t)frame_ctrl->subtype);

  // Dump the frame type to serial
  if (serialEnable == HIGH) {
    Serial.print("Frame type: ");
    Serial.print(type);
    Serial.print("\n");
  }

  String mgmt = "Mgmt:";
  // This is bad and could be improved by just checking the type and not the subtype like the called function does
  if (type == "Data") {
    triggerDATA = 1;
    Serial.print("Data\n");
  } else if (type == "Control") {
    triggerCTRL = 1;
    Serial.print("Control\n");
  } else if (strstr(type.c_str(), mgmt.c_str())) { // See if the string "Mgmt:" is a substring of the type
    triggerMGMT = 1;
  } else {
    Serial.print("Unknown frame type: ");
    Serial.print(type);
    Serial.print("\n");
  }

}

void capture(uint8_t *buff, uint16_t len)         // This seems to callback whenever a packet is heard?
{
  sniffer_buf2 *data = (sniffer_buf2 *)buff;     // No idea what this does (but *data is a pointer, right?)

  // Scanning logic
  // Currently looks for 1, 2.2, 5, 11 mbps, blinks DSSS
  // If it sees anything else, it blinks OFDM
  // I haven't fixed this because I am SOOOOOPER busy!

  switch (data ->rx_ctrl.rate) {
    case 1:
      triggerDSSS = 1;
      break;
    case 2:
      triggerDSSS = 1;
      break;
    case 5.5:
      triggerDSSS = 1;
      break;
    case 11:
      triggerDSSS = 1;
      break;
    default:
      triggerOFDM = 1;
      break;
  }

  // Write the data to serial, so we can see what is going on
  if (serialEnable == HIGH) {
    Serial.print(" Signal Strength: ");
    Serial.print(data->rx_ctrl.rssi);
    Serial.println();
    Serial.print(" Channel: ");
    Serial.print(data->rx_ctrl.channel);
    Serial.println();
    Serial.print(" Data Rate: ");
    Serial.print(data->rx_ctrl.rate);
    Serial.println();
    Serial.print(" MCS Index :");
    Serial.print(data->rx_ctrl.MCS);
    Serial.println();
    Serial.print(" Unknown :");
    Serial.print(data->rx_ctrl.is_group);
    Serial.println();
    Serial.println();
  }

}

void loop() {

  // Update timers
  unsigned long currentTimeDSSS = millis();
  unsigned long currentTimeOFDM = millis();
  unsigned long currentTimeMGMT = millis();
  unsigned long currentTimeCTRL = millis();
  unsigned long currentTimeDATA = millis();
  unsigned long currentTimeButton = millis(); // Set the current time for a button press

  //Serial.println("Tick!");

  // Minus button logic

  minusButtonState = digitalRead(minusButton); // Check the state of the button, copy to variable
  if ((minusButtonState == HIGH) && (currentTimeButton - previousTimeButton >= minButtonInterval)) {
    scanChannel--;              // Decrease the channel by 1
    if (scanChannel == 00) {    // Write new channel to display
      scanChannel = 13;
    }
    Display.Update(scanChannel); // Write new channel to display

    /*Serial.println("Minus button pressed!");
      Serial.println("Channel: ");
      Serial.print(scanChannel);
      Serial.println();*/

    previousTimeButton = currentTimeButton;
    resetScanning();            // Reset scanning so the new channel is used
  }

  // Plus button logic

  plusButtonState = digitalRead(plusButton); // Check the state of the button, copy to variable
  if ((plusButtonState == HIGH) && (currentTimeButton - previousTimeButton >= minButtonInterval)) {
    scanChannel++; // Increase the channel by 1
    if (scanChannel == 14) {
      scanChannel = 1;
    }
    Display.Update(scanChannel); // Write new channel to display

    /*Serial.println("plus button pressed!");
      Serial.println("Channel: ");
      Serial.print(scanChannel);
      Serial.println();*/

    previousTimeButton = currentTimeButton;
    resetScanning();            // Reset scanning so the new channel is used
  }

  // DSSS LEDs Logic

  // Check to see if we need to turn the DSSS LED off
  if ((stateDSSS == 1) && (currentTimeDSSS - previousTimeDSSS >= blinkDuration)) {      // Check to see if we need to turn DSSS off
    digitalWrite(DSSSPin, LOW);                                                         // Turn DSSS pin off
    stateDSSS = 0;                                                                      // Set the DSSS LED state to "off"
    //Serial.println("DSSS off!");
    previousTimeDSSS = currentTimeDSSS;                                                 // Reset the timer
    processingFrame = 0;                                                                // Say that we're no longer processing a frame
    //scanOn();
  }

  // Turn the DSSS LED on
  if ((triggerDSSS == 1) && (currentTimeDSSS - previousTimeDSSS >= minBlinkInterval)) { // Check to see if we need to turn DSSS on
    processingFrame = 1;                                                                // Say that we're busy processing a frame
    stateDSSS = 1;                                                                      // Set DSSS LED state to "on"
    digitalWrite(DSSSPin, HIGH);                                                        // Turn DSSS LED pin on
    //Serial.println("DSSS on!");
    triggerDSSS = 0;                                                                    // Reset the DSSS trigger to "off"
  }

  // OFDM LEDs Logic

  // Check to see if we need to turn the OFDM LED off
  if ((stateOFDM == 1) && (currentTimeOFDM - previousTimeOFDM >= blinkDuration)) {      // Check to see if we need to turn OFDM off
    digitalWrite(OFDMPin, LOW);                                                         // Turn OFDM pin off
    stateOFDM = 0;                                                                      // Set the OFDM LED state to "off"
    //Serial.println("OFDM off!");
    previousTimeOFDM = currentTimeOFDM;                                                 // Reset the timer
    processingFrame = 0;                                                                // Say that we're no longer processing a frame
    //scanOn();
  }

  // Turn the OFDM LED on
  if ((triggerOFDM == 1) && (currentTimeOFDM - previousTimeOFDM >= minBlinkInterval)) { // Check to see if we need to turn OFDM on
    processingFrame = 1;                                                                // Say that we're busy processing a frame
    stateOFDM = 1;                                                                      // Set OFDM LED state to "on"
    digitalWrite(OFDMPin, HIGH);                                                        // Turn OFDM LED pin on
    //Serial.println("OFDM on!");
    triggerOFDM = 0;                                                                    // Reset the OFDM trigger to "off"
  }

  // Check to see if we need to turn the MGMT LED off
  if ((stateMGMT == 1) && (currentTimeMGMT - previousTimeMGMT >= blinkDuration)) {      // Check to see if we need to turn MGMT off
    digitalWrite(pinMGMT, LOW);                                                         // Turn MGMT pin off
    stateMGMT = 0;                                                                      // Set the MGMT LED state to "off"
    //Serial.println("MGMT off!");
    previousTimeMGMT = currentTimeMGMT;                                                 // Reset the timer
    processingFrame = 0;                                                                // Say that we're no longer processing a frame
  }

  // Turn the MGMT LED on
  if ((triggerMGMT == 1) && (currentTimeMGMT - previousTimeMGMT >= minBlinkInterval)) { // Check to see if we need to turn MGMT on
    processingFrame = 1;                                                                // Say that we're busy processing a frame
    stateMGMT = 1;                                                                      // Set MGMT LED state to "on"
    digitalWrite(pinMGMT, HIGH);                                                        // Turn MGMT LED pin on
    //Serial.println("MGMT on!");
    triggerMGMT = 0;                                                                   // Reset the MGMT trigger to "off"
  }

  // Check to see if we need to turn the DATA LED off
  if ((stateDATA == 1) && (currentTimeDATA - previousTimeDATA >= blinkDuration)) {      // Check to see if we need to turn DATA off
    digitalWrite(pinDATA, LOW);                                                         // Turn DATA pin off
    stateDATA = 0;                                                                      // Set the DATA LED state to "off"
    //Serial.println("DATA off!");
    previousTimeDATA = currentTimeDATA;                                                 // Reset the timer
    processingFrame = 0;                                                                // Say that we're no longer processing a frame
  }

  // Turn the DATA LED on
  if ((triggerDATA == 1) && (currentTimeDATA - previousTimeDATA >= minBlinkInterval)) { // Check to see if we need to turn DATA on
    processingFrame = 1;                                                                // Say that we're busy processing a frame
    stateDATA = 1;                                                                      // Set DATA LED state to "on"
    digitalWrite(pinDATA, HIGH);                                                        // Turn DATA LED pin on
    //Serial.println("DATA on!");
    triggerDATA = 0;                                                                   // Reset the DATA trigger to "off"
  }

  // Check to see if we need to turn the CTRL LED off
  if ((stateCTRL == 1) && (currentTimeCTRL - previousTimeCTRL >= blinkDuration)) {      // Check to see if we need to turn CTRL off
    digitalWrite(pinCTRL, LOW);                                                         // Turn CTRL pin off
    stateCTRL = 0;                                                                      // Set the CTRL LED state to "off"
    //Serial.println("CTRL off!");
    previousTimeCTRL = currentTimeCTRL;                                                 // Reset the timer
    processingFrame = 0;                                                                // Say that we're no longer processing a frame
  }

  // Turn the CTRL LED on
  if ((triggerCTRL == 1) && (currentTimeCTRL - previousTimeCTRL >= minBlinkInterval)) { // Check to see if we need to turn CTRL on
    processingFrame = 1;                                                                // Say that we're busy processing a frame
    stateCTRL = 1;                                                                      // Set CTRL LED state to "on"
    digitalWrite(pinCTRL, HIGH);                                                        // Turn CTRL LED pin on
    //Serial.println("CTRL on!");
    triggerCTRL = 0;                                                                   // Reset the CTRL trigger to "off"
  }


}

// Reset scanning, used whenever we change channels with the buttons
void resetScanning() {
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  //   wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
  wifi_set_channel(scanChannel);
}

// Turns scanning off, currently not used

void scanOff() {                          // All commented out right now, but will likely use to stop callbacks and save battery power
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
}

// Turns scanning on, currently not used

void scanOn() {
  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
  wifi_set_channel(scanChannel);
}
