#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DigitalIO.h>
#include "SlaveComms.h"
#include "SystemStatus.h"

const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
const int RS485_SENDMODE_PIN = 12;

SoftwareSerial rs485serial(RS485_RX_PIN, RS485_TX_PIN, false);  // use TRUE logic i.e. IDLE = 5 volts

void setupSlaveComms()
{
  pinMode(RS485_RX_PIN, INPUT);
  pinMode(RS485_TX_PIN, OUTPUT);
  pinMode(RS485_SENDMODE_PIN, OUTPUT);
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  rs485serial.begin(4800);
}

void tickSlaveComms()
{
  static unsigned long lasttime = 0;
  
  if (rs485serial.available()) {
    unsigned char c = rs485serial.read();
    console->print(c, HEX);
    console->print(" ");     
    lasttime = millis();
  }  

  unsigned long timenow = millis();
  if (lasttime != 0 && timenow - lasttime > 500) {
    console->println("");
    lasttime = 0;
  }
  
}

// Send the given command on the RS485 serial bus.
// Puts the line into write mode, sends the command details including CRC16 checksum, then places line back into read mode
// returns true for success, false otherwise
bool sendCommand(unsigned char byteid, unsigned char bytecommand, unsigned long dwordparameter)
{
  const int BASELEN = 1+1+4;
  const int CRC16LEN = 2;
  const int BUFFLEN = BASELEN + CRC16LEN;
  unsigned char writebuffer[BUFFLEN];
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

  digitalWrite(RS485_SENDMODE_PIN, HIGH);
  int byteswritten = rs485serial.write(writebuffer, BUFFLEN);
  digitalWrite(RS485_SENDMODE_PIN, LOW);
  return (byteswritten == BUFFLEN);
}


boolean readReply(unsigned char byteid, unsigned char bytecommand, unsigned long &receivedStatus) {
  
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

 
