#include "RemoteModule.h"

const int INTERVAL_BETWEEN_STATUS_CHECKS = 120;  // seconds
const int INTERVAL_TO_WAIT_FOR_REPLY = 15;  // seconds

int RemoteModule::modulesAdded = 0;

RemoteModule &RemoteModule::getModule(uint8_t moduleID) {
  for (int i = 0; i < modulesAdded; ++i) {
    if ( (s_modules[i])->m_moduleID == moduleID) {
      return *(s_modules[i]);
    }
  }
  return DummyModule::s_dummyModule;
}

//* 100 = are you alive?  Response = device status; byte0 = status (0=good), bytes 1, 2, 3 = errorcounts (debug)
 void RemoteModule::checkStatusOccasionally(TimeStamp timenow) {
  if (!isBusFree() return;
  if (timenow - m_msgSentTime < INTERVAL_TO_WAIT_FOR_REPLY) {
    return;
  }
  if (m_remoteCheckingStatus == WAITING_FOR_REPLY) {
    m_remoteModuleStatus = NOT_RESPONDING;
  }
  
  if (timenow - m_statusReceivedTime > INTERVAL_BETWEEN_STATUS_CHECKS) {
    sendCommandToModule(timenow, 100, 0);
  }
}

bool RemoteModule::receiveMessage(TimeStamp timenow, unsigned char bytecommand, unsigned long receivedStatus) {
  if (bytecommand == 100) {
    m_remoteCheckingStatus = NOT_WAITING;
    m_statusReceivedTime = timenow;
    m_statuscode = receivedStatus;
    if ((receivedStatus & 0xff) == 0) {
      m_remoteModuleErrorStatus = OK;
    } else {
      m_remoteModuleErrorStatus = HAS_AN_ERROR;
    }
    return true;
  } else if (bytecommand == 255) {
    m_remoteCheckingStatus = NOT_WAITING;
    m_statusReceivedTime = timenow;
    m_statuscode = receivedStatus;
    m_remoteModuleErrorStatus = COMMAND_UNRECOGNISED;
    return true;
  }
  return false;
}

bool RemoteModule::sendCommandToModule(TimeStamp timenow, unsigned char bytecommand, unsigned long dwordparameter) {
  m_msgSentTime = timenow;
  m_remoteCheckingStatus = WAITING_FOR_REPLY;
  return sendCommand(this, m_moduleID, bytecommand, dwordparameter);
}

bool RemoteModule::sentMsgRecently(TimeStamp timenow) {
  return (timenow - m_msgSentTime < INTERVAL_TO_WAIT_FOR_REPLY);
}
    
