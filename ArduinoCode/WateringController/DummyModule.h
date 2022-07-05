#ifndef DUMMY_MODULE_H
#define DUMMY_MODULE_H

class DummyModule : public RemoteModule {
  public:
    DummyModule(uint8_t moduleID) : RemoteModule(moduleID) {};
    void tick(TimeStamp timenow) {};
    void receiveMessage(unsigned char bytecommand, unsigned long &receivedStatus) {};

  static DummyModule s_dummyModule;
}
#endif
