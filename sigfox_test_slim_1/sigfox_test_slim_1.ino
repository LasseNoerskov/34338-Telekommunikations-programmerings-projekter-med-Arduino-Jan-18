
#include <WISOL.h>
#include <SoftwareSerial.h>

Isigfox *Isigfox = new WISOL();
uint8_t PublicModeSF;

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


  int flagInit;
  // WISOL test
  flagInit = -1;
  while (flagInit == -1) {
    Serial.println(""); // Make a clean restart
    delay(1000);
    PublicModeSF = 0;
    flagInit = Isigfox->initSigfox();
    Isigfox->testComms();
    GetDeviceID();
  }
  Serial.println(""); // Make a clean start





  
  delay(5000);

  gpsSerial.print("testComs: ");
  gpsSerial.println(Isigfox->testComms());

  delay(5000);
  gpsSerial.print("GetZone: ");
  gpsSerial.println(Isigfox->getZone());

  delay(5000);
}

void loop() { // run over and over
  int runLoop = 1;
  
  if (gpsSerial.available()) {
    Serial.write(gpsSerial.read());
  }
  if (Serial.available()) {
    gpsSerial.write(Serial.read());
  }

  
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
