#include "RelayModule.h"
#include "SlaveComms.h"

const int INTERVAL_BETWEEN_VALVE_STATE_CHECKS = 90;  // seconds

void RelayModule::changeState(uint8_t valvenumber, bool newstate) {
  if (valvenumber < 1 || valvenumber > sizeof m_states_target / sizeof m_states_target[0]) return;
  m_states_target[valvenumber] = newstate;
}

// * For solenoids: bytecommands are:
// * 101 = what is your current output?  Response = output (bits 0->7 = current states, bits 8->15 = target states)
// * 102 = change output (bits 0->31).  Response = repeat target output
// Relay 0 is always set to on (see PICAXE code for reason; it is a workaround for startup conditions)
// Relays 1 to 7 are for real outputs
void RelayModule::tick(TimeStamp timenow) {
  if (!isBusFree()) return;
  if (sentMsgRecently(timenow)) return;

  m_states_target[0] = false;
  bool mismatch = false;
  uint8_t bitcode = 0;
  for (int i = 0; i < sizeof m_states_target / sizeof m_states_target[0]; ++i) {
    if (m_states_target[i] != m_states_remote[i]) mismatch = true;
    bitcode |= (1 << i);
  }

  if (mismatch) {
    sendCommandToModule(timenow, 102, bitcode);
  } else if (timenow - m_valveStateReceivedTime > INTERVAL_BETWEEN_VALVE_STATE_CHECKS) {
    sendCommandToModule(timenow, 101, 0);
  } else {
    checkStatusOccasionally(timenow);
  }
}

void RelayModule::receiveMessage(TimeStamp timenow, unsigned char bytecommand, unsigned long receivedStatus)  {
  bool handled;
  handled = RemoteModule::receiveMessage(timenow, bytecommand, receivedStatus);
  if (handled) return;
  if (bytecommand == 102) {
    m_remoteCheckingStatus = RemoteModuleCheckingStatus::NOT_WAITING;
    m_valveStateReceivedTime = timenow;
    for (int i = 0; i < 8; ++i) {
      m_states_remote[i] = (receivedStatus & (1 << i)) != 0;
    }
  }
}
