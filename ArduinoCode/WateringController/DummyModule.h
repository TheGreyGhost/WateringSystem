#ifndef DUMMY_MODULE_H
#define DUMMY_MODULE_H
#include "RemoteModule.h"
#include "Ticks.h"

class DummyModule : public RemoteModule {
  public:
    DummyModule(uint8_t moduleID) : RemoteModule(moduleID) {};
    void tick(Ticks ticksnow) {};
    void receiveMessage(unsigned char bytecommand, unsigned long &receivedStatus) {};

    static DummyModule s_dummyModule;
};
#endif
