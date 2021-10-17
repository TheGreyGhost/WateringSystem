#include <Arduino.h>
#include <DigitalIO.h>

//  Assist with reprogramming PICAXE which aren't listening to programming commands
// algorithm:
// 1) in normal operation: passes serial information from PC to PICAXE and vica versa (direct pass-thru)
// 2) during startup, sends frequent serial breaks to the PICAXE (high output for more than 1 frame; for 10 ms) every 20 ms
// 3) when input is detected from the PC (manually done by user using terminal), drop back to 1 second intervals
// 4) then, once further input is detected from the PC, stop sending breaks.

// 8 = input from PC
// 9 = output to PC
// 10 = input from PICAXE
// 11 = output to PICAXE

// portB (digital pin 8 to 13)  8 = 1, 9 = 2, 10 = 4, 11 = 8, 13 = 32
//PORTB maps to Arduino digital pins 0 to 7
//
//DDRB - The Port B Data Direction Register - read/write
//PORTB - The Port B Data Register - read/write
//PINB - The Port B Input Pins Register - read only

const long BREAK_US = 1000000;
const long BREAK_SPACING_QUICK_US = 20000;
const long BREAK_SPACING_SLOW_US = 200000;

char outputs[8];

void setup() {
  Serial.begin(9600);
//  while (!Serial) ; // wait for Arduino Serial Monitor to open
//  Serial.println("Ready");
  DDRB = B11111010;  // pin 8 and pin 10 are inputs

  int r;
  for (int i = 0; i < 8; ++i) {
    r = 0;
    if (i & 1) r = 8;
    if (i & 4) r |= 2;
    outputs[i] = r;
  }
}

void loop() {
  delaytest(BREAK_SPACING_QUICK_US, true);
}

// if infiniteloop is true: once PC input is detected, loop infinitely in passthrough
void passthrough(long breakSpacingUs, bool infiniteloop) {
  long startbreak, endbreak, timenow;
  bool serialbreak;
  int r;
  bool inputdetected;
  startbreak = micros();
  serialbreak = false;  

  do {
    timenow = micros();
    if (!serialbreak && timenow >= startbreak) {
      serialbreak = true;
      endbreak = startbreak + BREAK_US;
      startbreak = timenow + breakSpacingUs;
    } else if (serialbreak && timenow >= endbreak) {
      serialbreak = false;
    }
    r = PINB & B101;
    inputdetected = (r & 1);
    if (serialbreak) r |= 1;
    PORTB = (PORTB & B11000000) | outputs[r];
  } while (!inputdetected);

  if (infiniteloop) {
    do {
      r = PINB & B101;
      PORTB = (PORTB & B11000000) | outputs[r] | 32;
    } while (true);
  }
  
}

// if infiniteloop is true: once PC input is detected, loop infinitely in passthrough
void delaytest(long breakSpacingUs, bool infiniteloop) {
  long pcInputTime, picaxeInputTime, timenow;
  bool serialbreak;
  int r;
  bool pcInputDetected, picaxeInputDetected;
  serialbreak = false;  

  do {
    timenow = micros();
    r = PINB & B101;
    pcInputDetected = (r & 1);
    picaxeInputDetected = (r & 4);
    if (picaxeInputDetected) {
      picaxeInputTime = timenow;
    }
    if (pcInputDetected) {
      pcInputTime = timenow;
    }
    r |= 1;
    PORTB = (PORTB & B11000000) | outputs[r];
  } while (!pcInputDetected);

  long milliDelta = (pcInputTime - picaxeInputTime) / 1000;
  Serial.println((int)milliDelta, DEC);

  if (infiniteloop) {
    do {
      r = PINB & B101;
      PORTB = (PORTB & B11000000) | outputs[r] | 32;
    } while (true);
  }
}
