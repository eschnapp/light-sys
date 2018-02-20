#include <ccspi.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LightModule.h"
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
#define CPU_TEMP_SENSOR_MODULE_ID 100
#define PSU_TEMP_SENSOR_MODULE_ID 200


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
SensorAddress* _tempAddresses[20];
int _tempSensorCount = 0;
int _debugLevel = 0;
bool _sdInit = false;
SensorAddress* _cpuTempSensor = NULL;
float _cpuTemp = 0;
SensorAddress* _psuTempSensor = NULL;
float _psuTemp = 0;
LightModule* _allChannels[MAX_CHANNELS];
static const char numChars = 32;
char receivedChars[numChars];
char tmpBuf[numChars];
bool newData = false;

//  Global Map for mapping channel pin IDs by pin type.  
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


//void printAddress(uint8_t* deviceAddress)
//{
//  Serial.print((const char *)F("["));
//  Serial.print((int)deviceAddress, HEX);
//  Serial.print((const char *)F("] "));
//  for (uint8_t i = 0; i < 8; i++)
//  {
//    // zero pad the address if necessary
//    if (deviceAddress[i] < 16) Serial.print((const char *)F("0"));
//    Serial.print(deviceAddress[i], HEX);
//  }
//}

//void printProfiles()
//{
//  Serial.println((const char *)F("==================< START ACTIVE PROFILE INFORMATION >===================="));
//  for( size_t idx = 0; idx < _profiles.size(); idx++ ){
//    Serial.print((const char *)F("PROFILE_ID: "));
//    Serial.print(idx);
//    Serial.print((const char *)F(" MILLIS: "));
//    Serial.print(_profiles[idx].BITS._millis);
//    Serial.print((const char *)F(" UNITS_MASK "));
//    Serial.print(_profiles[idx].BITS._units, BIN);
//    Serial.print((const char *)F(" LAST CHECK "));
//    Serial.println( _lastProfileCheck[idx] );
//  }
//  Serial.println((const char *)F("==================< END ACTIVE PROFILE INFORMATION >===================="));
//}

void recvWithStartEndMarkers() {
    static bool recvInProgress = false;
    static char ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

bool fetchCommand(char* command_buf, int command_sz, char* value_buf, int value_sz) {
    if (newData == true) {
        strncpy(tmpBuf, receivedChars, numChars);
        char * strtokIndx; // this is used by strtok() as an index
        strtokIndx = strtok(tmpBuf,",");      // get the first part - the string
        if( strtokIndx == NULL )
        {
          strncpy(command_buf, tmpBuf, command_sz); // copy it to messageFromPC
          memset(value_buf, 0, value_sz);
          return true;
        }
        else
        {
          strncpy(command_buf, strtokIndx, command_sz); // copy it to messageFromPC
        }
 
        strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
        strncpy(value_buf, strtokIndx, value_sz); // copy it to messageFromPC       
        newData = false;
        return true;
    }
    return false;
}

void saveSensorFile( const DeviceAddress& address, String uid ) {
  if( _sdInit == true ) {
    Serial.println((const char *)F("Writing temp sensor file to SD card..."));
    String handle((const char *)F("/SENSORS/"));
    handle.concat(uid);
    handle.toUpperCase();
    File f = SD.open(handle, FILE_WRITE);
    if(f) {
      if( f.seek(0) ) {
        for( int idx = 0; idx < sizeof( address ); idx ++ ) {
          f.write( (char)address[idx]);
        }
        f.close(); 
        Serial.print((const char *)F("Sensor calibration saved to sensor file "));
        Serial.println( handle );
      } else {
        Serial.print((const char *)F("WARN: Failed to seek sensor file to start position: "));
        Serial.println( handle );        
      }
    } else {
      Serial.print((const char *)F("WARN: Failed to open sensor file for writing: "));
      Serial.println( handle );
    }
  } else {
    Serial.print((const char *)F("Serial is not initialized, cannot write sensor address file "));
    Serial.println(uid);  
  }
}
void calibrateTempSensor(int id) {
  Serial.print((const char *)F("Aquiring temp sensor for channel id "));
  Serial.println(id);
  Serial.print((const char *)F("Please rub the sensor in your hand for 30 seconds..."));
  delay(30000); // wait 30 seconds...
  Serial.print((const char *)F("Requesting temp from all sensors..."));
  sensors.requestTemperatures();
  float maxC = 0;
  int maxCidx = 0;
  for( int idx = 0; idx < _tempSensorCount; idx++ ) {
    uint8_t* add = _tempAddresses[idx]->_address;
    Serial.print((const char *)F("Getting temp for sensor "));
    Serial.print(_tempAddresses[idx]->toString());
    float c = sensors.getTempC(add);
    Serial.print((const char *)F(":  "));
    Serial.println(c);
    if( c > maxC ) {
      maxC = c;
      maxCidx = idx;
    }
  }
  
  Serial.print((const char *)F("Detected temp sensor address "));
  Serial.print( _tempAddresses[maxCidx]->toString());
  Serial.print((const char *)F(" With Temp reading: "));
  Serial.println(maxC);
  Serial.print((const char *)F("Attaching address to module "));
  Serial.println(id);
  
  _allChannels[id]->set_sensor_address(_tempAddresses[maxCidx]);
  saveSensorFile( _tempAddresses[maxCidx]->_address, String(id, DEC) );
}

void assignTempSensor(int moduleId_, const char* sensorAddress_ ) {

  Serial.print((const char *)F("Trying to locate sensor with address "));
  Serial.println(sensorAddress_);

  for( int idx = 0; idx < _tempSensorCount; idx++ )
  {
    if( strncmp(sensorAddress_, _tempAddresses[idx]->_address, sizeof(sensorAddress_)) == 0 )
    {
      Serial.print((const char *)F("Saving sensor ["));
      Serial.print(_tempAddresses[idx]->toString());
      Serial.print((const char *)F("] to module ID "));
      Serial.println(moduleId_);

      if( moduleId_ > MAX_CHANNELS )
      {
        if( moduleId_ == 100 )
        {
          Serial.print((const char *)F("Setting CPU temp sensor address: "));
          Serial.println(sensorAddress_);
          _cpuTempSensor = _tempAddresses[idx];
          saveSensorFile( _tempAddresses[idx]->_address, F("CPU") ); 
        }
        else if( moduleId_ == 200 )
        {
          Serial.print((const char *)F("Setting PSU temp sensor address: "));
          Serial.println(sensorAddress_);
          _cpuTempSensor = _tempAddresses[idx];
          saveSensorFile( _tempAddresses[idx]->_address, F("PSU") );           
        }
        else
        {
          Serial.println((const char *)F("Failed to locate the module for this module ID"));
        }
      }
      else 
      {
        _allChannels[moduleId_]->set_sensor_address(_tempAddresses[idx]);
        saveSensorFile( _tempAddresses[idx]->_address, String(moduleId_, DEC) );        
      }
      return;
    }
  }

  Serial.print((const char *)F("Failed to locate the sensor with the given address "));
  Serial.println(sensorAddress_);
}

void displayTemps() {
  Serial.println((const char *)F("[ID]---[ADDRESS]--------------[TEMP]"));
  Serial.println((const char *)F("===================================="));
  for( int idx = 0; idx < _tempSensorCount; idx++ ) {
    if( _tempAddresses[idx] != 0 ) {
      if( sensors.requestTemperaturesByAddress(_tempAddresses[idx]->_address)) {
        float tmpC = sensors.getTempC(_tempAddresses[idx]->_address);
        Serial.print(idx);
        Serial.print((const char *)F("       "));
        Serial.print(_tempAddresses[idx]->toString());
        Serial.print((const char *)F("   "));
        Serial.println( tmpC );
      }
    }
  }  
}

void showHelp() {
  Serial.println((const char *)F("Available Commands"));
  Serial.println((const char *)F("====================================================="));
  Serial.println((const char *)F("help      - print this help message"));
  Serial.println((const char *)F("status    - print current status of unit objects"));
  //Serial.println((const char *)F("temp      - temp show/calibrate"));
  //Serial.println((const char *)F("auto      - switch between manual and auto modes"));
  //Serial.println((const char *)F("profiles  - switch between manual and auto modes"));
  //Serial.println((const char *)F("switch    - switch UNIT/LIGHT/FAN OFF or ON"));
}

void processConsoleInput() {

    // fetch the serial data if any to process...
    char command[10];
    char value [20];
    if( !fetchCommand( command, sizeof(command), value, sizeof(value))) {
       // nothing to read from console... move on...
    } else {
        Serial.print((const char *)F("Received command: ["));
        Serial.print(command);
        Serial.print((const char *)F("]-["));
        Serial.print(value);
        Serial.println((const char *)F("]"));

        if( strncmp(command, "help", strlen(command)) == 0 )  
        {
          showHelp(); 
        } 
        else if( strncmp(command, "status", strlen(command)) == 0 )
        {
          showStatus();
        }
        else if( strncmp(command, "tmpshow", strlen(command)) == 0 ) 
        {
          displayTemps();
        }
        else if( strncmp(command, "tmpcal", strlen(command)) == 0 )
        {
          int sensorId = atoi(value);
          calibrateTempSensor(sensorId);
        }        
        else if( strncmp(command, "tmpset", strlen(command)) == 0 )
        {
          char tempBuf[20];
          strncpy(tempBuf, value, strlen(value));
          char * strtokIndx; // this is used by strtok() as an index
          strtokIndx = strtok(tempBuf,",");      // get the first part - the string
          if( strtokIndx == NULL )
          {
            Serial.println((const char *)F("tmpset must be formatted as <tmpset,[Module_ID],[SensorAddress]>"));
          }
          else
          {
            int moduleId = atoi(strtokIndx);
            char address[10];
            strtokIndx = strtok(NULL, ",");
            strncpy(address, strtokIndx, 10); // copy it to messageFromPC
            Serial.print((const char *)F("Setting temp sensor with address ["));
            Serial.print(address);
            Serial.print("] to module ID ");
            Serial.println(moduleId);
            assignTempSensor(moduleId, address);
          }          
        }        
        else if( strncmp(command, "uon", strlen(command)) == 0 )
        {
          if( strncmp(value, "all", sizeof(value) == 0 ))
          {
            Serial.print((const char *)F("Switching ALL units ON"));
            for( int idx = 0; idx < MAX_CHANNELS; idx++ )
            {
              _allChannels[idx]->switchUnit(false);
            }   
          }
          else
          {
            int moduleId = atoi(value);
            Serial.print((const char *)F("Switching Module "));
            Serial.print(moduleId);
            Serial.println((const char *)F(" ON"));
            _allChannels[moduleId]->switchUnit(false);
          }
        }   
        else if( strncmp(command, "uoff", strlen(command)) == 0 )
        {
          if( strncmp(value, "all", sizeof(value) == 0 ))
          {
            Serial.print((const char *)F("Switching ALL units OFF"));
            for( int idx = 0; idx < MAX_CHANNELS; idx++ )
            {
              _allChannels[idx]->switchUnit(true);
            }   
          }
          else
          {
            int moduleId = atoi(value);
            Serial.print((const char *)F("Switching Module "));
            Serial.print(moduleId);
            Serial.println((const char *)F(" OFF"));
            _allChannels[moduleId]->switchUnit(true);
          }
        }               
        else if( strncmp(command, "fanon", strlen(command)) == 0 )
        {
          if( strncmp(value, "all", sizeof(value) == 0 ))
          {
            Serial.print((const char *)F("Switching ALL fans ON"));
            for( int idx = 0; idx < MAX_CHANNELS; idx++ )
            {
              _allChannels[idx]->switchFan(false);
            }   
          }
          else
          {
            int moduleId = atoi(value);
            Serial.print((const char *)F("Switching Fan "));
            Serial.print(moduleId);
            Serial.println((const char *)F(" ON"));
            _allChannels[moduleId]->switchFan(false);
          }          
        }      
        else if( strncmp(command, "fanoff", strlen(command)) == 0 )
        {
          if( strncmp(value, "all", sizeof(value) == 0 ))
          {
            Serial.print((const char *)F("Switching ALL fans OFF!!!"));
            for( int idx = 0; idx < MAX_CHANNELS; idx++ )
            {
              _allChannels[idx]->switchFan(true);
            }   
          }
          else
          {
            int moduleId = atoi(value);
            Serial.print((const char *)F("Switching Fan "));
            Serial.print(moduleId);
            Serial.println((const char *)F(" OFF!!"));
            _allChannels[moduleId]->switchFan(true);
          }          
        }            
        else if( strncmp(command, "reset", strlen(command)) == 0 )
        {
          Serial.print((const char *)F("Reset in 5 seconds!"));
          delay(5000);
          asm ("  jmp 0; ");   
        }        
        else
        {
          Serial.println("Unknown commmand!");
        }
        
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
      Serial.print((const char *)F("\t\t"));
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void showStatus() {
    Serial.println((const char *)F("==================< STATUS BEGIN >===================="));
    unsigned long tm = millis();
    Serial.print((const char *)F("Uptime: ")); 
    Serial.print((unsigned long) (tm / (1000L*60*60*24*7))); Serial.print((const char *)F(" weeks "));
    Serial.print((unsigned long) ((tm / (1000L*60*60*24)) % 7)); Serial.print((const char *)F(" days "));
    Serial.print((unsigned long) ((tm / (1000L*60*60)) % 24)); Serial.print((const char *)F(" hours "));
    Serial.print((unsigned long) ((tm / (1000L*60)) % 60)); Serial.print((const char *)F(" minutes "));
    Serial.print((unsigned long) (tm / 1000L) % 60) ; Serial.print((const char *)F(" seconds ")); 
    Serial.println((const char *)F("."));
    if( _cpuTempSensor != NULL ) {
      Serial.print((const char *)F("CPU Temp Sensor: "));
      Serial.println(_cpuTemp);
    }
    if( _psuTempSensor != NULL ) {
      Serial.print((const char *)F("PSU Temp Sensor: "));
      Serial.println(_psuTemp);
    }    
    
    Serial.println((const char *)F("ID  CONNTECTED UNIT FAN     TEMP     SENSOR ADDRESS"));
    for( int idx = 0; idx < MAX_CHANNELS; idx++ ){
          Serial.print(idx);
          Serial.print((const char *)F("     "));
          Serial.print( _allChannels[idx]->is_connected() );
          Serial.print((const char *)F("         "));
          Serial.print( _allChannels[idx]->is_unit_on() );
          Serial.print((const char *)F("     ")); 
          Serial.print( _allChannels[idx]->is_fan_on());
          Serial.print((const char *)F("     ")); 
          Serial.print( _allChannels[idx]->getUnitTemp());
          Serial.print((const char *)F("  "));
          Serial.print(_allChannels[idx]->get_sensor_address() ? _allChannels[idx]->get_sensor_address()->toString() : F("NONE") );
          Serial.println((const char *)F(""));        
    }
    Serial.println((const char *)F("==================< STATUS END >===================="));  
}

void setup() {
 
  Serial.begin(9600);
  //Serial.setTimeout(100);
  Serial.print((const char *)F("Light Controller V 2.0 Startup Sequence Starting...."));
  Serial.print((const char *)F("========[ SETUP SEQUENCE START ]============"));

  // initialize the SD card library...
  if( !SD.begin(SDCARD_CS_PIN) ) {
    Serial.println((const char *)F("Couldnt initialize the SD card library!"));
  } else {
    Serial.print((const char *)F("SD Card file index: "));
    File root = SD.open((const char *)F("/"));
    printDirectory(root, 0);
    root.close();
    if( !SD.exists((const char *)F("/SENSORS/"))) {
      Serial.print((const char *)F("No sensors folder found - creating new one..."));
      if( !SD.mkdir((const char *)F("/SENSORS/"))) {
        Serial.print((const char *)F("Failed to create sensors folder in SD card!!"));
      }
    }
    _sdInit = true;
  }
  delay(5000);
  
  // setup the temp sensor array...
  sensors.begin();
  int devices = sensors.getDeviceCount();
  if( devices > 0 ) {
    _tempSensorCount = devices;
    Serial.print((const char *)F("Detecting Temperature Sensors... count: "));
    Serial.println(devices);
    for( int idx = 0; idx < devices; idx++ ){
      //uint8_t * add = new uint8_t(8);
      SensorAddress*  add = new SensorAddress();
      if( ! sensors.getAddress(add->_address, idx) ) {
        Serial.println((const char *)F("Failed to obtain address for device index: "));
        Serial.println(idx);
        continue;
      }
      Serial.print((const char *)F("Found TEMP Device: "));
      Serial.println(add->toString());
      _tempAddresses[idx] = add;
    }
  }

  delay(5000);
  // crete the unit modules...
  Serial.print((const char *)F("Configured MAX Units: "));
  Serial.println(MAX_CHANNELS);
  char ccnt = 0;
  for( char idx = 0; idx < MAX_CHANNELS; idx++ )
  {
    Serial.print((const char *)F("Creating Module for Unit "));
    Serial.println((int)idx);
    LightModule* module = new LightModule ( idx
                                          , _globalChannelMap[idx][PIN_OUT_FAN_CONTROL]
                                          , _globalChannelMap[idx][PIN_OUT_LIGHT_CONTROL]
                                          , _globalChannelMap[idx][PIN_IN_CONNECTION_IND] );
    
    Serial.print((const char *)F("Initializing Module ID: "));
    Serial.println(((int) idx));
    module->initializeModule();
    if( !module->is_initialized() ) {
      Serial.println((const char *)F("Failed to initialize Module" ));
      delete module;
      continue;
    }

    if( !module->is_connected() )
    {
      Serial.print((const char *)F("Unit is NOT connected! You can still connect the Unit later... ID: "));
      Serial.println((int)idx);
    }
    else 
    {
      ccnt++;
      Serial.print((const char *)F("Unit is connected, ensuring its OFF..."));
      module->switchUnit(false); // ensure unit is OFF
    }
    Serial.print((const char *)F("Module is initalized, ID: "));
    Serial.println((int)idx);
    _allChannels[idx] = module;
  }
  Serial.print((const char *)F("Total number of initialized modules: "));
  Serial.println(MAX_CHANNELS);
  
  Serial.print((const char *)F("Total number of conected modules: "));
  Serial.println((int) ccnt);

  if( _sdInit == true ) {
    Serial.print((const char *)F("Searching for TEMP sensor config files"));
    File folder = SD.open((const char *)F("/SENSORS/"));
    
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
        Serial.print((const char *)F("Sensor address read for file " ));
        Serial.print(nm);
        Serial.print((const char *)F(" is: "));
        Serial.println(a.toString());

        int fidx = -1;
        for( int idx = 0; idx < _tempSensorCount; idx++ ) {
          if( _tempAddresses[idx]->equals(a) ) {
            fidx = idx;
          }
        }
        if( fidx >= 0 ) {
          Serial.print((const char *)F("Located sensor instance... saving calibration to unit "));
          Serial.println(nm);
          
          nm.toUpperCase();
          if( nm.equals((const char *)F("CPU"))) {
            _cpuTempSensor = _tempAddresses[fidx];
          } else if( nm.equals((const char *)F("PSU"))) {
            _psuTempSensor = _tempAddresses[fidx];
          } else {
            _allChannels[nm.toInt()]->set_sensor_address(_tempAddresses[fidx]);
          }
        }
    }
  }
  
  Serial.print((const char *)F("Light Controller Startup Sequence Complete!"));
  Serial.print((const char *)F("========[ SETUP SEQUENCE END ]============"));  
}

int _loopCounter = 0;
void loop() {

  if( !_sdInit )
  {
    Serial.print((const char *)F("SD Is not initialized... not executing the loop and waiting 1 seconds..."));
    delay(1000);
    return;
  }
  
  // perform profile logic here....
  unsigned long currentT = millis();

  // read/write each module...
  for( size_t idx = 0; idx < MAX_CHANNELS; idx++ )
  {
    _allChannels[idx]->io();
  }

  // read cpu temp sensor if attached
  if( _cpuTempSensor != NULL ) {
    if( sensors.requestTemperaturesByAddress( _cpuTempSensor->_address ) ) {
      _cpuTemp = sensors.getTempC(_cpuTempSensor->_address);
    }    
  }

  // read psu temp sensor if attached
  if( _psuTempSensor != NULL ) {
    if( sensors.requestTemperaturesByAddress( _psuTempSensor->_address ) ) {
      _psuTemp = sensors.getTempC(_psuTempSensor->_address);
    }    
  }  
  
  // check for console commands...
  processConsoleInput();

  if( _loopCounter++ >= 20 ) {
    showStatus();
    _loopCounter = 0;
  }
}
