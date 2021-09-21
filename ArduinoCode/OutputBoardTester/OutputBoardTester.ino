/********************************************************************/
#include "OutputBoardTester.h"
#include "Commands.h"
#include "SystemStatus.h"

/********************************************************************/

void setup(void) 
{ 
  // start serial port 
  Serial.begin(9600);
  Serial.print("Version:");
  Serial.println(OBT_VERSION); 
  Serial.println("Setting up"); 

  setupSystemStatus();
  setupCommands();
} 

void loop(void) 
{ 
  tickCommands();
  tickSystemStatus();
}
