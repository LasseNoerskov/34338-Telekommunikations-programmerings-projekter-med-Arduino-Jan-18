/***********************************************************
*	sigfox_Support.h
*	Contains serveral support functions for the Sigfox module
*
*
***********************************************************/



#ifndef sigfox_support_h
#define sigfox_support_h

// unions to make it wasy to move vars into payload.
typedef union{
   int32_t val;
   byte bytes[4];
  
}INT32_U;
typedef union{
   int16_t val;
   byte bytes[2];
  
}INT16_U;


void format_payload(byte *data, long lat, long lng, int tim);

void Send_Pload(uint8_t *sendData, const uint8_t len);
void GetDeviceID();
void init_Sigfox();

#endif