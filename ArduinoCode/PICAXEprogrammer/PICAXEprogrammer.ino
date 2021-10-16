#include <Arduino.h>
#include <SoftwareSerial.h>
// the purpose of this module is to facilitate programming the PICAXE chips
// It receives serial from the programming PC and passes it through to the PICAXE, and passes back the PICAXE' reply in turn

#include <AltSoftSerial.h>
#include <DigitalIO.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Teensy 3.0 & 3.1  21        20         22
// Teensy 2.0         9        10       (none)
// Teensy++ 2.0      25         4       26, 27
// Arduino Uno        9         8         10
// Arduino Leonardo   5        13       (none)
// Arduino Mega      46        48       44, 45
// Wiring-S           5         6          4
// Sanguino          13        14         12

// This example code is in the public domain.

SoftwareSerial picaxeCOM(8, 9, true);  
bool readyToStart = false;

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor to open
  picaxeCOM.begin(4800);
  Serial.println("PICAXEcom begin");
  readyToStart = true;
}

void loop() {
  DigitiseLoop();
//  char r;
//  if (picaxeCOM.available()) {
//    r = picaxeCOM.read();
//    Serial.print(r);
//  } else {
//    if (Serial.available()) {
//      r = Serial.read();
//      picaxeCOM.write(r);    
//    }
//  }
}
  
  int msg[100];
  int msgidx = 0;

//void normalSerialLoop() {
//  if (msgidx < 100) {
//    if (picaxe.available()) {
//      msg[msgidx++] = picaxe.read();
//    }
//  } else {
//    for (int i = 0; i < msgidx; ++i) {
//      //Serial.println(gaps[idx], DEC);
//      Serial.println(msg[i], DEC);
//    }
//    msgidx = 0;
//  }
//}

//void AltSerialLoop() {
//  if (msgidx < 100) {
//    if (altSerial.available()) {
//      msg[msgidx++] = altSerial.read();
//    }
//  } else {
//    for (int i = 0; i < msgidx; ++i) {
//      //Serial.println(gaps[idx], DEC);
//      Serial.println(msg[i], DEC);
//    }
//    msgidx = 0;
//  }
//}


void DigitiseLoop() {
  char c;
  int oldstate, newstate;
  long lasttime, thistime, delta;
  int gaps[500];
  int idx = 0;
  if (!readyToStart) return;

  lasttime = micros();
  do {
    do {
      newstate = digitalRead(8);
    } while (newstate == oldstate);  
    thistime = micros();
    digitalWrite(13, newstate);
    delta = thistime - lasttime;
    if (delta > 32000) delta = 32000;
    gaps[idx++] = (newstate ? delta : -delta);  //(newstate ? thistime - lasttime : lasttime - thistime);  
    lasttime = thistime;
    oldstate = newstate;
  } while (idx < 10);

  for (idx = 0; idx < 500; ++idx) {
    //Serial.println(gaps[idx], DEC);
    Serial.println(gaps[idx], DEC);
  }
  idx = 0;
  readyToStart = false;
//  if (Serial.available()) {
//    c = Serial.read();
//    altSerial.print(c);
//  }
//  if (altSerial.available()) {
//    c = altSerial.read();
//    Serial.print(c);
//  }
}

// troubleshooting:
// the PICAXE message does the following:
// Hello: I'm your PICAXE-18M2 from the PICAXE is detected as the following bit patterns:
//1  111101101000    --> 1 start bit, then inverted bits from LSB to MSB, then 3 stop bits.  --> 1 01001000 = 72 = H
//13  101011001000
//25  111001001000
//37  111001001000
//49  100001001000
//61  111001011000
//73  111111011000
//85  101101101000
//97  111111011000
//109 101111001000
//121 101001001000
//133 111111011000
//145 101100001000
//157 100001001000
//169 101010001000
//181 110110001000
//193 111111011000
//205 111110101000
//217 101101101000
//229 100111101000
//241 101111101000
//253 111100101000
//265 101011101000
//277 101001011000
//289 111110011000
//301 111100011000
//313 101001101000
//325 110110011000
//337 101001111000

//   When the RxD input is tied high, the PICAXE outputs a config string
