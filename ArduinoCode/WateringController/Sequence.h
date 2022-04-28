#ifndef SEQUENCE_H
#define SEQUENCE_H

/*
ValveSequence is a sequence of on & off actions for valves.
For example - 
Valve 1 on for 5 min, then Valve 2 on for 10 min, then both valves off.


Typical usage:

1) Construct
2a) addValveOpenTime(valve, startTimeSeconds, endTimeSeconds) to add a time period for opening the given valve
2b) loadConfig() to load the previously save config
3) call start(timeNowSeconds) to start the defined sequence running
4) call tick() at periodic intervals
5) addSequence(sequence) to merge a previous sequence into this one
6) call stop() to stop the sequence running
7) pause() and restart()

inspectors:
1) getElapsedTimeSeconds() - how far into the sequence are we?
2) getRemainingTimeSeconds() - how long until the sequence is finished?
3) getValveList(on, off, all) - returns a list of valves which match the given valve status
4) getCurrentExpectedFlowrate() - what is the expected flowrate of the currently open valves?
5) getMaximumExpectedFlowrate() - what is the expected maximum flowrate for the current sequence?
6) checkForErrors(max flowrate) - are there any logic sequence errors? maximum flowrate exceeded, too many valves at once (current draw), maximum duration exceeded

*/

class ValveTime {
  public:
    ValveTime(Valve &valve, long startTimeSeconds, long duration);
    const Valve &getValve() {return (const)m_valve;}  
    long getStartTimeSeconds() {return m_startTimeSeconds;}
    long getEndTimeSeconds() {return m_endTimeSeconds;}

    ValveTime(const ValveTime &src); 
    ValveTime &operator=(const ValveTime &src);
    bool operator==(const ValveTime &rhs) {return m_ID == rhs.m_ID};
    bool operator<=(const ValveTime &rhs) {return m_ID <= rhs.m_ID};
    bool operator!=(const ValveTime &rhs) {return m_ID != rhs.m_ID};
    bool operator<(const ValveTime &rhs) {return m_ID < rhs.m_ID};
    
}

  private:
    Valve m_valve;
    long  m_startTimeSeconds;
    long  m_endTimeSeconds;
}

class ValveSequence {
public:
  ValveSequence() {
    
  }
  
  ValveSequence (const ValveSequence &old_obj) {
     
  }
  ~ValveSequence() {
    
  }

 long getElapsedTimeSeconds();
 long getRemainingTimeSeconds();
  getValveList(on, off, all) - returns a list of valves which match the given valve status
4) getCurrentExpectedFlowrate() - what is the expected flowrate of the currently open valves?
5) getMaximumExpectedFlowrate() - what is the expected maximum flowrate for the current sequence?
6) checkForErrors(max flowrate) - are there any logic sequence errors? maximum flowrate exceeded, too many valves at once (current draw), maximum duration exceeded

private:
  ValveTime *m_ValveTimes;  //sorted into ascending order of start time
  int m_ValveTimesCount;  
}



#endif