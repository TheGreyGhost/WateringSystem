#ifndef SLAVECOMMS_H
#define SLAVECOMMS_H
#include "Ticks.h"
#include "RemoteModule.h"

/*Send commands to slaves on the RS485 bus
 * 
 * Sequence is:
 * IDLE_BUS - ready to send
 * SENDING - has started sending a command
 * WAITING_FOR_REPLY - has completed sending the command and is waiting for the acknowledgement
 * RECEIVING - has received the start of the acknowledgement and is waiting for the rest of the reply.
 * ERROR_RECOVERY - is waiting for the bus to recover from an error
 * 
 * If WAITING_FOR_REPLY lasts longer than 1 second before the reply is received then it triggers an ERROR_RECOVERY state
 * If RECEIVING lasts longer than 1 second then it triggers an ERROR_RECOVERY state
 * 
 * In ERROR_RECOVERY, pauses for 10 seconds waiting for noise to stop
 */

void setupSlaveComms();
void tickSlaveComms(Ticks ticksnow);

bool sendCommand(RemoteModule &target, Ticks ticksnow, unsigned char byteid, unsigned char bytecommand, unsigned long dwordparameter);
bool sendCommandTestChar(); //for testing only

// calculate a CRC16 checksum of the given message
unsigned short crc16(const unsigned char* data_p, unsigned char length);

bool isBusFree();
#endif
