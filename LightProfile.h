#ifndef __LIGHTPROFILE_H__
#define __LIGHTPROFILE_H__
#include <StackList.h>

//
// BUFFER CONTENTS (64bit):
// [ PROGRAM_ID = 6 bits ][ MINUTE = 6 bits][HOUR = 4 bits][DAYOFWEEK = 3 bits][MONTH = 4 bits][DAYOFMONTH = 5 bits][UNIT_STATUS_MASK = 12 bits]
//

typedef union profile_pack {
  struct bits {
  unsigned int _id:6;
  unsigned int _minute:6;
  unsigned int _hour:4;
  unsigned int _dow:3;
  unsigned int _month:4;
  unsigned int _dom:5;
  unsigned long _units: 12;  
  } BITS ;
  struct bytes {
    byte _b1;
    byte _b2;
    byte _b3;
    byte _b4;
    byte _b5;
  } BYTES;
} profile_pack_t;

#endif
