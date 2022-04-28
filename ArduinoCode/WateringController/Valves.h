#ifndef VALVES_H
#define VALVES_H

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

  static bool operator==(const Valve& rhs) {return m_ID == rhs.m_ID;}
    
private:
  int m_ID;
  static const int VALVE_COUNT = 10;

  void checkID(); 

  static float s_flowratesLPM[VALVE_COUNT] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}; 
  static long s_maxOnTimeSeconds[VALVE_COUNT] = {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600};
  static boolean s_valveStates[VALVE_COUNT] = {false, false, false, false, false, false, false, false, false, false}; 
}

#endif
