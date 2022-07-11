#ifndef REMOTEMODULE_H
#define REMOTEMODULE_H
#include "Ticks.h"

enum class RemoteModuleErrorStatus {OK, NOT_RESPONDING, HAS_AN_ERROR, COMMAND_UNRECOGNISED};
enum class RemoteModuleCheckingStatus {NOT_WAITING, WAITING_FOR_REPLY};
const uint8_t DUMMY_MODULE_ID = 0;

class RemoteModule {
  public:
    // registers a module.  0 is reserved for dummy module
    RemoteModule(uint8_t moduleID) : m_moduleID(moduleID) {
      if (moduleID != DUMMY_MODULE_ID && modulesAdded < MAX_MODULE_COUNT) {
        s_modules[modulesAdded++] = this;
      }
    }
    
    virtual void tick(Ticks ticksnow) = 0;

    // returns true if message was handled properly (used for chaining handlers)
    // the base class handles msg 100 (status) and msg 255 (unrecognised command)
    bool receiveMessage(Ticks ticksnow, unsigned char bytecommand, unsigned long receivedStatus);  
    
    RemoteModuleErrorStatus getStatus() const {return m_remoteModuleErrorStatus;}
    uint32_t getStatusCode() const {return m_statuscode;}

    void checkStatusOccasionally(Ticks ticksnow);

    static RemoteModule &getModule(uint8_t moduleID);
    uint8_t getID() const {return m_moduleID;}

  protected:
    bool sendCommandToModule(Ticks ticksnow, unsigned char bytecommand, unsigned long dwordparameter);
    bool sentMsgRecently(Ticks timenow);
    
    Ticks m_msgSentTime; 
    Ticks m_statusReceivedTime;
    RemoteModuleErrorStatus m_remoteModuleErrorStatus = RemoteModuleErrorStatus::OK;
    RemoteModuleCheckingStatus m_remoteCheckingStatus = RemoteModuleCheckingStatus::NOT_WAITING;
   
  private:
    uint8_t m_moduleID;
    uint32_t m_statuscode;
    static const int MAX_MODULE_COUNT = 10;

    void checkID(); 
 
    static RemoteModule *s_modules[MAX_MODULE_COUNT];
    static int modulesAdded;
};

#endif
