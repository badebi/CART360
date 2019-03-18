#include <XBee.h>
#include "FilteringScheme.h"

#define myID 5

#define myIN 2

#define sleepButt 6
#define xBeeWake 19
#define VIB_PIN 9

bool interested = false;

uint8_t rID = 0;



//-------------------------------------------------------------------BLINK
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change:
long interval = 1000;           // interval at which to blink (milliseconds)
//--------------------------------------------------------------------------


// Define NewSoftSerial TX/RX pins
// Connect Arduino pin 9 to TX of usb-serial device
//uint8_t ssRX = 2;
// Connect Arduino pin 10 to RX of usb-serial device
//uint8_t ssTX = 3;
// Remember to connect all devices to a common Ground: XBee, Arduino and USB-Serial device

//NewSoftSerial nss(ssRX, ssTX);
int fadeValue = 0;

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle

Rx16Response rx16 = Rx16Response();

uint8_t payload[] {
  'T', 0, myID, rID, myIN, 0
};

//Tx64Request zbTx = Tx64Request(addr64, 0, payload, sizeof(payload),0x13);
Tx16Request tx = Tx16Request(0xFFFF, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();


//const int ledPin = 13;      // the pin that the LED is attached to
int count = 0;
int LEDState = 0;
//char fakeGND = 11;
int numNeighbor = 10;
int numLights = 4;
int currentNeighbors[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

long neighborTimers[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int fadePWMs[] = {
  0, 0, 0, 0
};

long neighborAddress[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
char displayArray[] = {
  5, 11
};

long time = (1000 * 1); //60000 milliseconds times number of minutes
long start = 0;
int calibrateRSSI[50];
int sortRSSI[50];
int threshold = 70;

long foursTimer = millis();

KalmanFilter kFilters[10];


/////////////////////////////////
long startTime = 0;
bool isSleeping = false;

bool touched = false;


void setup()
{


  // initialize the serial communication:

  xbee.setSerial(Serial);

  Serial.begin(9600);
  // pinMode(ledPin, OUTPUT);
  //pinMode(fakeGND, OUTPUT);
  // digitalWrite(ledPin, HIGH);
  //digitalWrite(fakeGND, LOW);


  start = millis();
  delay(10);
  float qVal = 0.015; //Set Q Kalman Filter
  float rVal = 1.2; //Set K Kalman Filter


  for (int i = 0; i < numNeighbor; i++) { //Initialize Kalman Filters for 10 neighbors
    //KalmanFilter(float q, float r, float p, float intial_value);
    kFilters[i].KalmanInit(qVal, rVal, 5.0, 65);
    pinMode(displayArray[i], OUTPUT);
  }


  payload[1] = threshold;
  displayOn();
  delay(1000);
  displayOff();

  pinMode (xBeeWake, OUTPUT);
  digitalWrite(xBeeWake, LOW);

  pinMode (sleepButt, INPUT);

  pinMode (VIB_PIN, OUTPUT);

}

void loop() {
  // when you press the button
  if (digitalRead(sleepButt)) {
    updateHibernate();
    delay (400);
  }

  if (isSleeping) {
    if (millis () - startTime > 10000) {
      digitalWrite( displayArray[0], HIGH);
      delay(5000);
      digitalWrite( displayArray[0], LOW);
    }
  }

  //Find neighbors
  for (int i = 0; i < 3; i++) {
    findNeighbors();
  }

  //Broadcast message to neighbors
  sendDataToNeighbors();
  //check for disconnected neighbors
  for (int i = 0; i < numNeighbor; i++) {

    if (millis() - neighborTimers[i] > 2000 ) {
      displayNeighbor(i, 0);
    }
  }
  delay(100);
}


/*
   updateHibernate()
*/
void updateHibernate() {
  if (isSleeping == false) {
    //Sleep baby
    startTime = millis();
    digitalWrite(xBeeWake, HIGH);
    isSleeping = true;
  } else {
    //wake UP
    digitalWrite(xBeeWake, LOW);
    isSleeping = false;
    // noTone(BUZZER_PIN);
  }
}

void findNeighbors() {
  /*
     Xbee SleepMode
  */



  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    // got something

    if (xbee.getResponse().getApiId() == RX_64_RESPONSE || xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      // now fill our rx class


      xbee.getResponse().getRx16Response(rx16);
      int rssiVal = rx16.getRssi();
      long address = rx16.getRemoteAddress16();
      if (address == 0) {
        if (rx16.getData(0) == 84) {
          //          Serial.println("T");
          if (rx16.getData(3) == 0 || rx16.getData(3) == myID) {
            int sID = rx16.getData(2);
            int sIN = rx16.getData(4);

            interested = compareInterests(sID, sIN);
            //            if (compareInterests(sID, sIN)) {
            //              digitalWrite(10, HIGH);
            //              //delay(250);
            //            }
          }
          threshold = rx16.getData(1);
        }
      }

      int index = IndexAddressMapper(address);
      neighborTimers[index] = millis();

      int x = kFilters[index].measureRSSI(rssiVal);
      Serial.print("RSSI :   ");
      Serial.println(x);
      /*-----------------------------------------------------------------------------------------------------------------------------------------*/
      if (x < threshold ) {


        //          interval = (x,threshold, 50, 1000, 50);

        if (x >= 65) interval = 1250;
        else if (x >= 55 && x < 65) interval = 750;
        else if (x >= 45 && x < 55 ) {
          //          noTone(BUZZER_PIN);
          interval = 250;
        }
        else {
          interval = 110;
          //          if (x < 35) digitalWrite(VIB_PIN, HIGH);
          if (x < 25) {
            //            if (i > 1023) {
            //              i = 0;
            //              noTone(BUZZER_PIN);
            //              return;
            //            }
            //            else {
            //            if (touched) {
            //              //              noTone(BUZZER_PIN);
            //              return;
            //            }

            //            digitalWrite(VIB_PIN, LOW);

            touched = true;

          }

        }
        /*----------------------------------------------------------------------------------------------------------------------------*/

        if (interested) {
          if (!touched) {
            displayNeighbor(index, 1);
          }
        }
      }
      else {
        displayNeighbor(index, 0);
      }
      //Serial.print("    ");
      //Serial.print(address);
      //      Serial.print("    ");
      //      Serial.print(threshold);
      //Serial.print("    ");
      //Serial.print(kFilters[0].r);
      //Serial.print("    ");
      //Serial.println(kFilters[0].q);

      count++;

    }
  }
}

bool compareInterests(int _sID, int _sIN) {
  if (_sIN == myIN) {
    rID = _sID;
    return true;
  }
  else return false;
}

int IndexAddressMapper(long address) {
  for (int i = 0; i < numNeighbor; i++) {
    if (neighborAddress[i] == address) {
      return i;
    }
  }

  for (int i = 0; i < numNeighbor; i++) {
    if (neighborAddress[i] == 0) {
      neighborAddress[i] = address;
      return i;
    }
  }
}

void displayNeighbor(int index, int set) {
  currentNeighbors[index] = set;
  int total = 0;
  int i;
  for (i = 0; i < numNeighbor; i++) {
    total += currentNeighbors[i];
  }


  (total > 3) ? (total = 3) : (total = total); //set total to maximum of 3 because of 0 offset in c++ array

  if (total == 0 || touched == true) {
    displayOff();
    return;
  }

  if (total == 3) {
    sendDataToNeighbors();
    return;
  }


  unsigned long currentMillis = millis();

  for (i = 0; i < total; i++ ) { //we want to display number of neighbors up to 4...2 people 2 lights
    //  digitalWrite(displayArray[0], 1); //turn on 0 whenever there is a neighbor

    //    digitalWrite(displayArray[i + 1], 1);

    //-------------------------------------------------------------- Blink without Delay--------------------------------------------------------------------------


    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      // set the LED with the ledState of the variable:
      digitalWrite(displayArray[i + 1], ledState);
      digitalWrite(displayArray[0], ledState);
    }
    //----------------------------------------------------------------------------------------------------------------------------------------------
  }


  for (i = total; i < numLights - 1; i++ ) { //turn off lights when neighbors leave
    digitalWrite(displayArray[i + 1], 0);
  }

}

void sendDataToNeighbors() {
  xbee.send(tx);
  delay(random(20, 50));
}

void displayOn() {


  for (int i = 0; i < numLights; i++) {
    digitalWrite(displayArray[i], HIGH);
  }
}

void displayOff() {


  for (int i = 0; i < numLights; i++) {
    digitalWrite(displayArray[i], LOW);
  }
}

//void fours() {
//  for (int j = 0; j < 1; j++) {
//    for (int i = 0; i < numLights; i++) {
//      for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 10) {
//        // sets the value (range from 0 to 255):
//        analogWrite(displayArray[i], fadeValue);
//        // wait for 30 milliseconds to see the dimming effect
//        delay(7);
//      }
//    }
//    for (int i = numLights - 2; i > 0; i--) {
//      for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 10) {
//        // sets the value (range from 0 to 255):
//        analogWrite(displayArray[i], fadeValue);
//        // wait for 30 milliseconds to see the dimming effect
//        delay(7);
//      }
//    }
//  }
//  for (int j = 0; j < 5; j++) {
//    for (int i = 0; i < numLights; i++) {
//      digitalWrite(displayArray[i], HIGH);
//    }
//    delay(100);
//    for (int i = 0; i < numLights; i++) {
//      digitalWrite(displayArray[i], LOW);
//    }
//    sendDataToNeighbors();
//  }
//}
