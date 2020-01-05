#include "sevensegment.h"
#include <Arduino.h>

const byte digit_pattern[16] =
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

int _pinClk;
int _pinLoad;
int _pinData;
int _numDigits;

//Assign Pins
sevensegment::sevensegment(int pinclk, int pinload, int pindata, int numdig)
{
  _pinClk = pinclk;
  _pinLoad = pinload;
  _pinData = pindata;
  _numDigits = numdig;

  pinMode(_pinClk, OUTPUT);
  pinMode(_pinLoad, OUTPUT);    
  pinMode(_pinData, OUTPUT);
}

//Update Numbers into Registers
void sevensegment::Update(unsigned int number)
{

  unsigned int digit_value;
  byte byte_data;
    
  //Turn off display
  set_register(MAX7219_SHUTDOWN_REG, MAX7219_OFF);

  for (int i = 0; i < _numDigits; i++)
  {
    digit_value = number % DIGIT_BASE;
    number /= DIGIT_BASE;

    byte_data = digit_pattern[digit_value];

  // not sure what this is for
  //    if (counter % _numDigits == i)
  //    {
  //      byte_data |= DP_FLAG;
  //    }

  //Set Register for this digit
  set_register(MAX7219_DIGIT_REG(i), byte_data);
  }
  //Turn display back on
  set_register(MAX7219_SHUTDOWN_REG, MAX7219_ON);
}

void sevensegment::Begin()
{  
  // disable test mode. datasheet table 10
  set_register(MAX7219_DISPLAYTEST_REG, MAX7219_OFF);
  // set medium intensity. datasheet table 7
  set_register(MAX7219_INTENSITY_REG, 0x2); ///was 0x8
  // turn off display. datasheet table 3
  set_register(MAX7219_SHUTDOWN_REG, MAX7219_OFF);
  // drive 8 digits. datasheet table 8
  set_register(MAX7219_SCANLIMIT_REG, 1); // was 7
  // no decode mode for all positions. datasheet table 4
  set_register(MAX7219_DECODE_REG, B00000000);
}

//Use to update single registers on MAX7219
void sevensegment::set_register(byte address, byte value)  
{
  digitalWrite(_pinLoad, LOW);
  shiftOut(_pinData, _pinClk, MSBFIRST, address);
  shiftOut(_pinData, _pinClk, MSBFIRST, value);
  digitalWrite(_pinLoad, HIGH);
}
