#include <Arduino.h>
#include "SlaveComms.h"
#include <SoftwareSerial.h>
#include <DigitalIO.h>

const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
const int RS485_SENDMODE_PIN = 12;

SoftwareSerial rs485serial(RS485_RX_PIN, RS485_TX_PIN);

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

/*
 * Protocol for communicating with slave device is:
 * 
 * 1) Master sends !{BYTEID}{BYTECOMMAND}{DWORDCOMMANDPARAM}{CRC16} then releases bus and waits for reply
 * 2) Slave waits at least 100 ms then responds ${BYTEID}{BYTECOMMAND}{DWORDSTATUS}{CRC16} then releases bus
 * 
 * Commands:
 * 100 = are you alive?  Response = device status; 0 = good
 * 
 * For solenoids:
 * 101 = what is your current output?  Response = output (bits 0->31)
 * 102 = change output (bits 0->31).  Response = repeat target output
 * 
 */

 
