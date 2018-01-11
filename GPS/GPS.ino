
#include <SoftwareSerial.h>
#include <Wire.h>

char la[7];
char lo[8];
char ti[7];

#define GPS_INFO_BUFFER_SIZE 128
char GPS_info_char;
char GPS_info_buffer[GPS_INFO_BUFFER_SIZE];
unsigned int received_char;

bool message_started=false;
 
SoftwareSerial gpsSerial(A3, A2); // 7=RX, 8=TX (needed to communicate with GPS)
 
void setup() {
  /////////////defining baud rates for Serial communication//////////////
  Serial.begin(9600);
  Serial.println("Connected");
  gpsSerial.begin(9600);
  gpsSerial.println("Connected");
  
   ////////////GPS INIALISATION///////////////////
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

}

void fetch() {
  Serial.println("fetching");
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
      Serial.write(GPS_info_buffer);
      Serial.println();
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
        Serial.println("overflow");
        message_started=false;
        received_char=0;
      }
    }
  }
  }
  Serial.println("done fetching");
}


void loop() {
 
  while (Serial.available()) {
    fetch();
    Serial.read();
  }
  
}
