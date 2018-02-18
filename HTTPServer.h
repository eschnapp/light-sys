#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <utility\debug.h>
#include <ccspi.h>
#include <SD.h>
#include <SPI.h>

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
                                      // There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
                                                          // Since only the first line is parsed this
                                                          // needs to be as large as the maximum action
                                                          // and path plus a little for whitespace and
                                                          // HTTP version.

#define TIMEOUT_MS            500    // Amount of time in milliseconds to wait for
                                     // an incoming request to finish.  Don't set this
                                     // too high or your server could be slow to respond.
                                     
class HTTPServer {
  public:
      typedef bool (*CallbackFunc)(String cmdIn,  Adafruit_CC3000_ClientRef& client);
      
  private:
    int _wifi_cs_pin;
    int _sd_cs_pin;
    int _irq_pin;
    int _vbat_pin;
    int _listenPort;
    String _wifi_ssid;
    String _wifi_pwd;
    int _wifi_type;
    Adafruit_CC3000 cc3000;
    Adafruit_CC3000_Server httpServer;
    uint8_t buffer[BUFFER_SIZE+1];
    int bufindex = 0;
    char action[MAX_ACTION+1];
    char path[MAX_PATH+1];
    bool _initialized = false;
    CallbackFunc _cb;
    
  public:
    HTTPServer( int cs3000_CSPIN
              , int sdcard_CSPIN
              , int cs3000_IRQPIN
              , int cs3000_VBATPIN
              , int listenPort_
              , String wifiSSID_
              , String wifiPWD_
              , int wifiType_
              , CallbackFunc cb_);

    bool initialize();
    void process();
    bool is_initialized() { return _initialized; }
  private:
  //void printDirectory(File dir, int numTabs);
  bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path);
  void parseFirstLine(char* line, char* action, char* path);
  bool displayConnectionDetails(void);
};

#endif
