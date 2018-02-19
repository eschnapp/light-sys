#include "console.h"


void ConsoleManager::recvWithStartEndMarkers() {
    static bool recvInProgress = false;
    static char ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (_serial.available() > 0 && newData == false) {
        rc = _serial.read();

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

bool 
ConsoleManager::fetchCommand(char* command_buf, size_t command_sz, char* value_buf, size_t value_sz) {
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
