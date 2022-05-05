#ifndef VALVES_H
#define VALVES_H

/*
 * Valve represents a valve in the watering system
 */

class Valve {
public:
  float getFlowrateLPM();
  long getMaxOnTimeSeconds();
  void setFlowrateLPM(float flowrateLPM);
  void setMaxOnTimeSeconds(long timeSeconds);
  boolean getValveState();
  void setValveState(boolean newState);

  void shutAllValves();
  void loadSettings();
  void saveSettings();

  void tick();

  Valve(const Valve &src) {m_ID = src.m_ID; }
  Valve &operator=(const Valve &src) {m_ID = src.m_ID;}

  // get the valve with the given ID.
  static Valve getValve(int ID) {
    int i;
    for (i=0; i < VALVE_COUNT; ++i) {
      if (s_valves[i].m_ID == ID) return s_valves[i];
    }
    return s_dummyValve;
  }
    
private:
  Valve(int ID) {m_ID = ID;}

  int m_ID;
  static const int VALVE_COUNT = 10;

  void checkID(); 

  static float s_flowratesLPM[VALVE_COUNT] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}; 
  static long s_maxOnTimeSeconds[VALVE_COUNT] = {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600};
  static boolean s_valveStates[VALVE_COUNT] = {false, false, false, false, false, false, false, false, false, false}; 
  static Valve s_valves[VALVE_COUNT] = {Valve(1), Valve(2), Valve(3), Valve(4), Valve(5), Valve(6), Valve(7), Valve(8), Valve(9), Valve(10)};
  static Valve s_dummyValve(0);
}

#endif
