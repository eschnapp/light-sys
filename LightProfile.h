#ifndef __LIGHTPROFILE_H__
#define __LIGHTPROFILE_H__
#include <StackList.h>

//
// BUFFER CONTENTS (64bit):
// [ PROGRAM_ID = 6 bits ][ MINUTE = 6 bits][HOUR = 4 bits][DAYOFWEEK = 3 bits][MONTH = 4 bits][DAYOFMONTH = 5 bits][UNIT_STATUS_MASK = 12 bits]
//

typedef union profile_pack {
  struct bits {
  byte  _id;
  unsigned long _millis;
  word _units;  
  } BITS ;
  struct bytes {
    byte _b1;
    byte _b2;
    byte _b3;
    byte _b4;
    byte _b5;
    byte _b6;
    byte _b7;
  } BYTES;
} profile_pack_t;

#endif
