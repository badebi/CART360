


#include <SharpIR.h>

#define LEDR 11
#define LEDG 10
#define LEDB 9

#define HEATER 3

#define VIB1 6

#define ir A0
#define model 20150
// ir: the pin where your sensor is attached
// model: an int that determines your sensor:  1080 for GP2Y0A21Y
//                                            20150 for GP2Y0A02Y
//                                            (working distance range according to the datasheets)

SharpIR SharpIR(ir, model);

int wiper = 0;
int prevStage;
int currentStage;

boolean wipeUp = 1;

//int ledPin = 13;                // choose the pin for the LED

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);


  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(VIB1, OUTPUT);

}

void loop() {
  delay(15);



  //  unsigned long pepe1 = millis(); // takes the time before the loop on the library begins

  int dis = SharpIR.distance(); // this returns the distance to the object you're measuring


  Serial.print("Mean distance: ");  // returns it to the serial monitor
  Serial.println(dis);

  //  unsigned long pepe2 = millis() - pepe1; // the following gives you the time taken to get the measurement
  //  Serial.print("Time taken (ms): ");
  //  Serial.println(pepe2);

  if (dis <= 30) {
    if (prevStage != 3) wiper = 0;
    analogWrite(LEDR, wiper + wiper/2);
    digitalWrite(HEATER, HIGH);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, LOW);
    analogWrite(VIB1, 255);
    
    prevStage = 3;
  } else if (31 < dis && dis < 57) {   
    if (prevStage != 2) wiper = 0;
    digitalWrite(LEDR, LOW);
    analogWrite(LEDB, wiper);
    digitalWrite(LEDG, LOW);
    analogWrite(VIB1, 150);
    prevStage = 2;
  } else if (57 <= dis && dis < 70) {
    if (prevStage != 1)wiper = 0;
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDB, LOW);
    analogWrite(LEDG, wiper);
    digitalWrite(HEATER, LOW);
    analogWrite(VIB1, 0);
    prevStage = 1;
  }  else {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDB, LOW);
    digitalWrite(LEDG, LOW);
    analogWrite(VIB1, 0);
    prevStage = 0;
  }

 wiper += 3;
//  else if (!wipeUp) wiper -+ 3;
}
