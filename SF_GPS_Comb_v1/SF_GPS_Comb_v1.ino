#include <WISOL.h>
#include <SoftwareSerial.h>
#include <sigfox_Support.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define USE_SIGFOX 1          // 0 = disable all Sigfox code, 1 = enable Sigfox code
#define TEST_SIGFOX 0          // 0 = normal operation. 1 = the program will pretend it is sending stuff.

#define MOCK_LAT 557822
#define MOCK_LONG 125168
#define MOCK_TIME 1034

#define SIGFOX_TRANSMISSION_INTERVAL 900000    // Time between Sigfox transmissions in ms (15m).
#define GPS_FIX_STD_INTERVAL 180000             // Standard time between GPS rewuests in ms (5m = 300000ms).

#define EEPROM_ADDR_LAT 0          // Address of the Lattitude in the EEPROM.
#define EEPROM_ADDR_LONG 4         // Address of the Lattitude in the EEPROM.
#define EEPROM_ADDR_GPS_TIM 8      // Address of the last-GPS-fix-time  in the EEPROM.
#define EEPROM_ADDR_SF_TIM 12      // Address of the last-Sigfox-Transission-time  in the EEPROM.
#define EEPROM_ADDR_WDT_COUNT 16   // Address of the WDT reset count in the EEPROM.

SoftwareSerial gpsSerial(A3, A2); // RX, TX

char la[7];
char lo[8];
char ti[7];

#define GPS_INFO_BUFFER_SIZE 128
char GPS_info_char;
char GPS_info_buffer[GPS_INFO_BUFFER_SIZE];
unsigned int received_char = 0;
bool message_started=false;

long lastSfSendTime = 0;                    // Last time (of day) the last Sigfox transmission was sent.
long lastGPSUpdateTime = 0;                 // Last time (of day) the GPS position was successfully updated.
long latitude;                              // Last valid GPS lat.
long longtitude;                            // Last valid GPS long.
long gpsTime;                               // Last valid GPS time.

unsigned long prevMillies = 0;       // last time millis() was called.
unsigned long currentTime = 0;       // The current Time Of Day in milli seconds. Time is UTC.
unsigned long WDT_resetCount = 0;


// WDT Interrupt function.
ISR(WDT_vect)
{
  eepromWrite(WDT_resetCount+1, EEPROM_ADDR_WDT_COUNT);
}

void initWDT(){
  /*
   * WatchDog control register:
   * WDIF: Watchdog interrupt flag. Set high by the system.
   * WDIE: Watchdog interrupt enable.
   * WDP[3]: Watchdog Timer Prescaler3
   * WDCE: Watchdog Change Enable.
   * WDE: Watchdog System Reset Enable.
   * WDP[2:0]: Watchdog Timer Prescaler.
   * 
   * WDIF:      '1'     
   * WDIE:      '1'
   * WDP[3]:    '1'
   * WDCE:      '0'
   * WDE:       '1'
   * WDP[2:0]:  '001'   
   * 
   * Setting WDP[3:0] to '1001' result in 8000ms timeout.
   * 
   */

   cli();   // Diabling interrupts.
   wdt_reset();   // Reset Watchdog Timer.

   // Entering EDT setup mode, by setting WDCE and WDE.
   WDTCSR |= (1<<WDCE) | (1<<WDE);

   // Setting the WDT control reg
   WDTCSR = 0xE9;

   sei();   // Enabling interrupts.
}

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


  //******************************//
  //           WDT                //
  //******************************//
  initWDT();
  wdt_reset();   // Reset Watchdog Timer.

  WDT_resetCount = eepromRead(EEPROM_ADDR_WDT_COUNT);

  Serial.print("WDT_RC:  ");
  Serial.println(WDT_resetCount);
  gpsSerial.print("WDT_RC:  ");
  gpsSerial.println(WDT_resetCount);
  
  //eepromWrite(0,EEPROM_ADDR_WDT_COUNT);

  
  //******************************//
  // Get old data from EEPROM     //
  //******************************//
//  Serial.println(eepromRead( EEPROM_ADDR_LAT));
//  Serial.println(eepromRead( EEPROM_ADDR_LONG));
//  Serial.println(eepromRead( EEPROM_ADDR_GPS_TIM));
//  Serial.println(eepromRead( EEPROM_ADDR_SF_TIM));

  // Reading old values from EEPROM.
  lastSfSendTime = eepromRead( EEPROM_ADDR_SF_TIM);                     // Last time (of day) the last Sigfox transmission was sent.
  lastGPSUpdateTime = eepromRead( EEPROM_ADDR_GPS_TIM);                 // Last time (of day) the GPS position was successfully updated.
  latitude = eepromRead( EEPROM_ADDR_LAT);                              // Last valid GPS lat.
  longtitude = eepromRead( EEPROM_ADDR_LONG);                           // Last valid GPS long.
  
  
  currentTime = lastGPSUpdateTime;              // Setting the starting time to the last GPS update, as it is closer than 0.

  
  gpsSerial.println("End of Setup");
  Serial.println("End of Setup");
}

void loop() { // run over and over

//  byte payloadData[] = {'H','E','L','L','O','W','O','R','L','D',4,2};
  byte payloadData[] = {0,0,0,0,0,0,0,0,0,0,0,0};
  boolean transmitPayloadFlag = false;              // Flag set when transmitting data.
  boolean sfTransmitAllowed_Time = false;              // Flag for allowing new Sigfox transmission, based on time since last.
  
  
  boolean updateGPS = false;
  boolean validGPS = false;
  boolean gpsUpdateAllowedTime = false;       // Flag for allowing new GPS update, based on time since last.
  
  char gpsTTemp[2] = {0,0};

  long debugLong = 0;

  
  
  //byte tHour = 0;
  //byte tMin = 0;
  //byte tSec = 0;
  

  while(1){
    wdt_reset();   // Reset Watchdog Timer.
    //******************************//
    //           Serial             //
    //******************************// 
    

    //******************************//
    //           Time               //
    //******************************//
    wdt_reset();   // Reset Watchdog Timer.
    updatedTOD();

    // Prints time only when not using Sigfox
    if(currentTime > debugLong+1000 && TEST_SIGFOX){
      debugLong = currentTime;
      Serial.print("TOD: ");
      Serial.print((currentTime));
      Serial.print(" ");
      Serial.print(floor((float)currentTime/60/60/1000));
      Serial.print(" ");
      Serial.print( floor( (currentTime/60/60/10)%100)/100*60 );
      Serial.print(" ");
      Serial.print(floor((currentTime/1000)%60));
      Serial.print("   ");
      Serial.println(millis());
    }

    // Check Wether enoough time has passed for a GPS update.
    if(gpsUpdateAllowedTime ) {      // && gpsUpdateAllowedTime
      gpsUpdateAllowedTime = false;
      updateGPS = true;
      //Serial.read();
    }
    // Check Wether enoough time has passed for a Sigfox transmission.
    if(  (lastSfSendTime + SIGFOX_TRANSMISSION_INTERVAL) < currentTime){
      sfTransmitAllowed_Time = true;

      //Serial.read();
    }
    
    //

    //******************************//
    //            GPS               //
    //******************************//
    wdt_reset();   // Reset Watchdog Timer.
    // If GPS_FIX_STD_INTERVAL time has passed, allow an update of the GPS pos.
    if(((lastGPSUpdateTime + GPS_FIX_STD_INTERVAL) < currentTime) ){
      gpsUpdateAllowedTime = true;
    }
    // If 'currentTime' is less than 'lastGPSUpdateTime' we have wrapped around at midnight, 
    // adjust 'lastGPSUpdateTime' to make sure new updates will happen.
    if( lastGPSUpdateTime > currentTime ){
      lastGPSUpdateTime = 600000;           // Set 'lastGPSUpdateTime' to 00:10:00.
    }
   
    
    // Updates the GPS postion and time.
    if(updateGPS || sfTransmitAllowed_Time){
      //Serial.println("Fecthing");
      wdt_reset();   // Reset Watchdog Timer.
      fetch();
      wdt_reset();   // Reset Watchdog Timer.
      //Serial.println("Done Fecthing");
      

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
        lastGPSUpdateTime *= 1000;                   // converting into milli seconds.
        
        // Using GPS time to set current TOD.
        prevMillies = millis();
        currentTime = lastGPSUpdateTime;       // Setting time of day var to the gps time.

        // Saving latest values in the EEPROM.
        eepromWrite(latitude, EEPROM_ADDR_LAT);
        eepromWrite(longtitude, EEPROM_ADDR_LONG);
        eepromWrite(lastGPSUpdateTime, EEPROM_ADDR_GPS_TIM);
        
        // printing studd, used for debugging.
        /*
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
        */
      }
      updateGPS = false;
    }
    
    

    //******************************//
    //          SIGFOX              //
    //******************************//
    wdt_reset();   // Reset Watchdog Timer.
    
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
    // Checks if all conditions are met in order to allow a transmission.
    if(sfTransmitAllowed_Time){
      sfTransmitAllowed_Time = false;
      transmitPayloadFlag = true;
    }
    
    // Sends the payload
    if((USE_SIGFOX || TEST_SIGFOX) && transmitPayloadFlag ){
      transmitPayloadFlag = false;
      gpsSerial.write("TRANSMITTING DATA!\r\n");
      
      
      if(USE_SIGFOX){
        wdt_reset();   // Reset Watchdog Timer.
        Send_Pload(payloadData,12);       // Send payload.
      }
      wdt_reset();   // Reset Watchdog Timer.
      updatedTOD();                     // Update time.
      lastSfSendTime = currentTime;     // Save time.
      eepromWrite(lastSfSendTime, EEPROM_ADDR_SF_TIM);  // save time to EEPROM.
      
      if(TEST_SIGFOX){
        gpsSerial.println("PAYLOAD: ");
        gpsSerial.print("Data [HEX]: ");
        for(int i=0; i<12;i++){
          gpsSerial.print(payloadData[i],HEX);
          gpsSerial.print(' ');
        }
        gpsSerial.print("\n\r");
        gpsSerial.print("LAT: ");
        debugLong = (((long)payloadData[0])<<16) + (((long)payloadData[1])<<8) + payloadData[2];
        gpsSerial.print(debugLong); gpsSerial.print("  ");
        gpsSerial.print("LONG: ");
        debugLong = (((long)payloadData[3])<<16) + (((long)payloadData[4])<<8) + payloadData[5];
        gpsSerial.print(debugLong); gpsSerial.print("  ");
        gpsSerial.print("TIME: ");
        debugLong = (((long)payloadData[6])<<8) + payloadData[7];
        gpsSerial.print(debugLong); gpsSerial.print("  ");

        gpsSerial.println("\n\r");
      }
    }
    
    
  }
}

// Updates the global time variable: Current time and prevTime.
// This fuction adds the time since last call to the 'currentTime'. The time is Time of Day in ms.
void updatedTOD(){
  currentTime += (millis()-prevMillies);
  prevMillies = millis();
  
}

// Fetches GPS coordinates, and updates global containers, la[6], lo[7], ti[6].
// loops until valid coordinates are recieved.
// Needs Global char arrays: la[6], lo[7], ti[6].
void fetch() {
  //Serial.println("fetching");
  bool newdat=false;
  long oldSendMill = 0;
  
  while(!newdat){
  //If message is not started, send a request for the gps module.
  if (!message_started && ( (oldSendMill+10) < millis() ) ) {
      gpsSerial.println("$PUBX,00*33");
      oldSendMill = millis();
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

// Writes a long to the EEPROM at adress 'addr'.
// Return 1 on succes, returns -1 if addr is out of range.
//https://playground.arduino.cc/Code/EEPROMReadWriteLong
int eepromWrite(long value , unsigned int addr)
{
    
    if (addr >= EEPROM.length()) {
      return -1;
    }
    
    //Decomposition from a long to 4 bytes by using bitshift.
    //One = Most significant -> Four = Least significant byte
    byte four = (value & 0xFF);
    byte three = ((value >> 8) & 0xFF);
    byte two = ((value >> 16) & 0xFF);
    byte one = ((value >> 24) & 0xFF);
    
    //Write the 4 bytes into the eeprom memory.
    EEPROM.write(addr, four);
    EEPROM.write(addr + 1, three);
    EEPROM.write(addr + 2, two);
    EEPROM.write(addr + 3, one);
    return 1;
}

// Reads a long from the EEPROM at adress 'addr'.
// Return the read value on succes, returns 0 if addr is out of range.
//https://playground.arduino.cc/Code/EEPROMReadWriteLong
long eepromRead( unsigned int addr )
{   
    if (addr >= EEPROM.length()) {
      return 0;
    }
    
    //Read the 4 bytes from the eeprom memory.
    long four = EEPROM.read(addr);
    long three = EEPROM.read(addr + 1);
    long two = EEPROM.read(addr + 2);
    long one = EEPROM.read(addr + 3);
    
    //Return the recomposed long by using bitshift.
    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

