#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "Valves.h"
#include "TimeStamp.h"
/*
ValveSequence is a sequence of on & off actions for valves.
For example - 
Valve 1 on for 5 min, then Valve 2 on for 10 min, then both valves off.

Typical usage:

1) Construct
2a) addValveOpenTime(valve, startTimeSeconds, duration) to add a time period for opening the given valve
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
    ValveTime(Valve &valve, TimeStamp startTimeStamp, long durationSeconds);
    const Valve &getValve() {return (const Valve &)m_valve;}
    TimeStamp getStartTimeStamp() {return m_startTimeStamp;}
    TimeStamp getEndTimeStamp() {return m_endTimeStamp;}
    // adjust the start and end time to account for a pause and unpause
    void resumeAfterPause(TimeStamp pauseTime, TimeStamp resumeTime);
//    ValveTime(const ValveTime &src);
//    ValveTime &operator=(const ValveTime &src);

    // the comparison operators compare the start times only.
    bool operator==(const ValveTime &rhs) {return m_startTimeStamp == rhs.m_startTimeStamp;}
    bool operator<=(const ValveTime &rhs) {return m_startTimeStamp <= rhs.m_startTimeStamp;}
    bool operator!=(const ValveTime &rhs) {return m_startTimeStamp != rhs.m_startTimeStamp;}
    bool operator<(const ValveTime &rhs) {return m_startTimeStamp < rhs.m_startTimeStamp;}
    bool operator>(const ValveTime &rhs) {return m_startTimeStamp > rhs.m_startTimeStamp;}
    ValveTime *getNext() {return m_next;}
    void setNext(ValveTime *next) {m_next = next;}

  private:
    Valve m_valve;
    TimeStamp  m_startTimeStamp;
    TimeStamp  m_endTimeStamp;
    ValveTime *m_next;
};

class ValveSequence {
public:
  ValveSequence() : m_ValveTimesHead(nullptr), m_running(false), m_paused(false) {};
  
  ValveSequence (const ValveSequence &old_obj);
  ~ValveSequence();
  ValveSequence &operator=(const ValveSequence &src);

  // return the time elapsed since the sequence has started.
  // If the sequence has not started yet, returns a negative number equal to the number of seconds until the sequence starts.
  // If the sequence is empty, returns 0
  // If the sequence is finished, returns the duration of the entire sequence.
  // The elapsed time will include the periods when the sequence was paused, if any
  long getElapsedTimeSeconds();

  // return the time remaining until the sequence finishes.
  //   If the sequence has not started yet, returns the duration of the entire sequence.
  //   If the sequence is already finished, or if the sequence is empty, returns 0.
  long getRemainingTimeSeconds();

  // returns the timestamp of the start of the sequence
  // if the sequence is empty, returns the current time
  TimeStamp getStartTime();

  // returns the timestamp of the end of the sequence
  // if the sequence is empty, returns the current time
  TimeStamp getEndTime();

  void start() {m_running = true;}
  void stop() {m_running = false;}
  void pause();
  void resume();

  //  getValveList(on, off, all) - returns a list of valves which match the given valve status
//4) getCurrentExpectedFlowrate() - what is the expected flowrate of the currently open valves?
//5) getMaximumExpectedFlowrate() - what is the expected maximum flowrate for the current sequence?
//6) checkForErrors(max flowrate) - are there any logic sequence errors? maximum flowrate exceeded, too many valves at once (current draw), maximum duration exceeded

  void addValveOpenTime(Valve valve, TimeStamp startTimeSeconds, int durationSeconds);

  // adds (merges) the src sequence into this one.
  void addSequence(ValveSequence &src);

  // adds (merges) the src sequence into this one; adjusts the timestamps so that the first timestamp in the src sequence is
  //   advanced or delayed to newStartTime and all other timestamps advance or delay by the same amount.
  void addSequence(ValveSequence &src, TimeStamp newStartTime);

  void tick(TimeStamp timenow);

  void printChain();  // for debugging test harness purposes only

private:
  void checkInvariants();
  void freeAllValveTimes();
  void deepCopyValveTimes(ValveTime *srcHead);
  
  ValveTime *m_ValveTimesHead;  //linked list sorted into ascending order of start time
  bool m_running; // true if the sequence is currently running
  bool m_paused; // true if the sequence is currently paused
  TimeStamp m_pausetime; // the time that the sequence was paused
  TimeStamp m_timenow; // the current time (last time that tick() was called)
};

#endif
