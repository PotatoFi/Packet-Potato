
#include <ESP8266WiFi.h>
#include <string>
#include "sdk_structs.h"
#include "ieee80211_structs.h"
#include "string_utils.h"
#include "math.h"
#include "sevensegment.h"

extern "C" {
#include "user_interface.h"
}

// Define special characters and display info
#define LEFT_DISPLAY  (1)
#define RIGHT_DISPLAY (0)
#define DECIMAL       (B10000000)
#define BLANK         (B00000000)
#define c             (B00001101)
#define h             (B00010111)
#define r             (B00000101)
#define S             (B01011011)
#define d             (B00111101)
#define t             (B00001111)
#define o             (B00011101)
#define P             (B01100111)
#define A             (B01110111)
#define O             (B01111110)
#define dash          (B00000001)
//                      PABCDEFG
sevenSegment display(14, 15, 13, 2);

// Display modes
#define CHANNEL       (1)
#define SIGNAL        (2)
#define RATE          (3)
#define RETRY         (4)

#define UP            (1)
#define DOWN          (0)

#define IS_MCS        (1)
#define NOT_MCS       (0)

// Define Arduino pins (board v1.02, v1.03, and v1.04)
const int pinOFDM = 4;
const int pinDSSS = 0;
const int pinMGMT = 5;
const int pinCTRL = 1;
const int pinDATA = 3;
const int plusButton = 12;
const int minusButton = 16;

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
signed minRSSI = -99;
byte frameModulation = 0;
unsigned frameRSSI = 0;
float frameRate = 0;
int retryRate = 0;
bool isRetry = 0;
byte retryBufferIndex;
const int retryBufferSize = 200;
bool retryBuffer[retryBufferSize] = {0};

// Define intervals and durations
const int blinkDuration = 10;          // Amount of time to keep LEDs lit, 10 is good
const int minBlinkInterval = 60;       // Minimum amount of time between starting blinks, 60 is good
const int shortPressInterval = 250;
const int longPressInterval = 750;
const int minDisplayPersist = 2500;    // How long the current display should persist after some events
const int signalFlashDuration = 500;   // In signal threshold edit mode, how long to show signal on screen
const int signalFlashInterval = 100;   // In signal threshold edit mode, how long to blank the screen
int eyeOpenInterval = 0;
int eyeClosedDuration = 0;

// Define timers
unsigned long whenPlusPressed = 0;
unsigned long whenMinusPressed = 0;
unsigned long whenBlinked = 0;
unsigned long whenDisplayPersist = 0;   // Persists current value on display after mode changes
unsigned long whenSignalFlash = 0;      // For flashing the signal in signal edit mode
unsigned long whenEyeClosed = 0;
unsigned long idleTimer = 0;            // For tracking how long the display has been idle

// State variables
bool plusButtonState = false;
bool minusButtonState = false;
bool plusButtonEvent = false;
bool minusButtonEvent = false;
bool blinkingState = false;
bool signalFlashState = false;
bool signalEditState = false;
bool displayPersist = false;       // Track if the screen needs to persist or not
bool idleMode = false;
bool eyeOpenState = false;

bool serialEnable = false;
bool signalMode = false;
byte displayMode = CHANNEL;

void setup() {

  // Define pin modes for inputs and outputs
  pinMode(pinOFDM, OUTPUT);
  pinMode(pinDSSS, OUTPUT);
  pinMode(pinMGMT, OUTPUT);
  pinMode(pinCTRL, OUTPUT);
  pinMode(pinDATA, OUTPUT);
  pinMode(plusButton, INPUT);
  pinMode(minusButton, INPUT);

  display.initialize();

  display.writeCustom(P, o);
  delay(400);
  display.writeCustom(o, t);
  delay(100);
  display.writeCustom(t, A);
  delay(400);
  display.writeCustom(A, t);
  delay(100);
  display.writeCustom(t, o);
  
  delay(500);
  if (digitalRead(minusButton) || digitalRead(plusButton)) {
    serialEnable = true;
  }
  delay(500);

  if (serialEnable) {
    Serial.begin(115200);
    Serial.print("\n");
    Serial.print("Welcome to the Packet Potato. Serial output is enabled at 115200 baud.");
    Serial.print("\n");
  }

  // Loop through numbers 0-99
  for (int initDisplay = 0 ; initDisplay <= 99 ; initDisplay++) {
      display.write(initDisplay);
      if (initDisplay >= 25) { digitalWrite(pinDSSS, HIGH); }
      if (initDisplay >= 40) { digitalWrite(pinOFDM, HIGH); }
      if (initDisplay >= 55) { digitalWrite(pinDATA, HIGH); }
      if (initDisplay >= 70) { digitalWrite(pinCTRL, HIGH); }
      if (initDisplay >= 85) { digitalWrite(pinMGMT, HIGH); }
      delay(25);
  }

  delay(250);
  
  // Loop through numbers 0-99
  for (int initDisplay = 99 ; initDisplay >= scanChannel ; initDisplay--) {
      display.write(initDisplay);
      if (initDisplay <= 25) { digitalWrite(pinDSSS, LOW); }
      if (initDisplay <= 40) { digitalWrite(pinOFDM, LOW); }
      if (initDisplay <= 55) { digitalWrite(pinDATA, LOW); }
      if (initDisplay <= 70) { digitalWrite(pinCTRL, LOW); }
      if (initDisplay <= 85) { digitalWrite(pinMGMT, LOW); }
      delay(10);
  }

  // Set display to current channel
  display.write(scanChannel);

  //Set station mode, callback, then cycle promiscuous mode
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  wifi_set_channel(scanChannel);
  //   wifi_set_promiscuous_rx_cb(capture);
  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);

  // Reset the idle animation timer
  idleReset();
}

struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;         // 0:is 11n frame, 1:is not 11n frame
  unsigned legacy_length: 12;   // If not 11n frame, shows length of the frame
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;              // If is 11n frame, shows the modulation and code used (range from 0 to 76)
  unsigned CWB: 1;              // If is 11n frame, shows if is HT40 frame or not
  unsigned HT_length: 16;       // If is 11n frame, shows length of frame
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;      // If is 11n frame, shows if is LDPC frame or not
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;         // Which channel this frame in
  unsigned: 12;
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  u8 buf[112];
  u16 cnt;
  u16 len;                    // Length of frame
};

void wifi_sniffer_packet_handler(uint8_t *buff, uint16_t len) {
    
  // First layer: type cast the received buffer into the generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  // Second layer: define pointer to where the actual 802.11 frame is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

  // Third layer: define pointers to the 802.11 frame header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the frame header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // Get the frame type
  byte frameType = getFrameType((wifi_promiscuous_pkt_type_t)frame_ctrl->type);

  // If it's a data frame, get the retry bit and update the buffer
  if ((frameType == 2) && (ppkt->rx_ctrl.rssi >= minRSSI)) {
    updateRetryBuffer(frame_ctrl->retry);
  }

  if ((millis() - whenBlinked >= minBlinkInterval) && (blinkingState == false) && (ppkt->rx_ctrl.rssi >= minRSSI)) {
    blinkingState = true;

    // Convert RSSI, if the display mode is enabled
    if (displayMode == SIGNAL) {
      frameRSSI = ppkt->rx_ctrl.rssi * -1;       // Convert RSSI to a positive number
    }
    
    if (ppkt->rx_ctrl.mcs != 0) {
      blinkOn(1,frameType,ppkt->rx_ctrl.mcs,frameRSSI,IS_MCS,isRetry);
    }
    else {
      switch (ppkt->rx_ctrl.rate) { // 0 is DSSS, 1 is OFDM
        case 0:
          frameRate = 1;
          blinkOn(0,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 1:
          frameRate = 2;
          blinkOn(0,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 2:
          frameRate = 5.5;
          blinkOn(0,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 3:
          frameRate = 11;
          blinkOn(0,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 11:
          frameRate = 6; 
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 15:
          frameRate = 9;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 10:
          frameRate = 12;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 14:
          frameRate = 18;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 9:
          frameRate = 24;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 13:
          frameRate = 36;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 8:
          frameRate = 48;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
        case 12:
          frameRate = 54;
          blinkOn(1,frameType,frameRate,frameRSSI,NOT_MCS,isRetry);
          break;
      }
    }

    if (serialEnable) {
      // Get strings for the subframe type
      String type = wifi_pkt_type2str((wifi_promiscuous_pkt_type_t)frame_ctrl->type, (wifi_mgmt_subtypes_t)frame_ctrl->subtype);
      Serial.print("Frame Type: ");
      Serial.print(type);
      Serial.print("\n");
      
      if (ppkt->rx_ctrl.mcs != 0) {
        Serial.print("MCS: ");
        Serial.print(ppkt->rx_ctrl.mcs);
        Serial.print("\n");
      }
      
      else {
      Serial.print("Data Rate: ");
      Serial.print(frameRate,1);
      Serial.print("\n");
      }

      Serial.print("RSSI: ");
      Serial.print(ppkt->rx_ctrl.rssi);
      Serial.print("\n");

      if (frameType == 2) {
        Serial.print("Retry: ");
        Serial.print(frame_ctrl->retry);
        Serial.print("\n");
      }
      else {
        Serial.print("Retry: N/A");
        Serial.print("\n");
      }

      Serial.print("Retry Rate: ");
      Serial.print(getRetry());
      Serial.print("%");
      Serial.print("\n");
    
      Serial.print("\n");
    }
  }
}

void capture(uint8_t *buff, uint16_t len) 
{
  sniffer_buf2 *data = (sniffer_buf2 *)buff;
}

void loop() {

  if ((millis() - whenBlinked >= blinkDuration)) {
    blinkOff();
    blinkingState = false;
  }

  // Start timer for plus button press
  plusButtonState = digitalRead(plusButton); // Check button state
  if ((plusButtonState == HIGH) && (plusButtonEvent == false) && (millis() - whenPlusPressed >= shortPressInterval)) { 
    whenPlusPressed = millis();
    plusButtonEvent = true;
  }

  // Plus button short press
  if ((plusButtonState == LOW) && (plusButtonEvent == true) && (millis() - whenPlusPressed <= longPressInterval)) {
    if (displayMode == SIGNAL) { inputSignalUp(); }
    else {inputChannelUp(); }
    plusButtonEvent = false;
    idleReset();
  }

  // Plus button long press
  if ((plusButtonState == HIGH) && (millis() - whenPlusPressed >= longPressInterval)) {
    inputModeUp();
    plusButtonEvent = false;
    signalEditState = false;  // Cancel signal edit mode
    idleReset();
  }

  // Start a timer for a minus button press
  minusButtonState = digitalRead(minusButton); // Check button state
  if ((minusButtonState == HIGH) && (minusButtonEvent == false) && (millis() - whenMinusPressed >= shortPressInterval)) { 
    whenMinusPressed = millis();
    minusButtonEvent = true;
  }

  // Minus button short press
  if ((minusButtonState == LOW) && (minusButtonEvent == true) && (millis() - whenMinusPressed <= longPressInterval)) {
    if (displayMode == SIGNAL) {inputSignalDown(); }
    else {inputChannelDown(); }   
    minusButtonEvent = false;
    idleReset();
  }

  // Minus button long press
  if ((minusButtonState == HIGH) && (millis() - whenMinusPressed >= longPressInterval)) {
    inputModeDown();    
    minusButtonEvent = false;
    signalEditState = false;  // Cancel signal edit mode
    idleReset();
  }

  if (signalEditState == true) {
    if ((signalFlashState == false) && (millis() - whenSignalFlash >= signalFlashDuration)) {
      display.writeCustom(BLANK, BLANK);
      whenSignalFlash = millis();
      signalFlashState = true;
    }
    if ((signalFlashState == true) && (millis() - whenSignalFlash >= signalFlashInterval)) {
      display.write(minRSSI * -1);
      whenSignalFlash = millis();
      signalFlashState = false;  
    }
  }

  // Release the display persistence flag when enough time has elapsed, reset chanel display and signal edit mode
  if ((displayPersist == true) && (millis() - whenDisplayPersist >= minDisplayPersist)) {
      displayPersist = false;               // Allow the display to be overwritten
      if (displayMode == CHANNEL) {
        display.write(scanChannel);
      }
      if (displayMode == SIGNAL)  {
        signalEditState = false;
      }
  }

  // When the screen hasn't been updated in awhile, enable idle mode
  if ((idleMode == false) && (millis() - idleTimer >= 5000)) {
    display.writeCustom(dash,dash);  
    idleMode = true;
    eyeOpenState = false;
    whenEyeClosed = millis();
    eyeClosedDuration = random(250,500);
  }

  // Blinks and winks
  if (idleMode == true) {

    if ((eyeOpenState == false) && (millis() - whenEyeClosed >= eyeClosedDuration)) {
      display.writeCustom(O,O);        // Open eyes
      whenEyeClosed = millis();
      eyeOpenInterval = random(250,5000);
      eyeOpenState = true;
    }

    if (eyeOpenState == true) {

      if ((millis() - whenEyeClosed >= eyeOpenInterval - 500)) {
        display.writeCustom(dash, dash);  // Close eyes
        whenEyeClosed = millis();
        eyeClosedDuration = random(60,200);
        eyeOpenState = false;  
      }

      if ((millis() - whenEyeClosed >= eyeOpenInterval)) {
        display.writeCustom(O,dash);  // Wink
        whenEyeClosed = millis();
        eyeClosedDuration = random(280,320);
        eyeOpenState = false;
      }
    }
  }
}

// Reset scanning, used whenever we change channels with the buttons
void resetScanning() {
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  for (int i = 0; i < retryBufferSize; i++) {
    retryBuffer[i] = 0;
  }

  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  wifi_promiscuous_enable(1);
  wifi_set_channel(scanChannel);
}

void blinkOn(boolean modulationType, byte frameType, float displayRate, int displayRSSI, bool isMCS, bool isRetry) {
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
  if (!displayPersist) {
    if (displayMode == SIGNAL) {
      display.write(displayRSSI);
    }
    if (displayMode == RATE) {
      display.write(displayRate);
      if (isMCS) {
        display.add(RIGHT_DISPLAY, DECIMAL);
        }
    }
    if (displayMode == RETRY) {
      display.write(getRetry());
      if (isRetry) {
        display.add(RIGHT_DISPLAY, DECIMAL);
      }
    }      
  }
  whenBlinked = millis();
  idleReset();
}

void blinkOff() {
  digitalWrite(pinDSSS, LOW);
  digitalWrite(pinOFDM, LOW);
  digitalWrite(pinMGMT, LOW);
  digitalWrite(pinCTRL, LOW);
  digitalWrite(pinDATA, LOW);
}

void indicateDisplayMode(byte direction) {
  
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  blinkOff();

  if (displayMode == CHANNEL) {
    delay(100);
    display.writeCustom(c,h);
    animateDisplayMode(direction);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinMGMT, HIGH);
        digitalWrite(pinCTRL, HIGH);
        digitalWrite(pinDATA, HIGH);
        delay(100);
        digitalWrite(pinMGMT, LOW);
        digitalWrite(pinCTRL, LOW);
        digitalWrite(pinDATA, LOW);
        delay(100);
      }
  }

  if (displayMode == SIGNAL) {
    delay(100);
    display.writeCustom(S,t);
    animateDisplayMode(direction);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinDATA, HIGH);
        delay(100);
        digitalWrite(pinDATA, LOW);
        delay(100);
      }
  }

  if (displayMode == RATE) {
    delay(100);
    display.writeCustom(d,r);
    animateDisplayMode(direction);
      for (int blinkLoop = 0 ; blinkLoop <= 3 ; blinkLoop++) {
        digitalWrite(pinCTRL, HIGH);
        delay(100);
        digitalWrite(pinCTRL, LOW);
        delay(100);
      }
  }

  if (displayMode == RETRY) {
    delay(100);
    display.writeCustom(r,r);
    animateDisplayMode(direction);
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
}

void animateDisplayMode(byte direction) {
  if (direction == UP) {
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
  }
  if (direction == DOWN) {
    digitalWrite(pinMGMT, HIGH);
    delay(100);
    digitalWrite(pinCTRL, HIGH);
    delay(100);
    digitalWrite(pinDATA, HIGH);
    delay(100);
    digitalWrite(pinMGMT, LOW);
    delay(100);
    digitalWrite(pinCTRL, LOW);
    delay(100);
    digitalWrite(pinDATA, LOW);
    delay(100);
  }
}

void inputChannelUp() {
  scanChannel++;
  if (scanChannel == 14) {
    scanChannel = 1;
  }
  display.write(scanChannel);
  displayPersist = true;
  whenDisplayPersist = millis();
  resetScanning();
}

void inputChannelDown() {
  scanChannel--;
  if (scanChannel == 0) {
    scanChannel = 13;
  }
  display.write(scanChannel);
  displayPersist = true;
  whenDisplayPersist = millis();
  resetScanning();
}

void inputSignalUp() {
  minRSSI = minRSSI + 5;
  if (minRSSI >= -20) {     // Loop back down to -99 dBm
    minRSSI = -99;
  }
    if (minRSSI == -94) {   // If we land on -94, force to -95
    minRSSI = -95;
  }
  display.write(minRSSI * -1);
  whenDisplayPersist = millis();
  displayPersist = true;
  signalEditState = true;
}

void inputSignalDown() {
  minRSSI = minRSSI - 5;
  if (minRSSI < -100) {     // Loop back up to -25 dBm
    minRSSI = -25;
  }
  if (minRSSI == -100) {    // If we land on -100, force to -99
    minRSSI = -99;
  }
  display.write(minRSSI * -1);
  whenDisplayPersist = millis();
  displayPersist = true;
  signalEditState = true;
}

void inputModeUp() {
  displayMode++;
  if (displayMode == 5) {
    displayMode = 1;
  }
  whenDisplayPersist = millis();
  displayPersist = true;
  indicateDisplayMode(UP);
}

void inputModeDown() {
  displayMode--;
  if (displayMode == 0) {
    displayMode = 4;
  }
  whenDisplayPersist = millis();
  displayPersist = true;
  indicateDisplayMode(DOWN);
}

void updateRetryBuffer(bool retryDataPoint) {
  if (retryBufferIndex == (retryBufferSize)) {
    retryBufferIndex = 0;
  }
  retryBuffer[retryBufferIndex] = retryDataPoint;     // Put the new data point in the buffer
  retryBufferIndex++;                                 // Go to the next position in the buffer
}

int getRetry() {
  float retryBufferCount = 0; 
  float retryRateFloat = 0;
  int retryRateRounded = 0;

  // Check the contents of the buffer
  for (int retryBufferIndex = 0 ; retryBufferIndex < (retryBufferSize - 1) ; retryBufferIndex++) {
    if (retryBuffer[retryBufferIndex] == 1) { // Whenever a retry is found...
      retryBufferCount++;                     // Add to the number of retries
    }
  }
  retryRateFloat = (float)retryBufferCount / retryBufferSize * 100; // Calculate the average
  retryRateRounded = round(retryRateFloat);                         // Convert to an integer
  return retryRateRounded;
}

void idleReset() {
  idleMode = false;
  idleTimer = millis();
}