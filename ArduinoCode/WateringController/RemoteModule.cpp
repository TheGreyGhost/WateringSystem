#include "RemoteModule.h"
#include "SlaveComms.h"
#include "DummyModule.h"

const int INTERVAL_BETWEEN_STATUS_CHECKS = 120;  // seconds
const int INTERVAL_TO_WAIT_FOR_REPLY = 15;  // seconds

RemoteModule *RemoteModule::s_modules[MAX_MODULE_COUNT];
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
 void RemoteModule::checkStatusOccasionally(Ticks ticksnow) {
  if (!isBusFree()) return;
  if (ticksnow - m_msgSentTime < INTERVAL_TO_WAIT_FOR_REPLY) {
    return;
  }
  if (m_remoteCheckingStatus == RemoteModuleCheckingStatus::WAITING_FOR_REPLY) {
    m_remoteModuleErrorStatus = RemoteModuleErrorStatus::NOT_RESPONDING;
  }
  
  if (ticksnow - m_statusReceivedTime > INTERVAL_BETWEEN_STATUS_CHECKS) {
    sendCommandToModule(ticksnow, 100, 0);
  }
}

bool RemoteModule::receiveMessage(Ticks ticksnow, unsigned char bytecommand, unsigned long receivedStatus) {
  if (bytecommand == 100) {
    m_remoteCheckingStatus = RemoteModuleCheckingStatus::NOT_WAITING;
    m_statusReceivedTime = ticksnow;
    m_statuscode = receivedStatus;
    if ((receivedStatus & 0xff) == 0) {
      m_remoteModuleErrorStatus = RemoteModuleErrorStatus::OK;
    } else {
      m_remoteModuleErrorStatus = RemoteModuleErrorStatus::HAS_AN_ERROR;
    }
    return true;
  } else if (bytecommand == 255) {
    m_remoteCheckingStatus = RemoteModuleCheckingStatus::NOT_WAITING;
    m_statusReceivedTime = ticksnow;
    m_statuscode = receivedStatus;
    m_remoteModuleErrorStatus = RemoteModuleErrorStatus::COMMAND_UNRECOGNISED;
    return true;
  }
  return false;
}

bool RemoteModule::sendCommandToModule(Ticks ticksnow, unsigned char bytecommand, unsigned long dwordparameter) {
  m_msgSentTime = ticksnow;
  m_remoteCheckingStatus = RemoteModuleCheckingStatus::WAITING_FOR_REPLY;
  return sendCommand(*this, ticksnow, m_moduleID, bytecommand, dwordparameter);
}

bool RemoteModule::sentMsgRecently(Ticks ticksnow) {
  return (ticksnow - m_msgSentTime < INTERVAL_TO_WAIT_FOR_REPLY);
}
    
