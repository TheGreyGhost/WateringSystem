#ifndef COMMANDS_H   
#define COMMANDS_H  
#include <Arduino.h>
#include "Ticks.h"

/*
 * Communicate with the Raspberry Pi
 */

// prepare for receiving/executing commands
void setupCommands();

// start executing the given command
void executeCommand(char command[]);

//call at frequent intervals (eg 100 ms) to check for new commands or continue the processing of any command currently in progress
void tickCommands(Ticks ticksnow);


#endif
