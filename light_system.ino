#include <EEPROM.h>

#include <StackList.h>

#include <DallasTemperature.h>


#include <OneWire.h>
#include <DallasTemperature.h>
#include "LightModule.h"
#include "LightProfile.h"
#include "vector.h"
#define MAX_CHANNELS 2
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 9
#define RPM_TACH_PIN_ID 2
#define BASE_EEPROM_ADDRESS 0


Vector<LightModule*> _allChannels;
volatile int _rpmCounter = 0;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Vector<SensorAddress*> _tempAddresses;
Vector<profile_pack_t> _profiles;
bool  _autoMode = false;


// LightModule* _allChannels[MAX_CHANNELS];


//  Global Map for mapping channel pin IDs by pin type.  
//  T - Tach input pin
//  F - Fan switch control pin
//  L - Light switch control pin
//  C - Connection indicator
//                                                       T     F   L    C
char             _globalChannelMap[MAX_CHANNELS][4] =  {{10,   6,  7,   8},
                                                        {22,  24, 26,  28}};
//                                                        {23,  25, 26,  27},
//                                                        {28,  29, 30,  31},
//                                                        {32,  33, 34,  35},
//                                                        {36,  37, 38,  39},
//                                                        {40,  41, 42,  43},
//                                                        {44,  45, 46,  47}};

void rpm() {
  _rpmCounter++;
};

void setup() {
  Serial.begin(9600);
  Serial.println("Light Controller V1.0 Startup Sequence Starting....");

  // handle general pins and interrupt status...
  pinMode(RPM_TACH_PIN_ID, INPUT);
  digitalWrite(RPM_TACH_PIN_ID, HIGH);
  attachInterrupt(digitalPinToInterrupt(RPM_TACH_PIN_ID), rpm, RISING);
 // noInterrupts();
  _rpmCounter = 0;

  // setup the temp sensor array...
  sensors.begin();
  int devices = sensors.getDeviceCount();
  if( devices > 0 ) {
    Serial.print(devices);
    Serial.print(" Temperature sensors detected...");
    for( int idx = 0; idx < devices; idx++ ){
      //uint8_t * add = new uint8_t(8);
      SensorAddress*  add = new SensorAddress();
      if( ! sensors.getAddress(add->_address, idx) ) {
        Serial.print("Failed to obtain address for device index: ");
        Serial.print(idx);
        continue;
      }
      Serial.print("Found TEMP Device: ");
      printAddress(add->_address);
      Serial.println();
      _tempAddresses.push_back(add);
    }
  }
  
  // crete the unit modules...
  Serial.print("Configured MAX Units: ");
  Serial.println(MAX_CHANNELS);
  char ccnt = 0;
  for( char idx = 0; idx < MAX_CHANNELS; idx++ )
  {
    Serial.print("Creating Module for Unit ");
    Serial.println((int)idx);
    LightModule* module = new LightModule ( idx
                                          , _globalChannelMap[idx][PIN_OUT_TACH_CONTROL]
                                          , _globalChannelMap[idx][PIN_OUT_FAN_CONTROL]
                                          , _globalChannelMap[idx][PIN_OUT_LIGHT_CONTROL]
                                          , _globalChannelMap[idx][PIN_IN_CONNECTION_IND] );
    Serial.print("Initializing Module ");
    Serial.println((int) idx);
    module->initializeModule();
    if( !module->is_initialized() ) 
    {
      Serial.print("ERROR: Failed to initialize Module " );
      Serial.println((int) idx);
      delete module;
      continue;
    }

    if( !module->is_connected() )
    {
      Serial.print("WARN: Unit is NOT connected! You can still connect the Unit later... ID: " );
      Serial.println((int) idx );
    }
    else 
    {
      ccnt++;
      module->switchUnit(false); // ensure unit is OFF
    }
    Serial.print("Module is initalized, ID: " );
    Serial.println((int)idx);
    _allChannels.push_back(module);
  }

  Serial.print("Total number of initialized modules: ");
  Serial.println( _allChannels.size());
  Serial.print("Total number of conected modules: ");
  Serial.println( ccnt );

  Serial.println("Loading Profiles....");
  loadProfiles();
  
  Serial.println("Light Controller Startup Sequence Complete!");
}

void loop() {
  // perform profile logic here....

  // read/write each module...
  for( size_t idx = 0; idx < _allChannels.size(); idx++ )
  {
    _allChannels[idx]->io();
  }
  
  // check for console commands...
  processConsoleInput();
  
//  String cmd = Serial.readStringUntil(0);
//  
//  if( cmd.length() > 0 ) {
//    if( cmd.startsWith("unit") == true ) {
//      int b = 5;
//      int id = cmd.substring(b,b+1).toInt();
//      int stat = cmd.substring(b+2,b+3).toInt();
//      _allChannels[id]->switchUnit( stat == 1 );   
//    } else if( cmd.startsWith("light") == true ) {
//      int b = 6;
//      int id = cmd.substring(b,b+1).toInt();
//      int stat = cmd.substring(b+2,b+3).toInt();
//      _allChannels[id]->switchLight( stat == 1 );
//    } else if( cmd.startsWith("fan")  == true ) {
//      int b = 4;
//      int id = cmd.substring(b,b+1).toInt();
//      int stat = cmd.substring(b+2,b+3).toInt();
//      _allChannels[id]->switchFan( stat == 1 );      
//    } else if( cmd.startsWith("temp") == true ) {
//      int b = 5;
//      int id = cmd.substring(b,b+1).toInt();
//      String scmd = cmd.substring(b+2);
//      if( scmd.equals("show") == true ) {
//        Serial.println("Not showing temps yet...");
//      } else if( scmd.equals("setup") == true ) {
//        Serial.print("Aquiring temp sensor for channel id ");
//        Serial.println(id);
//        Serial.println("Please hold the sensor in your hand for 20 seconds...");
//        delay(20000); // wait 20 seconds...
//        Serial.println("Requesting temp from all sensors...");
//        sensors.requestTemperatures();
//        float maxC = 0;
//        int maxCidx = 0;
//        for( int idx = 0; idx < _tempAddresses.size(); idx++ ) {
//          uint8_t* add = _tempAddresses[idx]->_address;
//          Serial.print("Getting temp for sensor ");
//          printAddress(add);
//          float c = sensors.getTempC(add);
//          Serial.print(":  ");
//          Serial.println(c);
//          if( c > maxC ) {
//            maxC = c;
//            maxCidx = idx;
//          }
//        }
//
//        Serial.print("Detected temp sensor address ");
//        printAddress( _tempAddresses[maxCidx]->_address );
//        Serial.print(" With Temp reading: ");
//        Serial.println(maxC);
//        Serial.print("Attaching address to module ");
//        Serial.println(id);
//
//        _allChannels[id]->set_sensor_address(_tempAddresses[maxCidx]->_address);
//      }
//      
//    }
//  }
  delay(100);
}

void printAddress(uint8_t* deviceAddress)
{
  Serial.print("[");
  Serial.print((int)deviceAddress, HEX);
  Serial.print("] ");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void printProfiles()
{
  Serial.println("==================< START ACTIVE PROFILE INFORMATION >====================");
  for( size_t idx = 0; idx < _profiles.size(); idx++ ){
    Serial.print("PROFILE_ID: ");
    Serial.print(idx);
    Serial.print(" MIN: ");
    Serial.print(_profiles[idx].BITS._minute);
    Serial.print(" HOUR: ");
    Serial.print(_profiles[idx].BITS._hour);
    Serial.print(" DOW: ");
    Serial.print(_profiles[idx].BITS._dow);
    Serial.print(" MONTH: ");
    Serial.print(_profiles[idx].BITS._month);
    Serial.print(" DOM: ");
    Serial.print(_profiles[idx].BITS._dom);
    Serial.print(" UNITS_MASK ");
    Serial.print(_profiles[idx].BITS._units, BIN);
  }
  Serial.println("==================< END ACTIVE PROFILE INFORMATION >====================");
}

void processConsoleInput() {
  String cmd = Serial.readStringUntil(0);
  if( cmd.length() <= 0 ) {
    return;
  }

  Vector<String> commands;
  split(cmd, " ", commands);

  cmd = commands[0];
  cmd.toLowerCase();
  if( cmd.equals("help") ) {
    Serial.println("help      - print this help message");
    Serial.println("status    - print current status of unit objects");
    Serial.println("settings  - change unit settings");
    Serial.println("mode      - switch between manual and auto modes");
    Serial.println("profiles  - switch between manual and auto modes");
    Serial.println("switch    - switch UNIT/LIGHT/FAN OFF or ON");
  } else if( cmd.equals("status") ) {
    Serial.println("==================< STATUS BEGIN >====================");
    Serial.println("ID  CONNTECTED UNIT FAN  FAN_RPM  TEMP");
    for( int idx = 0; idx < _allChannels.size(); idx++ ){
          Serial.print(idx);
          Serial.print("     ");
          Serial.print( _allChannels[idx]->is_connected() );
          Serial.print("     ");
          Serial.print( _allChannels[idx]->is_unit_on() );
          Serial.print("     "); 
          Serial.print( _allChannels[idx]->is_fan_on());
          Serial.print("     "); 
          Serial.print( _allChannels[idx]->getFanRpm());     
          Serial.print("     "); 
          Serial.print( _allChannels[idx]->getUnitTemp());                          
          Serial.println("");        
    }
    Serial.println("==================< STATUS END >====================");
  } else if( cmd.equals("settings") ) {
    Serial.print("soon");
  } else if( cmd.equals("mode") ) {
    if( commands.size() > 1 ) {
      cmd = commands[1];
      cmd.toLowerCase();
      if( cmd.equals("on") || cmd.equals ("1") ) {
        _autoMode = true;
      } else if( cmd.equals("off") || cmd.equals("0") ) {
        _autoMode = false;
      }
    }
    Serial.print("Auto Mode: ");
    Serial.println(_autoMode);    
  } else if( cmd.equals("profile") ) {  
  } else if( cmd.equals("switch") ) {      
    Serial.print("soon");
  } else {
    Serial.print("ERROR: unknown command: ");
    Serial.println(cmd);
  }
}

void split(String str, String sep, Vector<String>& v) {
  int idx = 0;
  v.clear();
  while( str.indexOf(sep, idx) > 1 ) {
    int newidx = str.indexOf( sep, idx );
    String sub = str.substring(idx, newidx);  
    v.push_back(sub);
    idx = newidx;
  }

  if( idx < str.length() ) {
    v.push_back( str.substring( idx ) );
  }
}

void saveProfiles() 
{
  // write profile count
  byte pc = _profiles.size();
  EEPROM[0] = pc;

  // write all the profiles...
  for( size_t idx = 0; idx < pc; idx++ ) {
    EEPROM.put( (idx*5)+1, _profiles[idx] );
  }
}

void loadProfiles()
{
  _profiles.clear();
  
  //read profile count
  byte profileCount = EEPROM[0];

  // read as many profiles as there are stored...
  for( byte idx = 0; idx < profileCount; idx++ ) {
    profile_pack_t profile;
    EEPROM.get(((idx*5)+1), profile);
    _profiles.push_back(profile);   
  }
}



