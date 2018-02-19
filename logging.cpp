#include <WString.h>
#include "logging.h"

static int Logger::_logLevel  = 0;

Logger::Logger( const char* name_, const HardwareSerial& serial_ )
: _serial(serial_){
  strncpy( _name, name_, 10 );
}

void 
Logger::p(const char* msg)
{
  _serial.print(msg);
}

void 
Logger::pln(const char* msg)
{
  _serial.println(msg);
}

void Logger::p(const int& val) { Serial.print(val); }
void Logger::p(const long& val){ Serial.print(val); }
void Logger::p(const float& val){ Serial.print(val); }
void Logger::p(const double& val){ Serial.print(val); }
void Logger::p(const char& val){ Serial.print(val); }
void Logger::p(const String& val){ Serial.print(val); }
void Logger::pln(const int& val){ Serial.println(val); }
void Logger::pln(const long& val){ Serial.println(val); }
void Logger::pln(const float& val){ Serial.println(val); }
void Logger::pln(const double& val){ Serial.println(val); }
void Logger::pln(const char& val){ Serial.println(val); }
void Logger::pln(const String& val){ Serial.println(val); }

void
Logger::output( const char* msg, const char* lvl, bool nl_ )
{
  p(F("["));
  p(lvl);
  p(F("] <"));
  p(_name);
  p(F("]: "));
  (nl_ == true) ? pln(msg) : p(msg);
}

void 
Logger::debug( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::DEBUG ) {
    output(msg, (const char *)F("DEBUG"), false);
  }
}

void 
Logger::Debug( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::DEBUG ) {
    output(msg, (const char *)F("DEBUG"), true);
  }
}

void 
Logger::info( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::INFO ) {
    output(msg, (const char *)F("INFO"), false);
  }  
}

void 
Logger::Info( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::INFO ) {
    output(msg, (const char *)F("INFO"), true);
  }  
}

void 
Logger::warn( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::WARN ) {
    output(msg, (const char *)F("WARN"), false);
  }  
}

void 
Logger::Warn( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::WARN ) {
    output(msg, (const char*)F("WARN"), true);
  }    
}

void 
Logger::error( const char* msg )
{
  if( Logger::getLogLevel()>= DebugLevel::ERR ) {
    output(msg, (const char*)F("ERROR"), false);
  }    
}

void 
Logger::Error( const char* msg )
{
  if( Logger::getLogLevel() >= DebugLevel::ERR ) {
    output(msg, (const char*)F("ERROR"), true);
  }    
}

