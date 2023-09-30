#ifndef VALVES_H
#define VALVES_H
#include <Arduino.h>
/*
 * Valve represents a valve in the watering system
 * Valid IDs are 0 to VALVE_COUNT-1.
 * INVALID_VALVE is reserved for an invalid valve number - this valve does nothing
 * Usage:
 * Use Valve as a reference to a particular valve. Multiple Instances of the same
 *   valve ID refer to the same underlying valve.
 *
 * 1) call setValveNewState() for all valves which you wish to turn on.  The remainder will be turned off.
 * 2) call tick() to update the current state of all valves with the new states.
 * 3) repeat from step 1.  After each tick, the newstate of all valves is reset to off
 *
 */
class Valve;

class AllValveStates {
public:
  AllValveStates();
  bool getValveState(const Valve &valve) const;
  void setValveState(const Valve &valve, bool newState);

private:
  static const int VALVE_COUNT = 10;
  uint8_t m_states[(VALVE_COUNT+7) / 8];
  friend class Valve;
};

class Valve {
public:
  explicit Valve(uint8_t ID) {m_ID = (ID >= AllValveStates::VALVE_COUNT) ? INVALID_VALVE : ID;}

  float getFlowrateLPM();
  long getMaxOnTimeSeconds();
  void setFlowrateLPM(float flowrateLPM);
  void setMaxOnTimeSeconds(long timeSeconds);
  bool getValveCurrentState();
  bool getValveNewState();
  void setValveNewState(bool newState);
  uint8_t getID() const {return m_ID;}

  static Valve emptyValve();

  void shutAllValves();
  void loadSettings();
  void saveSettings();

  static void tick();

  Valve(const Valve &src) {m_ID = src.m_ID; }
  Valve &operator=(const Valve &src) = default;

  bool isValid() const {return m_ID < AllValveStates::VALVE_COUNT;}

private:

  uint8_t m_ID;
  static const int INVALID_VALVE = 127;

  void checkID();

  static  float s_flowratesLPM[AllValveStates::VALVE_COUNT];
  static  long s_maxOnTimeSeconds[AllValveStates::VALVE_COUNT];
  static  AllValveStates s_valveStatesCurrent;
  static AllValveStates s_valveStatesNew;
};

#endif
