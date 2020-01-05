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
#define DP_FLAG       (B10000000)
#define DIGIT_BASE 10

class sevensegment
{
private:
  int _pinClk;
  int _pinLoad;
  int _pinData;
  int _numDigits;

  void set_register(byte address, byte value);
  
public:
  sevensegment(int pinclk, int pinload, int pindata, int numdig);

  void Begin();
  void Update(unsigned int number);
};
