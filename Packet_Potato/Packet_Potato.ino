
#include <ESP8266WiFi.h>
#include "sevensegment.h"

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

const int minBlinkInterval = 50;        // Minimum amount of time between blinks (not sure this works right now)
const int blinkDuration = 100;          // Amount of time to keep LED's lit (not sure this works right now)
const int minButtonInterval = 200;      // Minimum amount of time between button presses

// Define tickers to keep track of time

unsigned long currentTime = 0;          // Not currently used
unsigned long previousTime = 0;         // Not currently used
unsigned long currentTimeDSSS = 0;
unsigned long currentTimeOFDM = 0;
unsigned long currentTimeButton = 0;
unsigned long previousTimeDSSS = 0;
unsigned long previousTimeOFDM = 0;
unsigned long previousTimeButton = 0;

// Create state variables, these track whether the given LED is off (0) or on (1)
// These should probably be true/false, idk lol!

int stateOFDM = 0;                      // Current state of the OFDM LEDs
int stateDSSS = 0;                      // Current state of the DSSS LEDs 
int stateMGMT = 0;         
int stateCTRL = 0;            
int stateDATA = 0;
int plusButtonState = 0;                // Current state of plus button
int minusButtonState = 0;               // Current state of minus button

// Create scheduling variables, these ensure that only one one thing happens at a time

int processingFrame = 0;           // 0 is not processing a frame, 1 is processing a frame
                                   // This is experimental, currently commented out              

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

sevensegment Display(14,15,13,2); //CLK, LOAD/CS, DIN, Num of Digits

byte serialEnable = 1;          // Controls whether serial output is enabled or not

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

  //Initialize display
  Display.Begin();
  Display.Update(scanChannel);
   
   //Test LED's
   digitalWrite(pinDATA, HIGH); // Green LED ON
   delay(400);
   digitalWrite(pinCTRL, HIGH); // Green LED ON
   delay(400);
   digitalWrite(pinMGMT, HIGH); // Green LED ON
   delay(400);
   digitalWrite(DSSSPin, HIGH); // Green LED ON
   delay(400);
   digitalWrite(OFDMPin, HIGH); // Green LED ON
   delay(800);
   digitalWrite(DSSSPin, LOW); // LEDs OFF
   digitalWrite(OFDMPin, LOW); // Green LED ON
   digitalWrite(pinMGMT, LOW); // Green LED ON
   digitalWrite(pinCTRL, LOW); // Green LED ON
   digitalWrite(pinDATA, LOW); // LEDs OFF

   serialEnable = digitalRead(minusButton);

   if (serialEnable == HIGH) {
      Serial.begin(115200);
      Serial.print("Starting serial output!");
      digitalWrite(pinMGMT, HIGH);
      delay(500);
      digitalWrite(pinMGMT, LOW);
      delay(500);
      digitalWrite(pinMGMT, HIGH);
      delay(500);
      digitalWrite(pinMGMT, LOW);
      delay(500);
      digitalWrite(pinMGMT, HIGH);
      delay(500);
      digitalWrite(pinMGMT, LOW);
      delay(500);
      digitalWrite(pinMGMT, HIGH);
      delay(500);
      digitalWrite(pinMGMT, LOW);
      delay(500);
      digitalWrite(pinMGMT, HIGH);
   }

   //Set station mode, callback, then cycle promiscuous mode
   wifi_set_opmode(STATION_MODE);
   wifi_promiscuous_enable(0);
   WiFi.disconnect();

   wifi_set_promiscuous_rx_cb(capture);
   wifi_promiscuous_enable(1);
   wifi_set_channel(scanChannel);
   
}

struct RxControl {
  signed rssi:8; // signal intensity of packet
  unsigned rate:4;
  unsigned is_group:1;
  unsigned:1;
  unsigned sig_mode:2;          // 0:is 11n packet; 1:is not 11n packet;
  unsigned legacy_length:12;    // If not 11n packet, shows length of packet.
  unsigned damatch0:1;
  unsigned damatch1:1;
  unsigned bssidmatch0:1;
  unsigned bssidmatch1:1;
  unsigned MCS:7;               // If is 11n packet, shows the modulation and code used (range from 0 to 76)
  unsigned CWB:1;               // If is 11n packet, shows if is HT40 packet or not
  unsigned HT_length:16;        // If is 11n packet, shows length of packet.
  unsigned Smoothing:1;
  unsigned Not_Sounding:1;
  unsigned:1;
  unsigned Aggregation:1;
  unsigned STBC:2;
  unsigned FEC_CODING:1;       // If is 11n packet, shows if is LDPC packet or not.
  unsigned SGI:1;
  unsigned rxend_state:8;
  unsigned ampdu_cnt:8;
  unsigned channel:4;         // Which channel this packet in.
  unsigned:12;
};

struct sniffer_buf2 {         // I have absolutely no idea what is going on here, is it creating a struct?
  struct RxControl rx_ctrl;   // This is creating a structure or something...?
  u8 buf[112];                // Only mention of "buf" I can find. 112 refers to the length of the frame?
  u16 cnt;
  u16 len;                    // Length of packet
};

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
    
  if(triggerDSSS == 0) triggerOFDM = 1;   // I really don't remember what this does,
                                          // but I think it's a blanket that basically says, "if we aren't blinking DSSS,
                                          // then definetely blink OFDM no matter what!"
                                          // I don't like this method at all. Not one bit.

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

  unsigned long currentTimeDSSS = millis();   // Set the current time for DSSS .  // Going to try logic where only one LED blinks at a time
  unsigned long currentTimeOFDM = millis();   // Set the current time for OFDM
  unsigned long currentTimeButton = millis(); // Set the current time for a button press
  
  //Serial.println("Tick!");

  // Plus button logic (currently commented out because it doesn't work due to hardware problems)
  plusButtonState = digitalRead(plusButton); // Check the state of the button, copy to variable
  if ((plusButtonState == HIGH) && (currentTimeButton - previousTimeButton >= minButtonInterval)) {
    scanChannel++;                // Increase the channel by 1
    Display.Update(scanChannel);  // Write new channel to display
    if (scanChannel == 14) {
      scanChannel = 1;
    }
    /*Serial.println("Plus button pressed!");
    Serial.println("Channel: ");
    Serial.print(scanChannel);
    Serial.println();*/
    
    previousTimeButton = currentTimeButton;
    resetScanning();            // Reset scanning so the new channel is used
  }
  
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
  
}

// Reset scanning, used whenever we change channels with the buttons

void resetScanning() {
   wifi_set_opmode(STATION_MODE);
   wifi_promiscuous_enable(0);
   WiFi.disconnect();

   wifi_set_promiscuous_rx_cb(capture);
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
   wifi_set_promiscuous_rx_cb(capture);
   wifi_promiscuous_enable(1);
   wifi_set_channel(scanChannel);
}
