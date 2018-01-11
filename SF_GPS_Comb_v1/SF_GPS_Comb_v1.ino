
#include <WISOL.h>
#include <SoftwareSerial.h>
#include <sigfox_Support.h>

#define USE_SIGFOX 0          // 0 = disable all Sigfox code, 1 = enable Sigfox code
#define TEST_SIGFOX 1          // 0 = normal operation. 1 = the program will pretend it is sending stuff.

#define MOCK_LAT 557822
#define MOCK_LONG 125168
#define MOCK_TIME 1034


SoftwareSerial gpsSerial(A3, A2); // RX, TX

char la[7];
char lo[8];
char ti[7];

#define GPS_INFO_BUFFER_SIZE 128
char GPS_info_char;
char GPS_info_buffer[GPS_INFO_BUFFER_SIZE];
unsigned int received_char = 0;
bool message_started=false;

long prevMillies = 0;       // last time millis() was called.
long currentTime = 0;       // The current time of day in milli seconds.

void setup() {
  //******************************//
  //           Serial             //
  //******************************//
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // Setup the software serial.
  gpsSerial.begin(9600);

  //******************************//
  //           Sigfox             //
  //******************************//
  // Start the sigfox module. (Function loops until module is inittialized!)
  if(USE_SIGFOX){
    init_Sigfox();
  }

  //******************************//
  //           GPS                //
  //******************************//
  delay(2000);
  gpsSerial.println(F("$PUBX,40,RMC,0,0,0,0*47")); //RMC OFF
  delay(100);
  gpsSerial.println(F("$PUBX,40,VTG,0,0,0,0*5E")); //VTG OFF
  delay(100);
  gpsSerial.println(F("$PUBX,40,GGA,0,0,0,0*5A")); //CGA OFF
  delay(100);
  gpsSerial.println(F("$PUBX,40,GSA,0,0,0,0*4E")); //GSA OFF
  delay(100);
  gpsSerial.println(F("$PUBX,40,GSV,0,0,0,0*59")); //GSV OFF
  delay(100);
  gpsSerial.println(F("$PUBX,40,GLL,0,0,0,0*5C")); //GLL OFF
  delay(1000);

  
  gpsSerial.println("End of Setup");
  Serial.println("End of Setup");
}

void loop() { // run over and over

//  byte payloadData[] = {'H','E','L','L','O','W','O','R','L','D',4,2};
  byte payloadData[] = {0,0,0,0,0,0,0,0,0,0,0,0};
  boolean transmitPayloadFlag = false;              // Flag set when transmitting data.
  boolean sfUpdateAllowedTime = false;              // Flag for allowing new Sigfox transmission, based on time since last.
  long lastSfSendTime = 0;                          // Last time (of day) the last Sigfox transmission was sent.
  
  boolean updateGPS = false;
  boolean validGPS = false;
  boolean gpsUpdateAllowedTime = false;       // Flag for allowing new GPS update, based on time since last.
  long lastGPSUpdateTime = 0;                 // Last time (of day) the GPS position was successfully updated.

  long latitude;
  long longtitude;
  long gpsTime;
  char gpsTTemp[2] = {0,0};

  int32_t debugInt = 0;

  long prevMillies = 0;
  long currentTime = 0;       // The current time of day in milli seconds.
  
  //byte tHour = 0;
  //byte tMin = 0;
  //byte tSec = 0;
  

  while(1){
    //******************************//
    //           Serial             //
    //******************************// 
    // Recieving on the software serial and saving.
    /*
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

    // Write full gpsSerial sentence to norm serial
    if(gSBComp){
      Serial.write(gpsSerialBuf);
      gSBComp = false;
      gSB = 0;
    }
    // Write full normSerial sentence to norm serial
    if(nSBComp){
      gpsSerial.write(normSerialBuf);
      nSBComp = false;
      nSB = 0;
    }
    */

    //******************************//
    //           Time               //
    //******************************//
    prevMillies = millis();
    currentTime += (millis()-prevMillies);

    
    //gpsUpdateAllowedTime

    //******************************//
    //            GPS               //
    //******************************//
   
    if(Serial.available() ) {      // && gpsUpdateAllowedTime
      updateGPS = true;
      Serial.read();
    }
    // Updates the GPS postion and time.
    if(updateGPS){
      Serial.println("Fecthing");
      fetch();
      Serial.println("Done Fecthing");
      

      // Converting GPS data to integers
      latitude = atol(la);
      longtitude = atol(lo);
      gpsTime = atol(ti);
      if(latitude > 0 && longtitude > 0 && updateGPS==true ){
        validGPS = true;
        
        // updating time from GPS.
        gpsTTemp[0] = ti[0]; gpsTTemp[1] = ti[1];         // Putting hour chars into temp var.
        lastGPSUpdateTime = (atol(gpsTTemp))*60*60;  // Setting hours.
        gpsTTemp[0] = ti[2]; gpsTTemp[1] = ti[3];         // Putting minute chars into temp var.
        lastGPSUpdateTime += (atol(gpsTTemp))*60;    // Adding minutes.
        gpsTTemp[0] = ti[4]; gpsTTemp[1] = ti[5];         // Putting seconds chars into temp var.
        lastGPSUpdateTime += (atol(gpsTTemp));       // Adding seconds.
        prevMillies = millis();
        currentTime = lastGPSUpdateTime*1000;       // Setting time of day var to the gps time.
        
        Serial.println("Valid GPS found");
        Serial.print("Latitude: ");
        Serial.print(la);
        Serial.print("  ");
        Serial.print(latitude);
        Serial.println();
        
        Serial.print("Longtitude: ");
        Serial.print(lo);
        Serial.print("  ");
        Serial.print(longtitude);
        Serial.println();
        
        Serial.print("Time: ");
        Serial.print(ti);
        Serial.print("  ");
        Serial.print(gpsTime);
        Serial.print("  ");
        Serial.print(lastGPSUpdateTime);
        Serial.println();
      }
      updateGPS = false;
    }
    
    

    //******************************//
    //          SIGFOX              //
    //******************************//
    if(0){
      
    }
    //********************//
    //   Format payload.  //
    //********************//
    // Use test data.
    //format_payload(payloadData, MOCK_LAT, MOCK_LONG, MOCK_TIME);
    //format_payload(payloadData, 0x001234, 0, 0);
    format_payload(payloadData, latitude, longtitude, gpsTime);
    
    //********************//
    // Transmitt payload  //
    //********************//
    // Checks wether a 'SEND' command has been recieved. 
    
    // Sends the payload
    if((USE_SIGFOX || TEST_SIGFOX) && transmitPayloadFlag ){
      gpsSerial.write("TRANSMITTING DATA!\r\n");
      

      if(USE_SIGFOX){
        Send_Pload(payloadData,12);
        lastSfSendTime = currentTime;
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

// Fetches GPS coordinates, and updates global containers, la[6], lo[7], ti[6].
// loops until valid coordinates are recieved.
// Needs Global char arrays: la[6], lo[7], ti[6].
void fetch() {
  //Serial.println("fetching");
  bool newdat=false;
  while(!newdat){
  //If message is not started, send a request for the gps module.
  if (!message_started) {
      gpsSerial.println("$PUBX,00*33");
      }
  
  while (gpsSerial.available()) {
    //when gps is available, define the incomming character
    GPS_info_char = gpsSerial.read();
    //Start message in case of $
    if (GPS_info_char == '$'){
      message_started=true;
      received_char=0;
    }
    //End message in case of *
    else if (GPS_info_char == '*'){
      //Check if commas are placed correctly for the values of latitude, longitude and time.
      if(GPS_info_buffer[7]==','&&GPS_info_buffer[30]==','&&GPS_info_buffer[17]==','){
        //breaking loop and isolating the data in 3 variables.
        newdat=true;
        for (int i = 0; i < 6; i++) {
        if (i < 6) {ti[i] = GPS_info_buffer[8 + i];}
          else {ti[7] = 0;}
        }
        for (int i = 0; i < 7; i++) {
          if (i < 5) {lo[i] = GPS_info_buffer[31 + i];}
          else if (i < 7) {lo[i] = GPS_info_buffer[32 + i];}
          else {lo[8] = 0;}
        }
        for (int i = 0; i < 6; i++) {
          if (i < 4) {la[i] = GPS_info_buffer[18 + i];}
          else if (i < 6) {la[i] = GPS_info_buffer[19 + i];}
          else {la[7] = 0;}
        }
      }
      //Serial.write(GPS_info_buffer);
      //Serial.println();
      message_started=false; // ready for the new message
    }
    //placing incomming characters in buffer
    else if (message_started==true){
      if (received_char<=GPS_INFO_BUFFER_SIZE){
        GPS_info_buffer[received_char]=GPS_info_char;
        received_char++;
      }
      //Resetting after overflow.
      else{
        //Serial.println("overflow");
        message_started=false;
        received_char=0;
      }
    }
  }
  }
  //Serial.println("done fetching");
}


