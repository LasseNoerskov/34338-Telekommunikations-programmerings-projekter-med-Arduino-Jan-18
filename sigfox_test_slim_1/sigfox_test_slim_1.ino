
#include <WISOL.h>
#include <SoftwareSerial.h>

Isigfox *Isigfox = new WISOL();
uint8_t PublicModeSF;
int flagInit = -1;

SoftwareSerial gpsSerial(A3, A2); // RX, TX

void setup() {

  
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Setup the software serial.
  // set the data rate for the SoftwareSerial port
  gpsSerial.begin(57600);


  
  // WISOL test
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



  

  // Some testing of the Sigfox module
  /*
  delay(1000);
  gpsSerial.print("testComs: ");
  gpsSerial.println(Isigfox->testComms());
  delay(1000);
  gpsSerial.print("GetZone: ");
  gpsSerial.println(Isigfox->getZone());
  delay(1000);
  */
  gpsSerial.println("End of Setup");
  
}

void loop() { // run over and over
  int runLoop = 1;

  char normSerialBuf[] = {0,0,0,0,0,0,0,0,0,0};
  int nSB = 0;
  boolean nSBComp = false;
  char gpsSerialBuf[] = {0,0,0,0,0,0,0,0,0,0};
  int gSB = 0;
  boolean gSBComp = false;

  byte transmit[] = {'H','E','L','L','O','W','O','R','L','D',4,2};

  while(1){
      
    
    while(gpsSerial.available()) {
      gpsSerialBuf[gSB] = gpsSerial.read();
      gpsSerial.write(gpsSerialBuf[gSB]);
      if(gpsSerialBuf[gSB] == '\n' || gpsSerialBuf[gSB] == '\r'){
        gSBComp = true;
      }
      gSB++;
    }
    while(Serial.available()) {
      normSerialBuf[nSB] = Serial.read();

      if(normSerialBuf[nSB] == '\n' || normSerialBuf[nSB] == '\r'){
        nSBComp = true;
      }
      nSB++;
    }
    
    if(gSBComp){
      Serial.write(gpsSerialBuf);
      gSBComp = false;
      gSB = 0;
    }
    if(nSBComp){
      gpsSerial.write(normSerialBuf);
      nSBComp = false;
      nSB = 0;
    }

    
    if(gpsSerialBuf[0] == 'S' && gpsSerialBuf[1] == 'E' && gpsSerialBuf[2] == 'N' && gpsSerialBuf[3] == 'D'){
      gpsSerial.write("TRANSMITTING DATA!\r\n");
      gpsSerialBuf[0] = 0;

      Send_Pload(transmit,12);
    }
    
    
  }
}


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
