#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DigitalIO.h>
#include "SlaveComms.h"
#include "SystemStatus.h"

const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
const int RS485_SENDMODE_PIN = 12;

SoftwareSerial rs485serial(RS485_RX_PIN, RS485_TX_PIN, false);  // use TRUE logic i.e. IDLE = 5 volts

enum BusState {IDLE_BUS, SENDING, WAITING_FOR_REPLY, RECEIVING, ERROR_RECOVERY};
enum ErrorState {OK, REPLY_RECEIVED_WHEN_NOT_WAITING, TIMEOUT_WAITING_FOR_REPLY, TIMEOUT_DURING_REPLY, REPLY_DID_NOT_START_WITH_ACK_BYTE, UNEXPECTED_BYTE_ID, REPLY_TOO_LONG, CRC_FAILURE, ASSERT_ERROR}

ErrorState lastError;
BusState busState;
unsigned long millisLastTransition;
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

void setupSlaveComms()
{
  pinMode(RS485_RX_PIN, INPUT);
  pinMode(RS485_TX_PIN, OUTPUT);
  pinMode(RS485_SENDMODE_PIN, OUTPUT);
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  rs485serial.begin(4800);
  busState = IDLE_BUS;
  lastError = OK;
}

void tickSlaveComms(TimeStamp timenow)
{
  while (rs485serial.available()) {
    unsigned char c = rs485serial.read();
    switch (busState) {
      case ERROR_RECOVERY:
        enterNewBusState(ERROR_RECOVERY); 
        break;
      case IDLE_BUS:
      case SENDING:
        logError(REPLY_RECEIVED_WHEN_NOT_WAITING);
        enterNewBusState(ERROR_RECOVERY); 
        break;  
      case WAITING_FOR_REPLY:
        if (c != ACK_BYTE) {
          logError(REPLY_DID_NOT_START_WITH_ACK_BYTE);
          enterNewBusState(ERROR_RECOVERY); 
        } else {
          replyidx = 0;
          enterNewBusState(RECEIVING); 
        }
        break;
      case RECEIVING:
        if (replyidx >= BUFFLEN) {
          enterNewBusState(ERROR_RECOVERY); 
          logError(REPLY_TOO_LONG);
        } else {
          replybuffer[replyidx++] = c;
          enterNewBusState(RECEIVING); 
        }
        break;
      default:
        logError(ASSERT_ERROR);
        break;
    }
  }  

  switch (busState) {
    case ERROR_RECOVERY:
      if (millis() - millisLastTransition) > RECOVERY_MILLIS) enterNewBusState(IDLE_BUS); 
      break;
    case IDLE_BUS:
    case SENDING:
      break;  
    case WAITING_FOR_REPLY:
      if (millis() - millisLastTransition) > REPLY_START_TIMEOUT_MILLIS) {
        logError(TIMEOUT_WAITING_FOR_REPLY);
        enterNewBusState(ERROR_RECOVERY); 
      }
      break;
    case RECEIVING:
      if (replyidx == BUFFLEN) {
        if (millis() - millisLastTransition) > REPLY_END_WAIT_MILLIS) {
          enterNewBusState(IDLE_BUS);
          parseReply();
        }
      } else {
        if (millis() - millisLastTransition) > REPLY_CHARACTER_TIMEOUT_MILLIS) {
        logError(TIMEOUT_DURING_REPLY);
        enterNewBusState(ERROR_RECOVERY); 
      }
      break;
    default:
      logError(ASSERT_ERROR);
      break;
  }
}

void enterNewBusState(BusState newState) {
  busState = newState;
  millisLastTransition = millis();
}

void logError(ErrorState newError) {
  lastError = newError;
}

bool isBusFree() {
  return (busState == IDLE_BUS);
}

// Send the given command on the RS485 serial bus.
// Puts the line into write mode, sends the command details including CRC16 checksum, then places line back into read mode
// returns true for success, false otherwise
bool sendCommand(RemoteModule &target, unsigned char byteid, unsigned char bytecommand, unsigned long dwordparameter)
{
  if (busState != IDLE_BUS) return false;
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

  enterNewBusState(SENDING);
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

  enterNewBusState(WAITING_FOR_REPLY);
  replyidx = 0;
  
  return (byteswritten == BUFFLEN);
}

void parseReply() {
  if (replyidx != BUFFLEN) {
    logError(ASSERT_ERROR);
    return;
  }
  unsigned short replyCRC16 = crc16(replybuffer, BASELEN);
  if (replybuffer[BASELEN] != (replyCRC16 & 0xff) || replybuffer[BASELEN+1] != (replyCRC16 >> 8) & 0xff) {
    logError(CRC_FAILURE);
    enterNewBusState(ERROR_RECOVERY);
    return;
  }
  if (replybuffer[0] != writebuffer[0]) {
    logError(UNEXPECTED_BYTE_ID);
    enterNewBusState(ERROR_RECOVERY);
    return;
  }
  RemoteModule &target = RemoteModule::getModule(targetModuleID);
  target.receiveMessage(replyBuffer[1], replyBuffer[2] + (replyBuffer[3] >> 8) + (replyBuffer[4] >> 16) + (replyBuffer[5] >> 24));
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
bool sendCommandTestChar()
{
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

 
