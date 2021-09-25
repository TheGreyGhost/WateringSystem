#include <ctype.h>
#include <SoftwareSerial.h>
#include "Commands.h"
#include "SystemStatus.h"

const int MAX_COMMAND_LENGTH = 30;
const int COMMAND_BUFFER_SIZE = MAX_COMMAND_LENGTH + 2;  // if buffer fills to max size, truncation occurs
int commandBufferIdx = -1;
char commandBuffer[COMMAND_BUFFER_SIZE];  
const char COMMAND_START_CHAR = '!';

long timedelay = 1000; // default time (ms) for each transition
const int C_PIN = 3;
const int D_PIN = 4;
const int L_PIN = 5;

const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
const int RS485_SENDMODE_PIN = 12;

SoftwareSerial rs485serial(RS485_RX_PIN, RS485_TX_PIN);

// currently doesn't do anything in particular
void setupCommands()
{
  pinMode(C_PIN, OUTPUT);
  pinMode(D_PIN, OUTPUT);
  pinMode(L_PIN, OUTPUT);
  pinMode(RS485_RX_PIN, INPUT);
  pinMode(RS485_TX_PIN, OUTPUT);
  pinMode(RS485_SENDMODE_PIN, OUTPUT);
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  rs485serial.begin(4800);
}

// parse a long from the given string, returns in retval.  Also returns the ptr to the next character which wasn't parsed
// returns false if no valid number found (without altering retval)
bool parseLongFromString(const char *buffer, const char * &nextUnparsedChar, long &retval)
{
  while (isspace(*buffer)) {
    ++buffer;
  }
  nextUnparsedChar = buffer;
  if (!isdigit(*buffer) && *buffer != '-') return false;
  char *forceNonConst = (char *)nextUnparsedChar;
  retval = strtol(buffer, &forceNonConst, 10);
  nextUnparsedChar = (const char *)forceNonConst;
  return true;
}

// parse a long from the given string, returns in retval.  Also returns the ptr to the next character which wasn't parsed
// returns false if no valid number found (without altering retval)
bool parseFloatFromString(const char *buffer, const char * &nextUnparsedChar, float &retval)
{
  while (isspace(*buffer)) {
    ++buffer;
  }
  nextUnparsedChar = buffer;
  if (!isdigit(*buffer) && *buffer != '-') return false;
  char *forceNonConst = (char *)nextUnparsedChar;
  retval = (float)strtod(buffer, &forceNonConst);
  nextUnparsedChar = (const char *)forceNonConst;
  return true;
}


// output a pulse train on pins C, D, L.  
//  each char corresponds to a pin 
// c = c low
// C = c high
// d = d low
// D = d high
// l = l low
// L = l high
// delayms = duration of each pulse
void pulsetrain(char command [], long delayms) {
  int i;
  for (i = 0; i < strlen(command); ++i) {
    switch(command[i]) {
      case 'c': digitalWrite(C_PIN,  LOW); break;    
      case 'C': digitalWrite(C_PIN,  HIGH); break;    
      case 'd': digitalWrite(D_PIN,  LOW); break;    
      case 'D': digitalWrite(D_PIN,  HIGH); break;    
      case 'l': digitalWrite(L_PIN,  LOW); break;    
      case 'L': digitalWrite(L_PIN,  HIGH); break;    
    }
    console->print(command[i]);
    delay(timedelay);
  }
  console->println("");
}


// execute the command encoded in commandString.  Null-terminated
void executeCommand(char command[]) 
{
  bool commandIsValid = false;
  switch (command[0]) {
    case '?': {
      commandIsValid = true;
      console->println("commands (turn CR+LF on):");
      console->println("!t {time} = set command delay time (ms)");
      console->println("!cCdDlL = output train on pins C, D, L respectively.  c = low C = high etc.  Example: cDdCDd = cLo Dhi Dlo CHi Dhi Dlo");
      break;
    }
    case 'C':
    case 'c':
    case 'L':
    case 'l':
    case 'D':
    case 'd': {
      commandIsValid = true;
      pulsetrain(command, timedelay);
      break;
    }
    case 't': {
      commandIsValid = true; 
      long retval;
      const char *nextUnparsedChar;
      bool success = parseLongFromString(command+1, nextUnparsedChar, retval);
      if (success) {
        timedelay = retval;
        if (timedelay < 10) timedelay = 10;
        if (timedelay > 10000) timedelay = 10000;
        console->print("time delay set to: ");
        console->println(timedelay); 
      } else {
        console->print("invalid time delay specified"); 
      }
      break;
    }
    default: {
      break;
    }
  }

  if (!commandIsValid) {
    console->print("unknown command:");
    console->println(command);
    console->println("use ? for help");
  }
}

// look for incoming serial input (commands); collect the command and execute it when the entire command has arrived.
void tickCommands()
{
  while (consoleInput->available()) {
    if (commandBufferIdx < -1  || commandBufferIdx > COMMAND_BUFFER_SIZE) {
      assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
      commandBufferIdx = -1;
    }
    int nextChar = consoleInput->read();
    if (nextChar == COMMAND_START_CHAR) {
      commandBufferIdx = 0;        
    } else if (nextChar == '\n') {
      if (commandBufferIdx == -1) {
        console->println("Type !? for help");
      } else if (commandBufferIdx > 0) {
        if (commandBufferIdx > MAX_COMMAND_LENGTH) {
          commandBuffer[MAX_COMMAND_LENGTH] = '\0';
          console->print("Command too long:"); console->println(commandBuffer);
        } else {
          commandBuffer[commandBufferIdx++] = '\0';
          executeCommand(commandBuffer);
        } 
        commandBufferIdx = -1; 
      }
    } else {
      if (commandBufferIdx >= 0 && commandBufferIdx < COMMAND_BUFFER_SIZE) {
        commandBuffer[commandBufferIdx++] = nextChar;
      }
    }
  }
}
