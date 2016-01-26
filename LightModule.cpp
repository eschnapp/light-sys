#include <FreqCounter.h>

#include "LightModule.h"
#include "error_codes.h"
extern volatile int _rpmCounter;
extern DallasTemperature sensors;

LightModule::LightModule(char unitId_, char tachPinId_, char fanCtrlPinId_, char lightCtrlPinId_, char conIndPinId_)
:	_initialized( false )
,	_connected( false )
,	_statusCode(-1)
, _unitId( unitId_ )
, _unitOn( false )
, _fanOn( false ) 
, _rpm(0)
, _sensorAddress(0)
, _temp(0)
{
	_pinIds[PIN_OUT_TACH_CONTROL] 			= tachPinId_;
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
, _rpm(from_._rpm)
, _sensorAddress(from_._sensorAddress)
, _temp(from_._temp)
{
	_pinIds[PIN_OUT_TACH_CONTROL] 			= from_._pinIds[PIN_OUT_TACH_CONTROL];
	_pinIds[PIN_OUT_FAN_CONTROL] 	= from_._pinIds[PIN_OUT_FAN_CONTROL];
	_pinIds[PIN_OUT_LIGHT_CONTROL] 	= from_._pinIds[PIN_OUT_LIGHT_CONTROL];
	_pinIds[PIN_IN_CONNECTION_IND]  = from_._pinIds[PIN_IN_CONNECTION_IND];
};


void 
LightModule::initializeModule()
{
  // set the pins...
  Serial.print("Initializing Unit: ");
  Serial.println(_unitId);
  pinMode(_pinIds[PIN_OUT_TACH_CONTROL], OUTPUT );
  pinMode(_pinIds[PIN_OUT_FAN_CONTROL], OUTPUT);
  pinMode(_pinIds[PIN_OUT_LIGHT_CONTROL], OUTPUT);
  pinMode(_pinIds[PIN_IN_CONNECTION_IND], INPUT);
  //digitalWrite(_pinIds[PIN_IN_TACH], HIGH);
  digitalWrite( _pinIds[PIN_OUT_TACH_CONTROL], LOW);
  _initialized = true;
  switchLight(false);
  switchFan(false);
	return;
};

void
LightModule::io() 
{
  // input
  _connected = (digitalRead(_pinIds[PIN_IN_CONNECTION_IND]) == HIGH);
  if( !_connected ) {
    if( _unitOn || _fanOn ){
      switchUnit(false);
    }
  }
  // TODO: read RPM and temp here...
  _rpm = sampleRpm();
  _temp = sampleTemp();
};

float
LightModule::sampleTemp() {
  if( _sensorAddress == 0 ) {
    return 0;
  }
  if( !sensors.requestTemperaturesByAddress( _sensorAddress ) ) {
    return 0;
  }
  return sensors.getTempC(_sensorAddress);
}

int
LightModule::sampleRpm() {

  noInterrupts();
   _rpmCounter = 0;
//   Serial.print("SAMPLE_RPM [");
//   Serial.print((int)_unitId);
//   Serial.println("]: ");
//  Serial.print("TACH PIN: ");
//  Serial.println((int)_pinIds[PIN_OUT_TACH_CONTROL]);
  
  digitalWrite( _pinIds[PIN_OUT_TACH_CONTROL], HIGH);
//  Serial.println("Tach pin HIGH");
  interrupts();
  delay (500);  //Wait 1 second
  noInterrupts();
//   Serial.println( _rpmCounter );
   int rpm = ((((int)_rpmCounter / 10) * 600)); 
   digitalWrite( _pinIds[PIN_OUT_TACH_CONTROL], LOW);
//  Serial.println("Tach pin LOW");
   interrupts();
   return rpm;
};

bool 
LightModule::switchLight(bool turnOn_)
{
  Serial.print("LIGHT_STATE_CHANGE ID: ");
  Serial.print(_unitId);
  Serial.print(" STATE: ");
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
  Serial.print("FAN_STATE_CHANGE ID: ");
  Serial.print(_unitId);
  Serial.print(" STATE: ");
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

int	
LightModule::getFanRpm()
{
	return _rpm;
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

bool 
LightModule::setModuleRpmAlert(uint8_t rpmHigh_, uint8_t rpmLow_, const AlertHandler* handler_ )
{
	return false;
};

bool 
LightModule::clearModuleRpmAlert()
{
	return false;
};
