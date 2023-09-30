#include "Valves.h"
#include "SystemStatus.h"

void Valve::checkID() {
  if (m_ID >=0 && m_ID < AllValveStates::VALVE_COUNT) {
    return;
  }
  m_ID = INVALID_VALVE;
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

bool Valve::getValveNewState() {
  checkID();
  return s_valveStatesNew.getValveState(*this);
}

bool Valve::getValveCurrentState() {
  checkID();
  return s_valveStatesCurrent.getValveState(*this);
}

void Valve::setValveNewState(bool newState) {
  checkID();
  s_valveStatesNew.setValveState(*this, newState);
}

float Valve::s_flowratesLPM[AllValveStates::VALVE_COUNT] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
long Valve::s_maxOnTimeSeconds[AllValveStates::VALVE_COUNT] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
AllValveStates Valve::s_valveStatesCurrent;
AllValveStates Valve::s_valveStatesNew;

Valve Valve::emptyValve() {
  return Valve(INVALID_VALVE);
}

void Valve::tick() {
  for (int i = 0; i < AllValveStates::VALVE_COUNT; ++i) {
    Valve valve(i);
    s_valveStatesCurrent.setValveState(valve, s_valveStatesNew.getValveState(valve));  //todo add comms code
    s_valveStatesNew.setValveState(valve, false);
  }
}
// ---------------------

AllValveStates::AllValveStates() {
  for (auto &state :m_states) {
    state = 0;
  }
}

bool AllValveStates::getValveState(const Valve &valve) const {
  if (!valve.isValid()) return false;
  uint8_t valveID = valve.getID();
  if (valveID >= VALVE_COUNT) {
    assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
    return false;
  }
  uint8_t mask = 0x01 << (valveID & 0x07);
  bool bitset = (m_states[valveID / 8] & mask) != 0;
  return bitset;
}

void AllValveStates::setValveState(const Valve &valve, bool newState) {
  if (!valve.isValid()) return;
  uint8_t valveID = valve.getID();
  if (valveID >= VALVE_COUNT) {
    assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
    return;
  }
  uint8_t mask = 0x01 << (valveID & 0x07);
  if (newState) {
    m_states[valveID / 8] |= mask;
  } else {
    m_states[valveID / 8] &= ~mask;
  }
}
