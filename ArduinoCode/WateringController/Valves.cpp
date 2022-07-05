#include "Valves.h"
#include "SystemStatus.h"

void Valve::checkID() {
  if (m_ID >=0 && m_ID < VALVE_COUNT) {
    return;
  }
  m_ID = 0;
  assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
}

float Valve::getFlowrateLPM() {
  checkID();
  return s_flowratesLPM[m_ID];
}

long Valve::getMaxOnTimeSeconds() {
  checkID();
  return s_maxOnTimeSeconds[m_ID];
}

void Valve::setFlowrateLPM(float flowrateLPM) {
  checkID();
  s_flowratesLPM[m_ID] = flowrateLPM;
}

void Valve::setMaxOnTimeSeconds(long timeSeconds) {
  checkID();
  s_maxOnTimeSeconds[m_ID] = timeSeconds;
}

bool Valve::getValveState() {
  checkID();
  return s_valveStates[m_ID];
}

void Valve::setValveState(bool newState) {
  checkID();
  s_valveStates[m_ID] = newState;
}

const Valve Valve::s_valves[VALVE_COUNT] = {Valve(1), Valve(2), Valve(3), Valve(4), Valve(5), Valve(6), Valve(7), Valve(8), Valve(9), Valve(10)};
const Valve Valve::s_dummyValve(0);

float Valve::s_flowratesLPM[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
long Valve::s_maxOnTimeSeconds[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
bool Valve::s_valveStates[] = {false, false, false, false, false, false, false, false, false, false};
