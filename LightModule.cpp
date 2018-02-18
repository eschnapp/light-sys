#include <FreqCounter.h>

#include "LightModule.h"
#include "error_codes.h"
extern volatile int _rpmCounter;
extern DallasTemperature sensors;


String
SensorAddress::toString() {
  String buf;
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (_address[i] < 16) buf.concat((F("0")));
    buf.concat(String(_address[i], HEX));
  }
  return buf;
}

bool 
SensorAddress::equals(const SensorAddress& to_ ){
  return equals(to_._address);
}
bool 
SensorAddress::equals(const DeviceAddress& to_ ){
  for( int idx = 0; idx < 8; idx++ ) {
    if( _address[idx] != to_[idx] )
      return false;
  }
  return true;
}
bool 
SensorAddress::equals(const String& to_){
    uint8_t val;
    const char* pos = to_.c_str();
    
     /* WARNING: no sanitization or error-checking whatsoever */
    for(int count = 0; count < (to_.length() / 2); count++) {
        sscanf(pos, "%2hhx", &val);
        if( _address[count] != val )
          return false;
        pos += 2;
    }  
  return true;
}

LightModule::LightModule(char unitId_, char fanCtrlPinId_, char lightCtrlPinId_, char conIndPinId_)
:	_initialized( false )
,	_connected( false )
,	_statusCode(-1)
, _unitId( unitId_ )
, _unitOn( false )
, _fanOn( false ) 
, _sensorAddress(NULL)
, _temp(0)
{
	_pinIds[PIN_OUT_FAN_CONTROL] 	= fanCtrlPinId_;
	_pinIds[PIN_OUT_LIGHT_CONTROL] 	= lightCtrlPinId_;
	_pinIds[PIN_IN_CONNECTION_IND]  = conIndPinId_;
};
		
LightModule::LightModule( const LightModule& from_ )
:	_initialized( from_._initialized )
,	_connected( from_._connected )
,	_statusCode(-1)
, _unitId( from_._unitId )
, _unitOn( false )
, _fanOn( false )
, _sensorAddress(from_._sensorAddress)
, _temp(from_._temp)
{
	_pinIds[PIN_OUT_FAN_CONTROL] 	= from_._pinIds[PIN_OUT_FAN_CONTROL];
	_pinIds[PIN_OUT_LIGHT_CONTROL] 	= from_._pinIds[PIN_OUT_LIGHT_CONTROL];
	_pinIds[PIN_IN_CONNECTION_IND]  = from_._pinIds[PIN_IN_CONNECTION_IND];
};


void 
LightModule::initializeModule()
{
  // set the pins...
  Serial.print(F("Initializing Unit: "));
  Serial.println((int)_unitId);
  digitalWrite(_pinIds[PIN_OUT_FAN_CONTROL], LOW); // if not set to low it will click on briefly while pinMode is set...
  digitalWrite(_pinIds[PIN_OUT_LIGHT_CONTROL], LOW); // if not set to low it will click on briefly while pinMode is set...
  pinMode(_pinIds[PIN_OUT_FAN_CONTROL], OUTPUT);
  pinMode(_pinIds[PIN_OUT_LIGHT_CONTROL], OUTPUT);
  pinMode(_pinIds[PIN_IN_CONNECTION_IND], INPUT_PULLUP);
  _initialized = true;
  switchLight(false);
  switchFan(false);
  io();
	return;
};

void
LightModule::io() 
{
  // input
  _connected = (digitalRead(_pinIds[PIN_IN_CONNECTION_IND]) == LOW);
  if( !_connected ) {
    if( _unitOn || _fanOn ){
      switchUnit(false);
    }
  }
  _temp = sampleTemp();

  if( _temp >= 40.0 && _unitOn ) {
    Serial.print(F("TEMP ALERT WARNING FOR UNIT "));
    Serial.print(_unitId);
    Serial.println(F("Shutting Down unit now"));
    switchUnit(false);  
  } 
};

float
LightModule::sampleTemp() {
  if( _sensorAddress == NULL  ) {
    return 0;
  }
  if( !sensors.requestTemperaturesByAddress( _sensorAddress->_address ) ) {
    return 0;
  }
  return sensors.getTempC(_sensorAddress->_address);
}

bool 
LightModule::switchLight(bool turnOn_)
{
  Serial.print(F("LIGHT_STATE_CHANGE ID: "));
  Serial.print((int)_unitId);
  Serial.print(F(" STATE: "));
  Serial.println(turnOn_);
  if( _initialized == false ) {
    _statusCode = ERR_NOT_INITIALIZED;
    return false;
  }

  if( _connected == false && turnOn_ == true ) {
    _statusCode = ERR_NOT_CONNECTED;
    return false;
  }

  digitalWrite( _pinIds[PIN_OUT_LIGHT_CONTROL], ( turnOn_ == true ) ? LOW : HIGH );
  _unitOn = turnOn_;
	return true;
};

bool 
LightModule::switchFan(bool turnOn_)
{
  Serial.print(F("FAN_STATE_CHANGE ID: "));
  Serial.print((int)_unitId);
  Serial.print(F(" STATE: "));
  Serial.println(turnOn_);
  if( _initialized == false ) {
    _statusCode = ERR_NOT_INITIALIZED;
    return false;
  }

  if( _connected == false && turnOn_ == true ) {
    _statusCode = ERR_NOT_CONNECTED;
    return false;
  }

  digitalWrite( _pinIds[PIN_OUT_FAN_CONTROL], ( turnOn_ == true ) ? LOW : HIGH );
  _fanOn = turnOn_;
  return true;
};	

bool 
LightModule::switchUnit(bool turnOn_)
{
  if( turnOn_ == false ) {
    if( switchLight( false ) ){
      return  switchFan( false );
    }
  } else {
    if( switchFan( true ) ) {
      return switchLight( true );
    }
  }
	return false;
};


float
LightModule::getUnitTemp()
{
	return _temp;
};


bool 
LightModule::setModuleTempAlert(char highTemp_, char lowTemp_, const AlertHandler* handler_)
{
	return false;
};

bool 
LightModule::clearModuleTempAlert()
{
	return false;
};

