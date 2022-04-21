#ifndef SLAVECOMMS_H
#define SLAVECOMMS_H

/*Send commands to slaves on the RS485 bus
 * 
 * 
 */

void setupSlaveComms();
void tickSlaveComms();

bool sendCommand(unsigned char byteid, unsigned char bytecommand, unsigned long dwordparameter);
bool sendCommandTestChar(); //for testing only

// calculate a CRC16 checksum of the given message
unsigned short crc16(const unsigned char* data_p, unsigned char length);


#endif
