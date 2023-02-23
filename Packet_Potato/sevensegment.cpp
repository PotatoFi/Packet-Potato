#include "sevenSegment.h"
#include <Arduino.h>

const byte digitPattern[19] = // was 16 before I started to add stuff
{
  B01111110,  // 0
  B00110000,  // 1
  B01101101,  // 2
  B01111001,  // 3
  B00110011,  // 4
  B01011011,  // 5
  B01011111,  // 6
  B01110000,  // 7
  B01111111,  // 8
  B01111011,  // 9
  B01110111,  // A
  B00011111,  // b
  B01001110,  // C
  B00111101,  // d
  B01001111,  // E
  B01000111   // F
};

int _pinClock;
int _pinLoad;
int _pinData;
int _numDigits;
// byte counter;
byte oldScreen;
byte nowScreen;

//Assign Pins
sevenSegment::sevenSegment(int pinClock, int pinLoad, int pinData, int numDig)
{
  _pinClock = pinClock;
  _pinLoad = pinLoad;
  _pinData = pinData;
  _numDigits = numDig;

  pinMode(_pinClock, OUTPUT);
  pinMode(_pinLoad, OUTPUT);
  pinMode(_pinData, OUTPUT);
}

// Write numbers to registers
void sevenSegment::write(unsigned int number)
{

  unsigned int digitValue;
  byte byteData;

  //Turn off display
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_OFF);

  for (int i = 0; i < _numDigits; i++)
    {
      digitValue = number % DIGIT_BASE;
      number /= DIGIT_BASE;

      byteData = digitPattern[digitValue];

      /*
      // This seems to add decimal places
          if (counter % _numDigits == i)
          {
            byteData |= DP_FLAG;
          }
      */

      // Set Register for this digit
      setRegister(MAX7219_DIGIT_REG(i), byteData);
      if (i == 0) {
        oldScreen = byteData;
      }

      // If the leading number is 0, clear it
      if (i == 1 && byteData == B01111110) {
        setRegister(MAX7219_DIGIT_REG(1), BLANK);
      }
    }
  // Turn display back on
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_ON);
}

// Write custom characters to the display
void sevenSegment::writeCustom(byte leftNewScreen, byte rightNewScreen) {
  //byte nowScreen;
  setRegister(MAX7219_DIGIT_REG(1), BLANK);           // Clear the display
  setRegister(MAX7219_DIGIT_REG(0), BLANK);
  setRegister(MAX7219_DIGIT_REG(1), leftNewScreen);   // Write to the display
  setRegister(MAX7219_DIGIT_REG(0), rightNewScreen);
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_ON);      // Turn the display on
  rightNewScreen = nowScreen;                         // Remember the screen
}

// Add segments without clearing the display
void sevenSegment::add(byte side, byte newScreen) {
  //byte nowScreen;                                   // Create the nowScreen variable
  nowScreen = oldScreen | newScreen;                // Binary OR to merge old and new
  setRegister(MAX7219_DIGIT_REG(side), nowScreen);  // Write to the display
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_ON);    // Turn the display on
  oldScreen = nowScreen;                            // Remember the screen
}

void sevenSegment::initialize()
{
  // disable test mode. datasheet table 10
  setRegister(MAX7219_DISPLAYTEST_REG, MAX7219_OFF);
  // set medium intensity. datasheet table 7
  setRegister(MAX7219_INTENSITY_REG, 0x2); // was 0x8
  // turn off display. datasheet table 3
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_OFF);
  // drive 8 digits. datasheet table 8
  setRegister(MAX7219_SCANLIMIT_REG, 1); // was 7
  // no decode mode for all positions. datasheet table 4
  setRegister(MAX7219_DECODE_REG, B00000000);
}

void sevenSegment::off() {
  setRegister(MAX7219_SHUTDOWN_REG, MAX7219_OFF);
}

//Use to update single registers on MAX7219
void sevenSegment::setRegister(byte address, byte value)
{
  digitalWrite(_pinLoad, LOW);
  shiftOut(_pinData, _pinClock, MSBFIRST, address);
  shiftOut(_pinData, _pinClock, MSBFIRST, value);
  digitalWrite(_pinLoad, HIGH);
}
