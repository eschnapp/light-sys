#include "HTTPServer.h"


HTTPServer::HTTPServer( int cs3000_CSPIN
                      , int sdcard_CSPIN
                      , int cs3000_IRQPIN
                      , int cs3000_VBATPIN
                      , int listenPort_
                      , String wifiSSID_
                      , String wifiPWD_
                      , int wifiType_
                      , CallbackFunc cb_)
            : _wifi_cs_pin(cs3000_CSPIN)
            , _sd_cs_pin(sdcard_CSPIN)
            , _irq_pin(cs3000_IRQPIN)
            , _vbat_pin(cs3000_VBATPIN)
            , _listenPort(listenPort_)
            , _wifi_ssid( wifiSSID_ )
            , _wifi_pwd(wifiPWD_)
            , _wifi_type(wifiType_)
            , _cb(cb_)
            , cc3000( cs3000_CSPIN, cs3000_IRQPIN, cs3000_VBATPIN, SPI_CLOCK_DIVIDER)
            , httpServer(listenPort_)
{
}


bool
HTTPServer::initialize()
{
  Serial.println(F("Initializing CC3000 HTTP Server!\n")); 
  Serial.print(F("Free RAM: ")); 
  Serial.println(getFreeRam(), DEC);
  
  // Initialise the module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    return false;
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(_wifi_ssid);
  if (!cc3000.connectToAP(_wifi_ssid.c_str(), _wifi_pwd.c_str(), _wifi_type)) {
    Serial.println(F("Failed!"));
    return false;
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  // Display the IP address DNS, Gateway, etc.
  while (! displayConnectionDetails()) {
    delay(1000);
  }

//  // ******************************************************
//  // You can safely remove this to save some flash memory!
//  // ******************************************************
//  Serial.println(F("\r\nNOTE: This sketch may cause problems with other sketches"));
//  Serial.println(F("since the .disconnect() function is never called, so the"));
//  Serial.println(F("AP may refuse connection requests from the CC3000 until a"));
//  Serial.println(F("timeout period passes.  This is normal behaviour since"));
//  Serial.println(F("there isn't an obvious moment to disconnect with a server.\r\n"));
  
  // Start listening for connections
  httpServer.begin();
  Serial.println(F("Listening for connections..."));
  _initialized = true; 
  return true;
}


void 
HTTPServer::process()
{
  if( !_initialized ) {
    Serial.println(F("WARN: HTTPServer is not initialized! not processing any clients..."));
    return;
  }
  
  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    
    // Clear action and path strings.
    memset(&action, 0, sizeof(action));
    memset(&path,   0, sizeof(path));

    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, action, path);
    }

    // Handle the request if it was parsed.
    if (parsed) {
      Serial.println(F("Processing request"));
      Serial.print(F("Raw Buffer: "));
      for( int idx = 0; idx < bufindex; idx++ )
        Serial.print((char)buffer[idx]);
      Serial.println(F(""));
      Serial.print(F("Action: ")); Serial.println(action);
      Serial.print(F("Path: ")); Serial.println(path);
      // Check the action to see if it was a GET request.
      if (strcmp(action, "GET") == 0) {
        String msgBody = "";
        // first try to find the resource in the SD card...
        String hdl(path);
        hdl.toUpperCase();
        if( hdl.startsWith("/RUN?") ) {
            Serial.println(F("Processing command..."));
            if(_cb == NULL){
              Serial.println(F("No callback func!"));
              client.fastrprintln(F("HTTP/1.1 401 Unauthorized"));
              client.fastrprintln(path);
              client.fastrprintln(F(""));
              client.fastrprintln(F("GO AWAY SKRIPT KIDDY!"));                
            } else {
              bool res = (*_cb)(hdl.substring(hdl.indexOf("?") + 1), client);
              if(!res) {
                Serial.println(F("returned False!"));
                client.fastrprintln(F("HTTP/1.1 401 Unauthorized"));
                client.fastrprintln(path);
                client.fastrprintln(F("")); 
                client.fastrprintln(F("YEAH RIGHT... FUCK OFF!"));
              }       
            }  
        } else { 
          int ii = hdl.indexOf("?");
          if( ii > -1 ) {
            // remove all request content...
            hdl = hdl.substring(0, ii);  
          }
          
          if( !SD.exists(hdl) ) {
            Serial.print(F("Resource not found in SD: "));
            Serial.println(hdl);
            client.fastrprintln(F("HTTP/1.1 404 Not Found"));
            client.fastrprintln(path);
            client.fastrprintln(F(""));
            client.fastrprintln(F("NOTHING HERE... GO AWAY!"));
          } else { 
            Serial.println(F("Opening file for read..."));
            File file = SD.open(hdl);
            if( !file ) {
              client.fastrprintln(F("HTTP/1.1 500 Internal Server Error"));
              client.fastrprintln(F(""));            
            } else {
              Serial.println(F("Sending Response Header..."));
              // Respond with the path that was accessed.
              // First send the success response code.
              client.fastrprintln(F("HTTP/1.1 200 OK"));
              // Then send a few headers to identify the type of data returned and that
              // the connection will not be held open.
              //client.fastrprintln(F("Content-Type: text/html"));
              client.fastrprintln(F("Connection: close"));
              client.fastrprintln(F("Server: Adafruit CC3000"));
              // Send an empty line to signal start of body.
              client.fastrprintln(F(""));

              unsigned long sz = file.size();
              Serial.print(F("Streaming file from SD, Size: "));
              Serial.println(sz);
              char buf[200];
              memset(&buf, 0x00, 200); 
              int pos = 0;
              for(  pos = 0; sz > 0; sz -- ) {
                byte bt = file.read();
                if(bt == -1) break;
                buf[pos] = bt;
                pos += 1;
                if( pos > 199 ) {
                  pos = 0;  
                  // flush...
                  //Serial.println(F("Writing Chunk..."));
                  //Serial.print(".");
                  //delay(100);
                  client.write(buf, 200); 
                  client.flush();
                  memset(&buf, 0x00, 200);
                              
                }
                //Serial.println(F("Abount to read..."));
              }
              Serial.println("");

              if( pos > 0 ) {
                client.write(buf, pos);
                client.flush();
              }
              
              client.fastrprintln(F(""));
              file.close();
              Serial.println(F("Done processing resource file!"));
              // Now send the response data.
              //client.fastrprintln(F("Hello world! this is arduino talking yomama!"));
              //client.fastrprint(F("You accessed path: ")); client.fastrprintln(path);
            }
          }
        }
      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(100);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
  }  
}

// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool 
HTTPServer::parseRequest(uint8_t* buf, int bufSize, char* action, char* path) 
{
  // Check if the request ends with \r\n to signal end of first line.
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    parseFirstLine((char*)buf, action, path);
    return true;
  }
  return false;
}

// Parse the action and path from the first line of an HTTP request.
void 
HTTPServer::parseFirstLine(char* line, char* action, char* path) 
{
  // Parse first word up to whitespace as action.
  char* lineaction = strtok(line, " ");
  if (lineaction != NULL)
    strncpy(action, lineaction, MAX_ACTION);
  // Parse second word up to whitespace as path.
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL)
    strncpy(path, linepath, MAX_PATH);
}

// Tries to read the IP address and other connection details
bool 
HTTPServer::displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}


