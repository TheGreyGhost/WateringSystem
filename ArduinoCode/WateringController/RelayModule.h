#ifndef RELAYMODULE_H
#define RELAYMODULE_H
#include "RemoteModule.h"

class RelayModule : public RemoteModule {
  public:
    RelayModule(uint8_t moduleID) : RemoteModule(moduleID) {};
    void changeState(uint8_t valvenumber, bool newstate);
    void tick(TimeStamp timenow);
    void receiveMessage(TimeStamp timenow, unsigned char bytecommand, unsigned long receivedStatus);
    
  private:
    bool m_states_target[8];
    bool m_states_remote[8];
    TimeStamp m_valveStateReceivedTime;
};
#endif
