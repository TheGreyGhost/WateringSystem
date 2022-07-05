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
  bool getValveState();
  void setValveState(bool newState);
  int getID() const {return m_ID;}

  void shutAllValves();
  void loadSettings();
  void saveSettings();

  void tick();

  Valve(const Valve &src) {m_ID = src.m_ID; }
  Valve &operator=(const Valve &src) {m_ID = src.m_ID; return *this;}

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

  static  float s_flowratesLPM[VALVE_COUNT];
  static  long s_maxOnTimeSeconds[VALVE_COUNT];
  static  bool s_valveStates[VALVE_COUNT];
  static const Valve s_valves[VALVE_COUNT];
  static const Valve s_dummyValve;
};

#endif
