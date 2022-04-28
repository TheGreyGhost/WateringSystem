#include "Valves.h"
#include "SystemStatus.h"

void Valves::checkID() {
  if (m_ID >=0 && m_ID < VALVE_COUNT) {
    return;
  }
  m_ID = 0;
  assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
}

float Valves::getFlowrateLPM() {
  checkID();
  return s_flowratesLPM(m_ID);
}

long Valves::getMaxOnTimeSeconds() {
  checkID();
  return s_maxOnTimeSeconds(m_ID);
}

void Valves::setFlowrateLPM(float flowrateLPM) {
  checkID();
  s_flowratesLPM(m_ID) = flowrateLPM; 
}

void Valves::setMaxOnTimeSeconds(long timeSeconds) {
  checkID();
  s_maxOnTimeSeconds(m_ID) = timeSeconds;
}
