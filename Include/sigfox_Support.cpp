#include "Arduino.h"
#include <WISOL.h>
#include "sigfox_Support.h"

Isigfox *Isigfox = new WISOL();
uint8_t PublicModeSF;
int flagInit = -1;


// takes lat, long and time, and formats it into the 12byte data array.
void format_payload(byte *data, long lat, long lng, int tim){

  INT32_U latitude;
  latitude.val = 0;
  INT32_U longtitude;
  longtitude.val = 0;
  INT16_U gpsTime;
  gpsTime.val = 0;
  
  latitude.val = lat;
  longtitude.val = lng;
  gpsTime.val = tim;
  
  // reset payload.
  for(int i=0; i<12; i++){
    data[i] = 0;
  }
  
  // Assign data to payload
  // Latitude:
  data[0] = latitude.bytes[2];
  data[1] = latitude.bytes[1];
  data[2]= latitude.bytes[0];
  // Longtitude:
  data[3] = longtitude.bytes[2];
  data[4] = longtitude.bytes[1];
  data[5]= longtitude.bytes[0];
  // Time:
  data[6] = gpsTime.bytes[1];
  data[7] = gpsTime.bytes[0];
  
}

// Send payload function,
// Taken from the Xkit Demo program
void Send_Pload(uint8_t *sendData, const uint8_t len){
  // No downlink message require
  recvMsg *RecvMsg;

  
  RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
  Isigfox->sendPayload(sendData, len, 0, RecvMsg);
  for (int i = 0; i < RecvMsg->len; i++) {
    Serial.print(RecvMsg->inData[i]);
  }
  Serial.println("");
  free(RecvMsg);
  
  /*
  gpsSerial.println();
  for(int i = 0; i<12; i++){
    gpsSerial.print(sendData[i]);
    gpsSerial.print(' ');
  }
  gpsSerial.println();
  */
  // If want to get blocking downlink message, use the folling block instead
  /*
  recvMsg *RecvMsg;

  RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
  Isigfox->sendPayload(sendData, len, 1, RecvMsg);
  for (int i=0; i<RecvMsg->len; i++){
    Serial.print(RecvMsg->inData[i]);
  }
  Serial.println("");
  free(RecvMsg);
  */

  // If want to get non-blocking downlink message, use the folling block instead
  /*
  Isigfox->sendPayload(sendData, len, 1);
  timer.setTimeout(46000, getDLMsg);
  */
}


// Gets the device ID
// Taken from the Xkit demo program.
void GetDeviceID(){
  recvMsg *RecvMsg;
  const char msg[] = "AT$I=10";

  RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
  Isigfox->sendMessage(msg, 7, RecvMsg);

  Serial.print("Device ID: ");
  for (int i=0; i<RecvMsg->len; i++){
    Serial.print(RecvMsg->inData[i]);
  }
  Serial.println("");
  free(RecvMsg);
}


// Inits the Sigfox module.
// This function inits the sigfox module, test it ans returns the region and device ID.
// WARNING! This function loops until the Sigfox module is initialized.
void init_Sigfox(){
	// WISOL test ( taken from the Xkit Demo program).
    while (flagInit == -1) {
      Serial.println(""); // Make a clean restart
      delay(1000);
      PublicModeSF = 0;
      flagInit = Isigfox->initSigfox();
      Isigfox->testComms();
      GetDeviceID();
      //Isigfox->setPublicKey(); // set public key for usage with SNEK
    }
    Serial.println(""); // Make a clean start
	
}