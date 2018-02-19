#ifndef __LOGGING_H__
#define __LOGGING_H__
#include <HardwareSerial.h>
enum DebugLevel {
  DEBUG = 0,
  INFO = 1,
  WARN = 2,
  ERR = 3
};

class Logger 
{
  private:
    char _name[20];
    static int _logLevel;
    void output( const char* msg, const char* lvl, bool nl_ );
    const HardwareSerial& _serial;
    
  public:
    Logger( const char* name_, const HardwareSerial& serial_ );

    static int getLogLevel() { return _logLevel; }
    static void setLogLevel(int lvl_) { _logLevel = lvl_; }
    
    void p(const char* msg);
    //void p(const __FlashStringHelper* msg) { p((const char*) msg); }
    void p(const int& val);
    void p(const long& val);
    void p(const float& val);
    void p(const double& val);
    void p(const char& val);
    void p(const String& val);
    void pln( const char* msg );
    void pln(const int& val);
    void pln(const long& val);
    void pln(const float& val);
    void pln(const double& val);
    void pln(const char& val);    
    void pln(const String& val);    
    //void pln(const __FlashStringHelper* msg) { pln((const char*) msg); }
    void debug( const char* msg );
    //void debug( const __FlashStringHelper* msg ) { debug((const char*) msg); }
    void Debug( const char* msg );
    //void Debug( const __FlashStringHelper* msg ) { Debug((const char*) msg); }
    void info( const char* msg );
    //void info( const __FlashStringHelper* msg ) { info((const char*) msg); }
    void Info( const char* msg );
    //void Info( const __FlashStringHelper* msg ) { Info((const char*) msg); }
    void warn( const char* msg );
    //void warn( const __FlashStringHelper* msg ) { warn((const char*) msg); }
    void Warn( const char* msg );
    //void Warn( const __FlashStringHelper* msg ) { Warn((const char*) msg); }
    void error( const char* msg );
    //void error( const __FlashStringHelper* msg ) { error((const char*) msg); }
    void Error( const char* msg );  
    //void Error( const __FlashStringHelper* msg ) { Error((const char*) msg); }
};


#endif
