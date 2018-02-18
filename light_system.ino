#include <ccspi.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LightModule.h"
#include "LightProfile.h"
#include "vector.h"
#include "HTTPServer.h"
#include <SD.h>
#include <SPI.h>

#define MAX_CHANNELS 8
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9
#define BASE_EEPROM_ADDRESS 0
#define WIFI_CS_PIN 10
#define WIFI_IRQ_PIN 3
#define WIFI_VBAT_PIN 5
#define WIFI_SSID "ECC08"
#define WIFI_PWD "allyouneedisloveloveisallyouneed"
#define WIFI_TYPE WLAN_SEC_WPA2
#define SDCARD_CS_PIN 4


bool httpCallback( String cmd,  Adafruit_CC3000_ClientRef& client) {
  return processHttpServerCommand( cmd, client);  
}


Vector<LightModule*> _allChannels;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Vector<SensorAddress*> _tempAddresses;
Vector<profile_pack_t> _profiles;
bool  _autoMode = false;
Vector< unsigned long >_lastProfileCheck;
bool _sdInit = false;
SensorAddress* _cpuTempSensor = NULL;
float _cpuTemp = 0;
SensorAddress* _psuTempSensor = NULL;
float _psuTemp = 0;

HTTPServer _httpServer(  WIFI_CS_PIN
                      , SDCARD_CS_PIN
                      , WIFI_IRQ_PIN
                      , WIFI_VBAT_PIN
                      , 80
                      , WIFI_SSID
                      , WIFI_PWD
                      , WIFI_TYPE
                      , &httpCallback); 

// LightModule* _allChannels[MAX_CHANNELS];


//  Global Map for mapping channel pin IDs by pin type.  
//  T - Tach input pin
//  F - Fan switch control pin
//  L - Light switch control pin
//  C - Connection indicator
//                                                            F   L    C
//char             _globalChannelMap[MAX_CHANNELS][3] =  {{  34, 35,  22},
//                                                        {  36, 37,  24},
//                                                        {  38, 39,  26},
//                                                        {  40, 41,  28},
//                                                        {  42, 43,  23},
//                                                        {  44, 45,  25},
//                                                        {  46, 47,  27},
//                                                        {  48, 49,  29}};
char             _globalChannelMap[MAX_CHANNELS][3] =  {{  48, 49,  22},
                                                        {  46, 47,  24},
                                                        {  44, 45,  26},
                                                        {  42, 43,  28},
                                                        {  40, 41,  23},
                                                        {  38, 39,  25},
                                                        {  36, 37,  27},
                                                        {  34, 35,  29}};



bool processHttpServerCommand( String commandStr,  Adafruit_CC3000_ClientRef& client) {
  // tokenize the commands...
  String subCmd = "";
  String subsubCmd = "";  
  if( commandStr.length() <= 0 ) {
    return false;
  }

  commandStr.replace("%20", " ");
  int ii = commandStr.indexOf(" ");
  if( ii > 0 ) {
    subCmd = commandStr.substring(ii + 1);
    commandStr = commandStr.substring(0, ii);
  }
  ii = subCmd.indexOf(" ");
  if( ii > 0 ) {
    subsubCmd = subCmd.substring(ii + 1);
    subCmd = subCmd.substring(0, ii);
  }  

  Serial.print(F("HTTP Command Received: ["));
  Serial.print(commandStr);
  Serial.print(F("]["));
  Serial.print(subCmd);
  Serial.print(F("]["));
  Serial.print(subsubCmd);
  Serial.println(F("]"));
  
  commandStr.toUpperCase();
  if( commandStr.equals(F("STATUS")) ) {
    client.fastrprintln(F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    client.fastrprint(F("<x-status>"));
    client.fastrprintln(F("<x-cpu-temp>"));
    client.fastrprintln(String(_cpuTemp).c_str());
    client.fastrprintln(F("</x-cpu-temp>"));
    client.fastrprintln(F("<x-psu-temp>"));
    client.fastrprintln(String(_psuTemp).c_str());
    client.fastrprintln(F("</x-psu-temp>"));    
    for( int idx = 0; idx < _allChannels.size(); idx ++ ) {
      client.fastrprint(F("<x-module id='module"));
      client.fastrprint(String(idx).c_str());
      client.fastrprint(F("'> <x-light id='module-light-"));
      client.fastrprint(String(idx).c_str());
      client.fastrprint(F("'>"));
      client.fastrprint(String(_allChannels[idx]->is_unit_on()).c_str());
      client.fastrprint(F("</x-light><x-fan id='module-fan-"));
      client.fastrprint(String(idx).c_str());
      client.fastrprint(F("'>"));
      client.fastrprint(String(_allChannels[idx]->is_fan_on()).c_str());
      client.fastrprint(F("</x-fan><x-temp id='module-temp-"));
      client.fastrprint(String(idx).c_str());
      client.fastrprint(F("'>"));
      client.fastrprint(String(_allChannels[idx]->getUnitTemp()).c_str());
      client.fastrprint(F("'</x-temp></x-module>"));      
    }
    client.fastrprintln(F("</x-status>"));
    return true;
  } else if( commandStr.equals(F("SWITCH")) ) {
    subCmd.toUpperCase();
    if( subCmd.equals(F("ALL")) ) {
      subsubCmd.toUpperCase();
      if( subsubCmd.equals(F("ON")) ) {
           for( int idx = 0; idx < _allChannels.size(); idx ++ ) {
              _allChannels[idx]->switchUnit(false);
           }
          client.fastrprint(F("OK"));
          return true;           
      } else if( subsubCmd.equals(F("OFF")) ){
           for( int idx = 0; idx < _allChannels.size(); idx ++ ) {
              _allChannels[idx]->switchUnit(true);
           }         
            client.fastrprint(F("OK"));
            return true;
      } else {
        client.fastrprint(F("UNKNOWN ACTION "));
        client.fastrprint(subsubCmd.c_str());         
      }
    } else {
      int id = subCmd.toInt();
      if( id > 0 ) {
        subsubCmd.toUpperCase();
        if( subsubCmd.equals(F("ON")) ) {
            _allChannels[id]->switchUnit(false);
            client.fastrprint(F("OK"));
            return true;
        } else if( subsubCmd.equals(F("OFF"))) {
            _allChannels[id]->switchUnit(true);
            client.fastrprint(F("OK"));
            return true;
        } else {
          client.fastrprint(F("UNKNOWN ACTION "));
          client.fastrprint(subsubCmd.c_str());         
        }        
      } else {
        client.fastrprint(F("UNKNOWN TARGET "));
        client.fastrprint(subCmd.c_str());  
      }
    }
  }
  
  return false;
}



//void printAddress(uint8_t* deviceAddress)
//{
//  Serial.print(F("["));
//  Serial.print((int)deviceAddress, HEX);
//  Serial.print(F("] "));
//  for (uint8_t i = 0; i < 8; i++)
//  {
//    // zero pad the address if necessary
//    if (deviceAddress[i] < 16) Serial.print(F("0"));
//    Serial.print(deviceAddress[i], HEX);
//  }
//}

void printProfiles()
{
  Serial.println(F("==================< START ACTIVE PROFILE INFORMATION >===================="));
  for( size_t idx = 0; idx < _profiles.size(); idx++ ){
    Serial.print(F("PROFILE_ID: "));
    Serial.print(idx);
    Serial.print(F(" MILLIS: "));
    Serial.print(_profiles[idx].BITS._millis);
    Serial.print(F(" UNITS_MASK "));
    Serial.print(_profiles[idx].BITS._units, BIN);
    Serial.print(F(" LAST CHECK "));
    Serial.println( _lastProfileCheck[idx] );
  }
  Serial.println(F("==================< END ACTIVE PROFILE INFORMATION >===================="));
}

void saveSensorFile( const DeviceAddress& address, String uid ) {
  if( _sdInit == true ) {
    Serial.println(F("Writing temp sensor file to SD card..."));
    String handle(F("/SENSORS/"));
    handle.concat(uid);
    handle.toUpperCase();
    File f = SD.open(handle, FILE_WRITE);
    if(f) {
      if( f.seek(0) ) {
        for( int idx = 0; idx < sizeof( address ); idx ++ ) {
          f.write( (char)address[idx]);
        }
        f.close();
        Serial.print(F("Sensor calibration saved to sensor file "));
        Serial.println( handle );
      } else {
        Serial.print(F("WARN: Failed to seek sensor file to start position: "));
        Serial.println( handle );        
      }
    } else {
      Serial.print(F("WARN: Failed to open sensor file for writing: "));
      Serial.println( handle );
    }
  } else {
    Serial.print(F("Serial is not initialized, cannot write sensor address file "));
    Serial.println(uid);  
  }
}
void calibrateTempSensor(int id) {
  Serial.print(F("Aquiring temp sensor for channel id "));
  Serial.println(id);
  Serial.println(F("Please rub the sensor in your hand for 30 seconds..."));
  delay(30000); // wait 30 seconds...
  Serial.println(F("Requesting temp from all sensors..."));
  sensors.requestTemperatures();
  float maxC = 0;
  int maxCidx = 0;
  for( int idx = 0; idx < _tempAddresses.size(); idx++ ) {
    uint8_t* add = _tempAddresses[idx]->_address;
    Serial.print(F("Getting temp for sensor "));
    Serial.print(_tempAddresses[idx]->toString());
    float c = sensors.getTempC(add);
    Serial.print(F(":  "));
    Serial.println(c);
    if( c > maxC ) {
      maxC = c;
      maxCidx = idx;
    }
  }
  
  Serial.print(F("Detected temp sensor address "));
  Serial.print( _tempAddresses[maxCidx]->toString());
  Serial.print(F(" With Temp reading: "));
  Serial.println(maxC);
  Serial.print(F("Attaching address to module "));
  Serial.println(id);
  
  _allChannels[id]->set_sensor_address(_tempAddresses[maxCidx]);
  saveSensorFile( _tempAddresses[maxCidx]->_address, String(id, DEC) );
}

void saveProfiles() 
{
  Serial.print(F("Saving total of "));
  Serial.print(_profiles.size());
  Serial.println(F(" profiles to EEPROM..."));
  // write profile count
  byte pc = _profiles.size();
  EEPROM[0] = pc;

  // write all the profiles...
  for( size_t idx = 0; idx < pc; idx++ ) {
    EEPROM.put( (idx*5)+1, _profiles[idx] );
  }
}

void resetProfiles() 
{
  _profiles.clear();
  _lastProfileCheck.clear();
  Serial.println(F("Resetting all profiles..."));
}

void loadProfiles()
{
  _profiles.clear();
  _lastProfileCheck.clear();
  Serial.println(F("Reading profiles from EEPROM..."));
  //read profile count
  byte profileCount = EEPROM[0];
  Serial.print(F("Pofile Count: "));
  Serial.println(profileCount);
  // read as many profiles as there are stored...
  for( byte idx = 0; idx < profileCount; idx++ ) {
    profile_pack_t profile;
    EEPROM.get(((idx*5)+1), profile);
    _profiles.push_back(profile);   
    _lastProfileCheck.push_back(millis());
  }
}


void processConsoleInput() {

  String cmd = Serial.readStringUntil(0);
  String subCmd = "";
  String subsubCmd = "";
  
  if( cmd.length() <= 0 ) {

    return;
  }

  int ii = cmd.indexOf(" ");
  if( ii > 0 ) {
    subCmd = cmd.substring(ii + 1);
    cmd = cmd.substring(0, ii);
  }

  ii = subCmd.indexOf(" ");
  if( ii > 0 ) {
    subsubCmd = subCmd.substring(ii + 1);
    subCmd = subCmd.substring(0, ii);
  }
    
  Serial.print(F("CMD = "));
  Serial.println(cmd);
  Serial.print(F("SUBCMD = "));
  Serial.println(subCmd);
  
  cmd.toLowerCase();
  if( cmd.equals(F("help")) ) {
    Serial.println(F("help      - print this help message"));
    Serial.println(F("status    - print current status of unit objects"));
    Serial.println(F("temp      - temp show/calibrate"));
    Serial.println(F("auto      - switch between manual and auto modes"));
    Serial.println(F("profiles  - switch between manual and auto modes"));
    Serial.println(F("switch    - switch UNIT/LIGHT/FAN OFF or ON"));
  } else if( cmd.equals(F("status")) ) {
    Serial.println(F("==================< STATUS BEGIN >===================="));
    unsigned long tm = millis();
    Serial.print(F("Uptime: ")); 
    Serial.print((unsigned long) (tm / (1000L*60*60*24*7))); Serial.print(F(" weeks "));
    Serial.print((unsigned long) ((tm / (1000L*60*60*24)) % 7)); Serial.print(F(" days "));
    Serial.print((unsigned long) ((tm / (1000L*60*60)) % 24)); Serial.print(F(" hours "));
    Serial.print((unsigned long) ((tm / (1000L*60)) % 60)); Serial.print(F(" minutes "));
    Serial.print((unsigned long) (tm / 1000L) % 60) ; Serial.print(F(" seconds ")); 
    Serial.println(F("."));
    if( _cpuTempSensor != NULL ) {
      Serial.print(F("CPU Temp Sensor: "));
      Serial.println(_cpuTemp);
    }
    if( _psuTempSensor != NULL ) {
      Serial.print(F("PSU Temp Sensor: "));
      Serial.println(_psuTemp);
    }    
    
    Serial.println(F("ID  CONNTECTED UNIT FAN     TEMP     SENSOR ADDRESS"));
    for( int idx = 0; idx < _allChannels.size(); idx++ ){
          Serial.print(idx);
          Serial.print(F("     "));
          Serial.print( _allChannels[idx]->is_connected() );
          Serial.print(F("         "));
          Serial.print( _allChannels[idx]->is_unit_on() );
          Serial.print(F("     ")); 
          Serial.print( _allChannels[idx]->is_fan_on());
          Serial.print(F("     ")); 
          Serial.print( _allChannels[idx]->getUnitTemp());
          Serial.print(F("  "));
          Serial.print(_allChannels[idx]->get_sensor_address() ? _allChannels[idx]->get_sensor_address()->toString() : F("NONE") );
          Serial.println(F(""));        
    }
    Serial.println(F("==================< STATUS END >===================="));
  } else if( cmd.equals(F("temp")) ) {
      if(subCmd.length() > 0 ) {
        subCmd.toLowerCase();
        if( subCmd.equals(F("show")) ) {
          Serial.println(F("ID      ADDRESS                 TEMP"));
          for( int idx = 0; idx < _tempAddresses.size(); idx++ ) {
            if( _tempAddresses[idx] != 0 ) {
              if( sensors.requestTemperaturesByAddress(_tempAddresses[idx]->_address)) {
                float tmpC = sensors.getTempC(_tempAddresses[idx]->_address);
                Serial.print(idx);
                Serial.print(F("       "));
                Serial.print(_tempAddresses[idx]->toString());
                Serial.print(F("   "));
                Serial.println( tmpC );
              }
            }
          }
        } else if(subCmd.equals(F("calibrate"))) {
          if( subsubCmd.length() > 0 ) {
            int id = subsubCmd.toInt();
            calibrateTempSensor(id);
            Serial.println(F("Done."));
          } else {
            Serial.println(F("Must supply unit ID to calibrate for.")); 
          }
        } else if(subCmd.equals(F("set"))) {
          int pos = subsubCmd.indexOf(" ");
          if( pos <= -1 ) {
             Serial.println(F("format must be temp set [ID/CPU] [ADDRESS]"));
          } else {
            String tgt = subsubCmd.substring(0, pos);
            tgt.toUpperCase();
            bool found = false;
            for( int idx = 0; idx < _tempAddresses.size(); idx++ ){
              if( _tempAddresses[idx]->equals( subsubCmd.substring(pos+1) ) ) {
                found = true;
                if( tgt.equals(F("CPU")) ){
                  Serial.print(F("Setting CPU Temp sensor: "));
                  Serial.println(_tempAddresses[idx]->toString());
                  _cpuTempSensor = _tempAddresses[idx];
                  saveSensorFile(_tempAddresses[idx]->_address, F("CPU"));
                } else if( tgt.equals(F("PSU")) ){
                  Serial.print(F("Setting PSU Temp sensor: "));
                  Serial.println(_tempAddresses[idx]->toString());
                  _psuTempSensor = _tempAddresses[idx];
                  saveSensorFile(_tempAddresses[idx]->_address, F("PSU"));                  
                } else {
                  int uid = tgt.toInt();                
                  Serial.print(F("Setting TEMP sensor "));
                  Serial.print(_tempAddresses[idx]->toString());
                  Serial.print(F(" to unit "));
                  Serial.println(uid);
                  
                  _allChannels[idx]->set_sensor_address(_tempAddresses[idx]);
                  saveSensorFile(_tempAddresses[idx]->_address, String(uid, DEC));
                }
              }
            }

            if( !found ) {
                Serial.print(F("WARN: Cannot find matching sensor for address "));
                Serial.println(subsubCmd.substring(pos+1));
            }
          }
        } else {
          Serial.print(F("unknown cmd temp "));
          Serial.println(subCmd);
        }

      } else {
        Serial.println(F("I only know temp [SHOW/CALIBRATE]"));
      }
  } else if( cmd.equals(F("auto")) ) {
    if( subCmd.length() > 0 ) {
      subCmd.toLowerCase();
      if( subCmd.equals(F("on")) || subCmd.equals (F("1")) ) {
        _autoMode = true;
      } else if( subCmd.equals(F("off")) || subCmd.equals(F("0")) ) {
        _autoMode = false;
      }
    }
    Serial.print(F("Auto Mode: "));
    Serial.println(_autoMode);    
    Serial.println(F("Done."));
  } else if( cmd.equals(F("profile")) ) {  
    if( subCmd.length() > 0 ) {
      subCmd.toLowerCase();
      if( subCmd.equals(F("show")) ) {
        printProfiles();
      } else if (subCmd.equals(F("save"))) {
        saveProfiles();
        Serial.println(F("Done."));
      } else if( subCmd.equals(F("load"))) { 
        loadProfiles();
        Serial.println(F("Done."));
      } else if( subCmd.equals(F("reset"))) {
        resetProfiles();
        Serial.println(F("Done."));
      } else if (subCmd.equals(F("delete"))) {
        int id = subsubCmd.toInt();
        Serial.print(F("Deleting profile ID = "));
        Serial.println(id);
        _profiles.remove(id);
        _lastProfileCheck.remove(id);
        Serial.println(F("Done."));
      } else if (subCmd.equals(F("set"))) {
        int cnt = 0;
        for( int pos = 0; pos > -1; cnt++, pos++) {
          pos = subsubCmd.indexOf(" ",pos);
         if( pos == -1 ) break;
        }
        if( cnt < 2 ) {
          Serial.println(F("Format must be PROFILE SET [ID MIN HR DOW MON DOM BITMASK]"));
        } else {
          profile_pack_t profile;
          int cnt = 0;
          for( int pos = 0; pos > -1; cnt++) {
            int npos = subsubCmd.indexOf(" ",pos);
            switch( cnt ) {
              case 0:
                profile.BITS._id = subsubCmd.substring(pos, npos + 1).toInt();
                Serial.print(F("ID = "));
                Serial.println(subsubCmd.substring(pos, npos + 1));
                break;
              case 1:
                profile.BITS._millis = subsubCmd.substring(pos, npos + 1).toInt();
                Serial.print(F("MILLIS = "));
                Serial.println(subsubCmd.substring(pos, npos + 1));
                break;
              case 2:
                profile.BITS._units = subsubCmd.substring(pos).toInt();
                Serial.print(F("UNITS = "));
                Serial.println(subsubCmd.substring(pos).toInt(), BIN);                
                break;                
            }
            pos = npos + 1;
            if( npos == -1 ) break;
          }
          if( profile.BITS._id >= _profiles.size() ) {
            _profiles.push_back(profile);
            _lastProfileCheck.push_back(millis());
          } else {
          _profiles[profile.BITS._id] = profile;
          _lastProfileCheck[profile.BITS._id] = millis();
          }
          Serial.println(F("Done."));
        }
      } else {
        Serial.println(F("I only know profiles SHOW/SAVE/LOAD/SET"));
      }
    }
  } else if( cmd.equals(F("switch")) ) {      
    if(subCmd.length() > 0 && subsubCmd.length() > 0 ) {
      subCmd.toLowerCase();
      int unitId = subsubCmd.toInt();
      if( subCmd.equals(F("fan")) ) {
        _allChannels[unitId]->switchFan(!_allChannels[unitId]->is_fan_on());
      } else if( subCmd.equals(F("unit")) ) {
        _allChannels[unitId]->switchUnit(!_allChannels[unitId]->is_unit_on());
      } else if( subCmd.equals(F("all")) ) {
        for( int idx = 0; idx < _allChannels.size(); idx++ ) {
          _allChannels[idx]->switchUnit(unitId);
        }
        Serial.println(F("Done."));
      } else {
        Serial.println(F("Unknown switch cmd! I only know FAN/UNIT"));
      }
    }
  } else if ( cmd.equals(F("reset")) ) {
     asm ("  jmp 0; ");   
  }  else {
    Serial.print(F("ERROR: unknown command: "));
    Serial.println(cmd);
  }
}

void printDirectory(File dir, int numTabs)
{
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print(F("\t\t"));
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void setup() {
 
  Serial.begin(115200);
  Serial.setTimeout(100);
  Serial.println(F("Light Controller V1.0 Startup Sequence Starting...."));

  // initialize the SD card library...
  if( !SD.begin(SDCARD_CS_PIN) ) {
    Serial.println(F("Couldnt initialize the SD card library!"));
  } else {
    Serial.println(F("SD Card file index: "));
    File root = SD.open(F("/"));
    printDirectory(root, 0);
    root.close();
    if( !SD.exists(F("/SENSORS/"))) {
      Serial.println(F("No sensors folder found - creating new one..."));
      if( !SD.mkdir(F("/SENSORS/"))) {
        Serial.println(F("WARN: Failed to create sensors folder in SD card!!"));
      }
    }
    _sdInit = true;
  }
  
  // setup the temp sensor array...
  sensors.begin();
  int devices = sensors.getDeviceCount();
  if( devices > 0 ) {
    Serial.print(devices);
    Serial.println(F(" Temperature sensors detected..."));
    for( int idx = 0; idx < devices; idx++ ){
      //uint8_t * add = new uint8_t(8);
      SensorAddress*  add = new SensorAddress();
      if( ! sensors.getAddress(add->_address, idx) ) {
        Serial.print(F("Failed to obtain address for device index: "));
        Serial.print(idx);
        continue;
      }
      Serial.print(F("Found TEMP Device: "));
      Serial.println(add->toString());
      _tempAddresses.push_back(add);
    }
  }
  
  // crete the unit modules...
  Serial.print(F("Configured MAX Units: "));
  Serial.println(MAX_CHANNELS);
  char ccnt = 0;
  for( char idx = 0; idx < MAX_CHANNELS; idx++ )
  {
    Serial.print(F("Creating Module for Unit "));
    Serial.println((int)idx);
    LightModule* module = new LightModule ( idx
                                          , _globalChannelMap[idx][PIN_OUT_FAN_CONTROL]
                                          , _globalChannelMap[idx][PIN_OUT_LIGHT_CONTROL]
                                          , _globalChannelMap[idx][PIN_IN_CONNECTION_IND] );
    Serial.print(F("Initializing Module "));
    Serial.println((int) idx);
    module->initializeModule();
    if( !module->is_initialized() ) 
    {
      Serial.print(F("ERROR: Failed to initialize Module " ));
      Serial.println((int) idx);
      delete module;
      continue;
    }

    if( !module->is_connected() )
    {
      Serial.print(F("WARN: Unit is NOT connected! You can still connect the Unit later... ID: "));
      Serial.println((int) idx );
    }
    else 
    {
      ccnt++;
      module->switchUnit(false); // ensure unit is OFF
    }
    Serial.print(F("Module is initalized, ID: "));
    Serial.println((int)idx);
    _allChannels.push_back(module);
  }

  Serial.print(F("Total number of initialized modules: "));
  Serial.println( _allChannels.size());
  Serial.print(F("Total number of conected modules: "));
  Serial.println( (int) ccnt );

  Serial.println(F("Loading Profiles...."));
  loadProfiles();

  if( _sdInit == true ) {
    Serial.println(F("Searching for TEMP sensor config files"));
    File folder = SD.open(F("/SENSORS/"));
    
    while(1) {
        File f = folder.openNextFile();
        if( !f ) break;
        
        SensorAddress a;
        for( int idx = 0; idx < 8; idx ++ ) {
          a._address[idx] = f.read();
          if( a._address[idx] == -1 ) 
            continue;
        }

        String nm = f.name();
        Serial.print(F("Sensor address read for file "));
        Serial.print(nm);
        Serial.print(F(" is: "));
        Serial.println(a.toString());
        int fidx = -1;
        for( int idx = 0; idx < _tempAddresses.size(); idx++ ) {
          if( _tempAddresses[idx]->equals(a) ) {
            fidx = idx;
          }
        }
        if( fidx >= 0 ) {
          Serial.print(F("Located sensor instance... saving calibration to unit "));
          Serial.println(nm);  
          nm.toUpperCase();
          if( nm.equals(F("CPU"))) {
            _cpuTempSensor = _tempAddresses[fidx];
          } else if( nm.equals(F("PSU"))) {
            _psuTempSensor = _tempAddresses[fidx];
          } else {
            _allChannels[nm.toInt()]->set_sensor_address(_tempAddresses[fidx]);
          }
        }
    }
  }
  
  Serial.println(F("Initializing the HTTP Server on port 80..."));
  if( !_httpServer.initialize() ) {
    Serial.println(F("WARNING! Failed to initialize the HTTP server!"));
  }
  Serial.println(F("Light Controller Startup Sequence Complete!"));
}

void loop() {
  // perform profile logic here....
  unsigned long currentT = millis();

  if( _autoMode == true ) {
    for( int idx = 0; idx < _profiles.size(); idx ++ ) {
      long delta = currentT - _lastProfileCheck[idx];
      if(delta < 0 ) {
        // rollover of profile check!
        delta = (4294967295 - _lastProfileCheck[idx]) + currentT;
      } 
      if( _profiles[idx].BITS._millis <= delta ) {
        Serial.print(F("Processing PROFILE ID "));
        Serial.println(idx);
        _lastProfileCheck[idx] = currentT;     
        for( int chid = 0; chid < _allChannels.size(); chid++ ) {
          if( bitRead( _profiles[idx].BITS._units, chid ) == 1 ) {
            _allChannels[chid]->switchUnit( !_allChannels[chid]->is_unit_on() );
          } else {
            //_allChannels[chid]->switchUnit( false );
          }
        }
      }
    }
  }

  // read/write each module...
  for( size_t idx = 0; idx < _allChannels.size(); idx++ )
  {
    _allChannels[idx]->io();
  }

  // read cpu temp sensor if attached
  if( _cpuTempSensor != NULL ) {
    if( sensors.requestTemperaturesByAddress( _cpuTempSensor->_address ) ) {
      _cpuTemp = sensors.getTempC(_cpuTempSensor->_address);
    }    
  }

  if( _psuTempSensor != NULL ) {
    if( sensors.requestTemperaturesByAddress( _psuTempSensor->_address ) ) {
      _psuTemp = sensors.getTempC(_psuTempSensor->_address);
    }    
  }  

  // process HTTP server clients...
  if(_httpServer.is_initialized() ){
    _httpServer.process();
  }
  
  // check for console commands...
  processConsoleInput();
}


