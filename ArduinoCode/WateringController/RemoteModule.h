#ifndef REMOTEMODULE_H
#define REMOTEMODULE_H
#include "TimeStamp.h"

enum RemoteModuleErrorStatus {OK, NOT_RESPONDING, HAS_AN_ERROR, COMMAND_UNRECOGNISED};
enum RemoteModuleCheckingStatus {NOT_WAITING, WAITING_FOR_REPLY};

class RemoteModule {
  public:
    RemoteModule(uint8_t moduleID) : m_moduleID(moduleID) {
      if (modulesAdded < MAX_MODULE_COUNT) {
        s_modules[modulesAdded++] = this;
      }
    }
    
    virtual void tick(TimeStamp timenow) = 0;

    // returns true if message was handled properly (used for chaining handlers)
    // the base class handles msg 100 (status) and msg 255 (unrecognised command)
    bool receiveMessage(TimeStamp timenow, unsigned char bytecommand, unsigned long receivedStatus);  
    
    RemoteModuleErrorStatus getStatus() {return m_remoteModuleErrorStatus;}
    uint32_t getStatusCode() {return m_statuscode;}

    void checkStatusOccasionally(TimeStamp timenow);

    static RemoteModule &getModule(uint8_t moduleID);
    uint8_t getID() {return m_moduleID;}

  protected:
    bool sendCommandToModule(TimeStamp timenow, unsigned char bytecommand, unsigned long dwordparameter);
    bool sentMsgRecently(TimeStamp timenow);
    
    TimeStamp m_msgSentTime; 
    TimeStamp m_statusReceivedTime;
   
  private:
    uint8_t m_moduleID;
    RemoteModuleErrorStatus m_remoteModuleErrorStatus = OK;
    RemoteModuleCheckingStatus m_remoteCheckingStatus = NOT_WAITING;
    uint32_t m_statuscode;
    static const int MAX_MODULE_COUNT = 10;

    void checkID(); 
 
    static const RemoteModule *s_modules[MAX_MODULE_COUNT];
    static int modulesAdded;
};

#endif
