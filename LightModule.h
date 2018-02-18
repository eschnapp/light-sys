#ifndef __LIGHTMODULE__H__
#define __LIGHTMODULE__H__
#include <inttypes.h>
#include <Arduino.h>
#include <DallasTemperature.h>

enum LihtModulePinTypes {
	PIN_OUT_FAN_CONTROL,
	PIN_OUT_LIGHT_CONTROL,
	PIN_IN_CONNECTION_IND
};

class SensorAddress 
{
  public:
    DeviceAddress _address;

    String toString();
    bool equals(const SensorAddress& to_ );
    bool equals(const DeviceAddress& to_ );
    bool equals(const String& to_);
};

class LightModule 
{
	private:
		bool 	_initialized;
		bool 	_connected;
    bool  _unitOn;
    bool  _fanOn;
		char	_pinIds[3];
		int		_statusCode;
    char  _unitId;
    float _temp;
    SensorAddress* _sensorAddress;

	
	public:
	
	
		typedef void AlertHandler(const LightModule* const handler_);
	
		LightModule(char unitId_, char fanCtrlPinId_, char lightCtrlPinId_, char conIndPinId_);
		LightModule( const LightModule& from_ );
		
		void initializeModule();
			
		bool switchLight(bool turnOff_ = true);
		bool switchFan(bool turnOff_ = true);	
		bool switchUnit(bool turnOff_=true);
		
		bool is_initialized() { return _initialized; };
		bool is_connected() { return _connected; };
		bool is_unit_on() { return _unitOn; };
    bool is_fan_on() { return _fanOn; };
		int  get_status_code() { return _statusCode; };
    void set_sensor_address(SensorAddress* address_) { _sensorAddress = address_; }
    SensorAddress* get_sensor_address() { return _sensorAddress; }
		char get_unitId() { return _unitId; };
		float getUnitTemp();
		
		bool setModuleTempAlert(char highTemp_, char lowTemp_, const AlertHandler* handler_);
		bool clearModuleTempAlert();

    void io();
    float sampleTemp();
};

#endif
