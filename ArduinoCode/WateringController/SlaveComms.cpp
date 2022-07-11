#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DigitalIO.h>
#include "SlaveComms.h"
#include "SystemStatus.h"
#include "Ticks.h"

const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
const int RS485_SENDMODE_PIN = 12;

SoftwareSerial rs485serial(RS485_RX_PIN, RS485_TX_PIN, false);  // use TRUE logic i.e. IDLE = 5 volts

enum class BusState {IDLE_BUS, SENDING, WAITING_FOR_REPLY, RECEIVING, ERROR_RECOVERY};
enum class ErrorState {OK, REPLY_RECEIVED_WHEN_NOT_WAITING, TIMEOUT_WAITING_FOR_REPLY, TIMEOUT_DURING_REPLY, REPLY_DID_NOT_START_WITH_ACK_BYTE, UNEXPECTED_BYTE_ID, REPLY_TOO_LONG, CRC_FAILURE, ASSERT_ERROR};

ErrorState lastError;
BusState busState;
Ticks millisLastTransition;
int replyidx;

const char ATTENTION_BYTE = '!';
const int BASELEN = 1+1+4;
const int CRC16LEN = 2;
const int BUFFLEN = BASELEN + CRC16LEN;
unsigned char writebuffer[BUFFLEN];

const char ACK_BYTE = '$';
unsigned char replybuffer[BUFFLEN];
uint8_t targetModuleID;

const unsigned long RECOVERY_MILLIS = 10000; // length of time to wait for the bus to recover (waits until the bus is silent for this long)
const unsigned long REPLY_START_TIMEOUT_MILLIS = 1000; // length of time to wait for a response to start after sending a message
const unsigned long REPLY_CHARACTER_TIMEOUT_MILLIS = 1000; // length of time to wait for the end of a response once it has started
const unsigned long REPLY_END_WAIT_MILLIS = 500; // length of time to wait after the expected end of a reply

void enterNewBusState(BusState newState, Ticks ticksnow) {
  busState = newState;
  millisLastTransition = ticksnow;
}

void logError(ErrorState newError) {
  lastError = newError;
}

bool isBusFree() {
  return (busState == BusState::IDLE_BUS);
}

void parseReply(Ticks ticksnow);

void setupSlaveComms()
{
  pinMode(RS485_RX_PIN, INPUT);
  pinMode(RS485_TX_PIN, OUTPUT);
  pinMode(RS485_SENDMODE_PIN, OUTPUT);
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  rs485serial.begin(4800);
  busState = BusState::IDLE_BUS;
  lastError = ErrorState::OK;
}

void tickSlaveComms(Ticks ticksnow) {
  while (rs485serial.available()) {
    unsigned char c = rs485serial.read();
    switch (busState) {
      case BusState::ERROR_RECOVERY:
        enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
        break;
      case BusState::IDLE_BUS:
      case BusState::SENDING:
        logError(ErrorState::REPLY_RECEIVED_WHEN_NOT_WAITING);
        enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
        break;  
      case BusState::WAITING_FOR_REPLY:
        if (c != ACK_BYTE) {
          logError(ErrorState::REPLY_DID_NOT_START_WITH_ACK_BYTE);
          enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
        } else {
          replyidx = 0;
          enterNewBusState(BusState::RECEIVING, ticksnow); 
        }
        break;
      case BusState::RECEIVING:
        if (replyidx >= BUFFLEN) {
          enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
          logError(ErrorState::REPLY_TOO_LONG);
        } else {
          replybuffer[replyidx++] = c;
          enterNewBusState(BusState::RECEIVING, ticksnow); 
        }
        break;
      default:
        logError(ErrorState::ASSERT_ERROR);
        break;
    }
  }  

  switch (busState) {
    case BusState::ERROR_RECOVERY:
      if (ticksnow - millisLastTransition > RECOVERY_MILLIS) enterNewBusState(BusState::IDLE_BUS, ticksnow); 
      break;
    case BusState::IDLE_BUS:
    case BusState::SENDING:
      break;  
    case BusState::WAITING_FOR_REPLY:
      if (ticksnow - millisLastTransition > REPLY_START_TIMEOUT_MILLIS) {
        logError(ErrorState::TIMEOUT_WAITING_FOR_REPLY);
        enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
      }
      break;
    case BusState::RECEIVING:
      if (replyidx == BUFFLEN) {
        if (ticksnow - millisLastTransition > REPLY_END_WAIT_MILLIS) {
          enterNewBusState(BusState::IDLE_BUS, ticksnow);
          parseReply(ticksnow);
        }
      } else {
        if (ticksnow - millisLastTransition > REPLY_CHARACTER_TIMEOUT_MILLIS) {
          logError(ErrorState::TIMEOUT_DURING_REPLY);
          enterNewBusState(BusState::ERROR_RECOVERY, ticksnow); 
        }  
      }
      break;
    default:
      logError(ErrorState::ASSERT_ERROR);
      break;
  }
}



// Send the given command on the RS485 serial bus.
// Puts the line into write mode, sends the command details including CRC16 checksum, then places line back into read mode
// returns true for success, false otherwise
bool sendCommand(RemoteModule &target, Ticks ticksnow, unsigned char byteid, unsigned char bytecommand, unsigned long dwordparameter)
{
  if (busState != BusState::IDLE_BUS) return false;
  targetModuleID = target.getID();
  writebuffer[0] = byteid;
  writebuffer[1] = bytecommand;
  writebuffer[2] = dwordparameter & 0xff;
  writebuffer[3] = (dwordparameter>>8) & 0xff;
  writebuffer[4] = (dwordparameter>>16) & 0xff;
  writebuffer[5] = (dwordparameter>>24) & 0xff;

  unsigned short checksum;
  checksum = crc16(writebuffer, BASELEN);
  writebuffer[BASELEN] = checksum & 0xff;
  writebuffer[BASELEN+1] = (checksum>>8) & 0xff;

  enterNewBusState(BusState::SENDING, ticksnow);
  digitalWrite(RS485_SENDMODE_PIN, HIGH);
  int byteswritten = rs485serial.write(ATTENTION_BYTE);
  Serial.println(ATTENTION_BYTE, HEX); 
  delay(10);
  for (int i = 0; i < 8; ++i) {
   byteswritten = rs485serial.write(writebuffer[i]);
   Serial.println(writebuffer[i], HEX); 
   delay(10);
  }
  digitalWrite(RS485_SENDMODE_PIN, LOW);

  enterNewBusState(BusState::WAITING_FOR_REPLY, ticksnow);
  replyidx = 0;
  
  return (byteswritten == BUFFLEN);
}

void parseReply(Ticks ticksnow) {
  if (replyidx != BUFFLEN) {
    logError(ErrorState::ASSERT_ERROR);
    return;
  }
  unsigned short replyCRC16 = crc16(replybuffer, BASELEN);
  if (replybuffer[BASELEN] != (replyCRC16 & 0xff) || replybuffer[BASELEN+1] != (replyCRC16 >> 8) & 0xff) {
    logError(ErrorState::CRC_FAILURE);
    enterNewBusState(BusState::ERROR_RECOVERY, ticksnow);
    return;
  }
  if (replybuffer[0] != writebuffer[0]) {
    logError(ErrorState::UNEXPECTED_BYTE_ID);
    enterNewBusState(BusState::ERROR_RECOVERY, ticksnow);
    return;
  }
  RemoteModule &target = RemoteModule::getModule(targetModuleID);
  target.receiveMessage(ticksnow, replybuffer[1], replybuffer[2] + ((uint32_t)replybuffer[3] << 8) + ((uint32_t)replybuffer[4] << 16) + ((uint32_t)replybuffer[5] << 24));
}

unsigned short crc16(const unsigned char* data_p, unsigned char length){
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}

// Send a test char on the RS485 serial bus.
// Puts the line into write mode, sends the char, then places line back into read mode
// returns true for success, false otherwise
bool sendCommandTestChar() {
  bool success = true;
  const int BASELEN = 1+1+4;
  const int CRC16LEN = 2;
  const int BUFFLEN = BASELEN + CRC16LEN;
  unsigned char writebuffer[BUFFLEN];
  writebuffer[0] = '!';

  digitalWrite(RS485_SENDMODE_PIN, HIGH);
  for (int i = 0; i < 1000; ++i) {
    int byteswritten = rs485serial.write(writebuffer, 1);
    if (byteswritten != 1) success = false;
  }  
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  return success;
}


/*
 * Protocol for communicating with slave device is:
 * 
 * 1) Master sends !{BYTEID}{BYTECOMMAND}{DWORDCOMMANDPARAM}{CRC16} then releases bus and waits for reply
 * 2) Slave waits at least 100 ms then responds ${BYTEID}{BYTECOMMAND}{DWORDSTATUS}{CRC16} then releases bus
 * 
 * Commands:
 * 100 = are you alive?  Response = device status; byte0 = status (0=good), bytes 1, 2, 3 = errorcounts (debug)
 * 
 * For solenoids:
 * 101 = what is your current output?  Response = output (bits 0->7 = current states, bits 8->15 = target states)
 * 102 = change output (bits 0->31).  Response = repeat target output
 * 
 */

 
