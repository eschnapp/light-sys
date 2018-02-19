#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <HardwareSerial.h>

class ConsoleManager
{
  private:
    static const char numChars = 32;
    char receivedChars[numChars];
    char tmpBuf[numChars];
    bool newData = false;
    const HardwareSerial& _serial;
    
  public:
  ConsoleManager(const HardwareSerial& serial_)
  : _serial(serial_)
  {}
    void recvWithStartEndMarkers();
    bool fetchCommand(char* command_buf, size_t command_sz, char* value_buf, size_t value_sz);
};
#endif

