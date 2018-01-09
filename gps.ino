

#include <SoftwareSerial.h>
#include "tempo.h"

#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

const int colorR = 255;
const int colorG = 255;
const int colorB = 255;

bool first_loop_exec;

char la[7];
char lo[8];
char ti[7];
      
unsigned long time_1=0;
unsigned long time_2=0;

#define GPS_INFO_BUFFER_SIZE 128
char GPS_info_char;
char GPS_info_buffer[GPS_INFO_BUFFER_SIZE];
unsigned int received_char;

bool message_started;

SoftwareSerial gpsSerial(A3, A2); // 7=RX, 8=TX (needed to communicate with GPS)

// REAL TIME SCHEDULER PARAMETERS AND VARIABLES
#define SCHEDULER_TIME 10000 // scheduler interrupt runs every 20000us = 20ms
#define DIVIDER_STD 200 // logging message sent every 100 scheduler times (20ms) 1s
#define DIVIDER_DELAY 500 // delay after forwarding meggages is 3s
unsigned int divider=0;
unsigned int divider_max=DIVIDER_DELAY;


// SENDS THE POLLING MESSAGE TO GPS
void scheduled_interrupt() 
{
  divider++;
  if (divider==divider_max) {
    divider=0;
    divider_max=DIVIDER_STD;
    time_1 = millis();
    gpsSerial.println("$PUBX,00*33"); // data polling to the GPS
  }
}
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  } 
  Serial.println("Connected");
  gpsSerial.begin(57600);
  gpsSerial.println("Connected");
  

  first_loop_exec=true;
  message_started=false;

  Timer1.initialize(); // initialize 10ms scheduler timer
  Timer1.setPeriod(SCHEDULER_TIME); // sets the main scheduler time in microseconds (10ms in this case)
  Timer1.attachInterrupt(scheduled_interrupt); // attaches the interrupt
  Timer1.start(); // starts the timer

  lcd.begin(16, 2);
    
  lcd.setRGB(colorR, colorG, colorB);
  
}
void loop() { // run over and over
  if (first_loop_exec == true){
    lcd.print("Connected");
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
    first_loop_exec = false;
  }
  // MANAGES THE CHARACTERS RECEIVED BY GPS
  while (gpsSerial.available()) {
    GPS_info_char=gpsSerial.read();
    if (GPS_info_char == '$'){ // start of message
      message_started=true;
      received_char=0;
    }
    else if (GPS_info_char == '*'){ // end of message
      for(int i=0;i<6;i++){
        if(i < 6){ti[i]=GPS_info_buffer[8+i];}
        else{ti[7]=0;} 
      }
      for(int i=0;i<7;i++){
        if(i < 5){lo[i]=GPS_info_buffer[31+i];}
        else if(i< 7){lo[i]=GPS_info_buffer[32+i];}
        else{lo[8]=0;} 
      }
      for(int i=0;i<6;i++){
        if(i < 4){la[i]=GPS_info_buffer[18+i];}
        else if(i< 6){la[i]=GPS_info_buffer[19+i];} 
        else{la[7]=0;}
      }
      delay(100);
      Serial.write(ti);
      Serial.println();
      Serial.write(lo);
      Serial.println();
      Serial.write(la);
      Serial.println();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(ti);
      lcd.setCursor(0, 1);
      lcd.print(la);
      lcd.print("  ");
      //lcd.setCursor(0, 8);
      lcd.print(lo);
      
      message_started=false; // ready for the new message
    }else if (message_started==true){ // the message is already started and I got a new character
      if (received_char<=GPS_INFO_BUFFER_SIZE){ // to avoid buffer overflow
        GPS_info_buffer[received_char]=GPS_info_char;
        received_char++;
      }else{ // resets everything (overflow happened)
        message_started=false;
        received_char=0;
      }
    }
  }
  while (Serial.available()) {
    gpsSerial.write(Serial.read());
  }
}
