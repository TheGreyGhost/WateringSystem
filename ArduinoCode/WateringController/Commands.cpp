#include <ctype.h>
#include <SoftwareSerial.h>
#include "Commands.h"
#include "SystemStatus.h"
#include "SlaveComms.h"

boolean parseAndExecuteRcommand(char command[], Print *errorconsole);

const int MAX_COMMAND_LENGTH = 30;
const int COMMAND_BUFFER_SIZE = MAX_COMMAND_LENGTH + 2;  // if buffer fills to max size, truncation occurs
int commandBufferIdx = -1;
char commandBuffer[COMMAND_BUFFER_SIZE];  
const char COMMAND_START_CHAR = '!';

void setupCommands()
{
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
bool parseULongFromHexString(const char *buffer, const char * &nextUnparsedChar, unsigned long &retval)
{
  while (isspace(*buffer)) {
    ++buffer;
  }
  nextUnparsedChar = buffer;
  if (!isdigit(*buffer) && !(*buffer >= 'a' && *buffer <='f') && !(*buffer >= 'A' && *buffer <='F')) return false;
  char *forceNonConst = (char *)nextUnparsedChar;
  retval = strtol(buffer, &forceNonConst, 16);
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

// execute the command encoded in commandString.  Null-terminated
void executeCommand(char command[]) 
{
  bool commandIsValid = false;
  switch (command[0]) {
    case '?': {
      commandIsValid = true;
      console->println("commands (turn CR+LF on):");
      console->println("!s {byteID} {byteCommand} {dwordParameter}.  = Send to RS485 Example !s 5A 34 FF03 ");
      console->println("!r+ {byteID} {byteRelayNumber 0 - 7} = remote turn on relay # Example !r+ 3B 3");
      console->println("!r- {byteID} {byteRelayNumber 0 - 7} = remote turn off relay # Example !r- 3B 3");
      console->println("!r0 {byteID} = remote turn off all relays  Example !r0 3B");
      console->println("!rr {byteID} = read relay status of remote relays Example !rr 3B");
      console->println("!rs {byteID} = read status of remote controller Example !rs 3B");
      break;
    }
    case 's': {
      commandIsValid = true; 
      unsigned long retval;
      unsigned char byteid;
      unsigned char bytecommand;
      unsigned long dwordparameter;
      
      const char *nextUnparsedChar;
      bool success = parseULongFromHexString(command+1, nextUnparsedChar, retval);
      if (success && retval <= 0xFF) {
        byteid = (unsigned char)retval;
        success = parseULongFromHexString(nextUnparsedChar, nextUnparsedChar, retval);
      }
      if (success && retval <= 0xFF) {
        bytecommand = (unsigned char)retval;
        success = parseULongFromHexString(nextUnparsedChar, nextUnparsedChar, retval);
      }
      if (success) {
        dwordparameter = retval;
        success = sendCommand(byteid, bytecommand, dwordparameter);
        if (!success) {
          console->println("transmission failed"); 
        }
      } else {
        console->println("invalid parameters; type !? for help"); 
      }
      break;
    }
    case 'r': {
      commandIsValid = parseAndExecuteRcommand(command, console);
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

// parse r+, r- , r0, rr, rs
// command[0] is the leading r
// returns false if command not recognised; prints error to console as well
boolean parseAndExecuteRcommand(char command[], Print *errorconsole) 
{
  const unsigned long MIN_RELAY_NUMBER = 0;
  const unsigned long MAX_RELAY_NUMBER_PLUS_ONE = 8;
  bool needByteRelayNumber = false;
  unsigned long retval;
  unsigned char byteid;
  unsigned char relaynumber;   
  
  switch(command[1]) {
    case '+':
    case '-': {
      needByteRelayNumber = true;
      break;
    }
    case '0':
    case 'r':
    case 's': {
      break;
    }  
    default: {
      return false; 
    }
  }

  const char *nextUnparsedChar;
  bool success = parseULongFromHexString(command+2, nextUnparsedChar, retval);
  if (success && retval <= 0xFF) {
    byteid = (unsigned char)retval;
    if (needByteRelayNumber) {
      success = parseULongFromHexString(nextUnparsedChar, nextUnparsedChar, retval);
      if (success && retval >= MIN_RELAY_NUMBER && retval < MAX_RELAY_NUMBER_PLUS_ONE) {
        relaynumber = (unsigned char)retval;
      }
    }  
  }
  if (!success) {
    errorconsole->println("invalid parameters; type !? for help"); 
    return true;
  }

  switch(command[1]) {
    case '+': {
      
    }
    case '-': {
      needByteRelayNumber = true;
      break;
    }
    case '0':
    case 'r':
    case 's': {
      break;
    }  
    default: {
      return false; 
    }
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
