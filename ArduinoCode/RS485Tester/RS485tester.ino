/********************************************************************/
#include "RS485Tester.h"
#include "Commands.h"
#include "SystemStatus.h"
#include <SoftwareSerial.h>
/********************************************************************/

void setup(void) 
{ 
  // start serial port 
  Serial.begin(9600);
  Serial.print("Version:");
  Serial.println(RS485T_VERSION); 
  Serial.println("Setting up"); 

  setupSystemStatus();
  setupSlaveComms();
  setupCommands();
  Serial.println("Ready"); 
} 

void loop(void) 
{ 
  tickCommands();
  tickSlaveComms();
  tickSystemStatus();
}
