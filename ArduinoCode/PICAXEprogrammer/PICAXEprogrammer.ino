#include <Arduino.h>
#include <DigitalIO.h>

//  Assist with reprogramming PICAXE which aren't listening to programming commands
//  I think the USB to serial interface I use also has trouble with sending serial BREAKS (serial signal raised to high for more than one frame, ~ 12 bits).

// 1) SETUP:
//   connect PICAXE pin C.5 (SerIn) to Arduino Pin 11 via 22K resistor
//   connect PICAXE pin C.0 (SerOut) to Arduino Pin 10 via 22K resistor
//   connect input from PC (normally connected to PICAXE pin C.5) to Arduino pin 8
//   connect output to PC (normally connected to PICAXE pin C.0) to Arduino pin 9
// 2) Apply power to the Arduino or press its reset button.  It will apply a continuous high to the PICAXE pin C.0, which the PICAXE interprets as a serial break
// 3a) Open the PICAXE serial monitor and verify that the PICAXE is sending its identification data (repeats at ~ 1 second intervals)
// 3b) If necessary: remove the PICAxe power (both GND and +5V) and restore.  Repeat until the identification data is being sent.
// 4) Start the download.  When the Arduino detects the input signal from the PC, it sends a low to the PICAXE pin C.0 for 3 bits, then passes through the data from 
//    the PC to the PICAXE.  Buffers these data to account for the 3 bit low duration which was inserted.  Also set the pin 13 LED to show that it is buffering.

// pin allocation:
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

const int BIT_LENGTH_US = (int)(1000000ul/4800); // 1/4800 ~ 

char outputs[8];

void setup() {
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
  ringbuffer();
}

const int BUFFER_SIZE = 256;
int ringbufferDuration[BUFFER_SIZE];
bool ringbufferState[BUFFER_SIZE];
int headIdx = 0;
int tailIdx = 0;

void ringbuffer() {
  bool serialbreak = true;
  bool buffering = false;
  int r;
  long timenow, lasttime;
  int delta;
  bool inputdetected;
  
  do {
    long timenow = micros();
    if (buffering) {
      delta = timenow - lasttime;   
      while (tailIdx != headIdx && delta > ringbufferDuration[tailIdx]) {
        delta -= ringbufferDuration[tailIdx];
        tailIdx = (tailIdx + 1) & 0xff;
      }
      
      ringbufferDuration[headIdx] += delta;
      ringbufferDuration[tailIdx] -= delta;
      r = PINB & B101;
      inputdetected = (r & 1);
      if (inputdetected != ringbufferState[headIdx]) {
        headIdx = (headIdx + 1) & 0xff;
        ringbufferDuration[headIdx] = 0;
        ringbufferState[headIdx] = inputdetected;
      }

      if (ringbufferState[tailIdx]) {
        r |= 1;    
      } else {
        r &= 0xfe; 
      }
      PORTB = (PORTB & B11000000) | outputs[r] | 32;
    } else {
      r = PINB & B101;
      inputdetected = (r & 1);
      if (serialbreak) r |= 1;
      PORTB = (PORTB & B11000000) | outputs[r];
      if (inputdetected) {
        ringbufferState[headIdx] = false;
        ringbufferDuration[headIdx] = 5 * BIT_LENGTH_US  ; headIdx = (headIdx + 1) & 0xff;
        ringbufferState[headIdx] = true;
        ringbufferDuration[headIdx] = 0;
        buffering = true;
      }
    } 
    lasttime = timenow;
  } while (true);
}

const long BREAK_US = 1000000;
const long BREAK_SPACING_QUICK_US = 20000;
const long BREAK_SPACING_SLOW_US = 200000;

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
