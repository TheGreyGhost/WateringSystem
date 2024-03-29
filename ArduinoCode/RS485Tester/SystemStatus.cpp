#include <Arduino.h>
#include <DigitalIO.h>
#include "SystemStatus.h"
#include "WatchDog.h"
#include "OutputDestination.h"
#include "RS485Tester.h"

byte assertFailureCode = 0;

Print *console;
Stream *consoleInput;

void printDebugInfo(Print &dest)
{
  dest.print("Version:"); dest.println(RS485T_VERSION); 
  dest.print("Last Assert Error:"); dest.println(assertFailureCode); 
}

DigitalPin<LED_BUILTIN> pinStatusLED;

const int MAX_ERROR_DEPTH = 16;
byte errorStack[MAX_ERROR_DEPTH];
byte errorStackIdx = 0;

void updateStatusLEDisr();

void setupSystemStatus()
{
  pinStatusLED.mode(OUTPUT);
  console = new OutputDestinationSerial();
  consoleInput = &Serial;
  WatchDog::init(updateStatusLEDisr);
  WatchDog::setPeriod(OVF_250MS);
  WatchDog::start();
}

void tickStatusLEDsequence();

void tickSystemStatus()
{
  tickStatusLEDsequence();
}

bool shutdownErrorsPresent()
{
  if (assertFailureCode != 0) {
    return true;
  }
  return false;
}

void populateErrorStack()
{
  errorStackIdx = 0;
  if (assertFailureCode != 0) {
    errorStack[errorStackIdx++] = ERRORCODE_ASSERT | assertFailureCode;
  }
}

const byte PAUSE_BETWEEN_CODES = 8; // intervals of 250 ms
const byte BIT_SPACING = 4; // intervals of 250 ms
const byte ZERO_BIT_LENGTH = 1; // intervals of 250 ms
const byte ONE_BIT_LENGTH = 2; // intervals of 250 ms
const byte NO_ERROR_FLASH_LENGTH = 4; // intervals of 250 ms  LED ON, LED OFF,

enum LedState {OK, ERROR_STACK, ERROR_STACK_BETWEEN_CODES} ledState = OK;
byte whichErrorEntry = 0;

// the sequences are shifted out MSB first, 0 = LED on, 1 = LED off.  Each bit is 250 ms, so unsigned long is 8 seconds.  The sequence stops when it is all zeros (so the last bit in the flash sequence is always a 1, 
//   corresponding to LED off
const unsigned long SEQ_BETWEEN_CODES = 0xff000000; // 2 second off
const unsigned long SEQ_NO_ERROR = 0x0f000000; // 2 seconds: 1 second off, 1 second on

const byte ZERO_BIT_CODE = 0x07; // 250 ms on, 750 ms off
const byte ONE_BIT_CODE = 0x01;  // 750 ms on, 250 ms off

volatile unsigned long flashSequence;  // each bit corresponds to a 250 ms window: 1 = LED off, 0 = LED on.  shifted out MSB first.  When flashSequence == 0, ready for next one

// The ISR actually alters the LED.  This function writes a UL for the ISR to tick to the LED.  When the ISR is finished, it 
//    writes the next one.

void tickStatusLEDsequence()
{
  if (flashSequence) return;  // wait until ISR has finished flashing the sequence

  if (ledState == ERROR_STACK) {  // if we just sent a code, make a pause
    flashSequence = SEQ_BETWEEN_CODES;
    ledState = ERROR_STACK_BETWEEN_CODES;
  } else {
    ++whichErrorEntry;
    if (whichErrorEntry >= errorStackIdx) {
      populateErrorStack();
      whichErrorEntry = 0;
    }
    if (errorStackIdx == 0) {
      flashSequence = SEQ_NO_ERROR;
      ledState = OK;
    } else {
      ledState = ERROR_STACK;
      byte errorcode = errorStack[whichErrorEntry];
      unsigned long flashSeq = 0;
      for (int i = 0; i < 8; ++i) {
         flashSeq <<= 4;
         flashSeq |= (errorcode & 0x80) ? ONE_BIT_CODE : ZERO_BIT_CODE;
         errorcode <<= 1; 
      }
      flashSequence = flashSeq;
    }
  }
}

// ISR to update the LED state from a comms register
void updateStatusLEDisr()
{
  pinStatusLED.write((flashSequence & 0x80000000UL)^0x80000000UL);
  flashSequence <<= 1;
}
