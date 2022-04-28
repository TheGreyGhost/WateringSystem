#include "Sequence.h"

ValveTime::ValveTime(Valve &valve, long startTimeSeconds, long duration) {
  m_valve = valve;
  m_startTimeSeconds = startTimeSeconds;
  m_endTimeSeconds = startTimeSeconds + duration;  
}

ValveTime::ValveTime(const ValveTime &src) {
  if (src == this) return;
  m_valve = src.m_valve;
  m_startTimeSeconds = src.m_startTimeSeconds;
  m_endTimeSeconds = src.m_endTimeSeconds;
}

ValveTime &operator=(const ValveTime &src) {
  if (src == this) return;
  m_valve = src.m_valve;
  m_startTimeSeconds = src.m_startTimeSeconds;
  m_endTimeSeconds = src.m_endTimeSeconds     
}
    
    const Valve &getValve() {return (const)m_valve;}  
    long getStartTimeSeconds() {return m_startTimeSeconds;}
    long getEndTimeSeconds() {return m_endTimeSeconds;}
  
  private:
    Valve m_valve;
    long  m_startTimeSeconds;
    long  m_endTimeSeconds;
}
