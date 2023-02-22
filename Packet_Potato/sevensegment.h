#include "Arduino.h"

// the MAX7219 address map (datasheet table 2)
#define MAX7219_DECODE_REG      (0x09)
#define MAX7219_INTENSITY_REG   (0x0A)
#define MAX7219_SCANLIMIT_REG   (0x0B)
#define MAX7219_SHUTDOWN_REG    (0X0C)
#define MAX7219_DISPLAYTEST_REG (0x0F)
#define MAX7219_DIGIT_REG(pos)  ((pos) + 1)
// shutdown mode (datasheet table 3)
#define MAX7219_OFF             (0x0)
#define MAX7219_ON              (0x1)
//misc
#define DIGIT_BASE 10
#define DP_FLAG                 (B10000000)
#define BLANK                   (B00000000)

class sevenSegment
{
private:
  int _pinClock;
  int _pinLoad;
  int _pinData;
  int _numDigits;

  void setRegister(byte address, byte value);

public:
  sevenSegment(int pinClock, int pinLoad, int pinData, int numDig);
  // Initialize the display
  void initialize();
  // Write something new to the display, destructively
  void write(unsigned int number);
  // Write custom characters to the display, destructively
  void writeCustom(byte leftNewScreen, byte rightNewScreen);
  // Add new segments to the display while preserving what was there before
  void add(byte side, byte newScreen);
  // Turn off the display
  void off();
};
