
#include <WISOL.h>
#include <SoftwareSerial.h>
#include <sigfox_Support.h>

#define USE_SIGFOX 1          // 0 = disable all Sigfox code, 1 = enable Sigfox code
#define TEST_SIGFOX 0          // 0 = normal operation. 1 = the program will pretend it is sending stuff.

#define MOCK_LAT 557822
#define MOCK_LONG 125168
#define MOCK_TIME 1034


SoftwareSerial gpsSerial(A3, A2); // RX, TX

void setup() {

  
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only.
  }

  // Setup the software serial.
  gpsSerial.begin(57600);

  // Start the sigfox module. (Function loops until module is inittialized!)
  if(USE_SIGFOX){
    init_Sigfox();
  }


  

  
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

//  byte payloadData[] = {'H','E','L','L','O','W','O','R','L','D',4,2};
  byte payloadData[] = {0,0,0,0,0,0,0,0,0,0,0,0};

  INT32_U latitude;
  latitude.val = 0;
  INT32_U longtitude;
  longtitude.val = 0;
  INT16_U gpsTime;
  gpsTime.val = 0;

  int32_t debugInt = 0;
  

  while(1){
      
    // Recieving on the software serial and saving.
    while(gpsSerial.available()) {
      gpsSerialBuf[gSB] = gpsSerial.read();
      gpsSerial.write(gpsSerialBuf[gSB]);
      if(gpsSerialBuf[gSB] == '\n' || gpsSerialBuf[gSB] == '\r'){
        gSBComp = true;
      }
      gSB++;
    }
    // Recieving on the normal serial
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


    //********************//
    //   Format payload.  //
    //********************//
    // Use test data.
    

    format_payload(payloadData, MOCK_LAT, MOCK_LONG, MOCK_TIME);
    //format_payload(payloadData, 0x001234, 0, 0);
    
    
    //********************//
    // Transmitt payload  //
    //********************//
    // 
    if((USE_SIGFOX || TEST_SIGFOX) && gpsSerialBuf[0] == 'S' && gpsSerialBuf[1] == 'E' && gpsSerialBuf[2] == 'N' && gpsSerialBuf[3] == 'D'){
      gpsSerial.write("TRANSMITTING DATA!\r\n");
      gpsSerialBuf[0] = 0;

      if(USE_SIGFOX){
        Send_Pload(payloadData,12);
      }
      if(TEST_SIGFOX){
        gpsSerial.println("\nPAYLOAD: ");
        gpsSerial.print("Data [HEX]: ");
        for(int i=0; i<12;i++){
          gpsSerial.print(payloadData[i],HEX);
          gpsSerial.print(' ');
        }
        gpsSerial.print("\n\r");
        gpsSerial.print("LAT: ");
        debugInt = (((long)payloadData[0])<<16) + (((long)payloadData[1])<<8) + payloadData[2];
        gpsSerial.print(debugInt); gpsSerial.print("  ");
        gpsSerial.print("LONG: ");
        debugInt = (((long)payloadData[3])<<16) + (((long)payloadData[4])<<8) + payloadData[5];
        gpsSerial.print(debugInt); gpsSerial.print("  ");
        gpsSerial.print("TIME: ");
        debugInt = (((long)payloadData[6])<<8) + payloadData[7];
        gpsSerial.print(debugInt); gpsSerial.print("  ");

        gpsSerial.print("\n\r");
      }
    }
    
    
  }
}





