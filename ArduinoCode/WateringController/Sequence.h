#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "Valves.h"
#include "SharedMemory.h"
#include "SuccessCode.h"
#include "TimeStamp.h"

/*
ValveSequence is a sequence of on & off actions for valves.
For example - 
Valve 1 on for 5 min, then Valve 2 on for 10 min, then both valves off.

Typical usage:

1) Construct, specifying the maximum number of elements
2a) addValveOpenPeriod(valve, startTimeSeconds, duration) to add a time period for opening the given valve
2b) loadConfig() to load the previously save config
3) call start(timeNowSeconds) to start the defined sequence running
4) At periodic intervals:
 a) register all running sequences
 b) call tick()
 The running sequences must be re-registered before each call to tick.
5) addSequence(sequence) to merge a previous sequence into this one
6) call stop() to stop the sequence running
7) pause() and resume() are used to pause the sequence and resume it.  Pausing a sequence closes its valves.

If two or more sequences are running simultaneously and have valves in common, then the valve will be open when
   either one of its sequences request it.

inspectors:
1) getElapsedTimeSeconds() - how far into the sequence are we?
2) getRemainingTimeSeconds() - how long until the sequence is finished?
3) getValveList(on, off, all) - returns a list of valves which match the given valve status
4) getCurrentExpectedFlowrate() - what is the expected flowrate of the currently open valves?
5) getMaximumExpectedFlowrate() - what is the expected maximum flowrate for the current sequence?
6) checkForErrors(max flowrate) - are there any logic sequence errors? maximum flowrate exceeded, too many valves at once (current draw), maximum duration exceeded

 ValveSequence can be moved using .transferFrom() - eg
 dest.transferFrom(src).
 The src will be invalidated.
 */

// shared memory pool for sequences and schedules
extern SharedMemory g_sharedMemorySequencesSchedules;

class ValveAndState {
public:
  ValveAndState(const Valve& valve, bool state) {m_valveAndState = (state ? 0x80 : 0x00) | valve.getID();}
  Valve getValve() const {return Valve(m_valveAndState & 0x7f);}
  bool  getState() const {return (m_valveAndState & 0x80) != 0;}
private:
  uint8_t m_valveAndState;  // 0x80 = on, 0x00 = off.  0x7f = valve ID
};

class ValveStateChange {
  public:
    ValveStateChange(const Valve& valve, uint16_t timeSeconds, bool state);
    Valve getValve() {return Valve(m_valveAndState.getValve());}
    ValveAndState getValveAndState() {return m_valveAndState;}
    uint16_t getTimeSeconds() const {return m_timeSeconds;}
    // adjust the start and end time to account for a pause and unpause

    // returns true if this is a valid ValveStateChange.  False if it is empty of invalid for another reason.
    bool isValid() const;

    // return a valvestatechange which does nothing (is invalid)
  static ValveStateChange emptyValveStateChange();

    // the comparison operators compare the start times only.
    bool operator==(const ValveStateChange &rhs) const {return m_timeSeconds == rhs.m_timeSeconds;}
    bool operator<=(const ValveStateChange &rhs) const {return m_timeSeconds <= rhs.m_timeSeconds;}
    bool operator!=(const ValveStateChange &rhs) const {return m_timeSeconds != rhs.m_timeSeconds;}
    bool operator<(const ValveStateChange &rhs) const {return m_timeSeconds < rhs.m_timeSeconds;}
    bool operator>(const ValveStateChange &rhs) const {return m_timeSeconds > rhs.m_timeSeconds;}

  private:
  ValveAndState m_valveAndState;  // 0x80 = on, 0x00 = off.  0x7f = valve ID
    uint16_t m_timeSeconds;
};

class ValveSequenceSMAtoken {
public:
  explicit ValveSequenceSMAtoken(uint8_t numberOfElements);
  ValveSequenceSMAtoken();  // return an empty (invalid) token

  ValveSequenceSMAtoken(const ValveSequenceSMAtoken &old_obj) = delete;
  ValveSequenceSMAtoken &operator=(const ValveSequenceSMAtoken &src) = delete;

  // transfers the token from src to this.  src is invalidated.  If this is not invalid, free it first.
  void transferFrom(ValveSequenceSMAtoken &src);

  bool isValid() {return m_sma.isValid();}

  SuccessCode resize(uint8_t newNumberOfElements);
  ValveStateChange &operator[](uint8_t index);

  ~ValveSequenceSMAtoken();

private:
    struct ValveSequenceData {
        bool m_running; // true if the sequence is currently running
        bool m_paused; // true if the sequence is currently paused
        TimeStamp m_zerotime; // the time corresponding to '0' in ValveStateChange::m_timeSeconds
        TimeStamp m_pausetime; // the time that the sequence was paused
        uint8_t m_numberOfElements;  // number of elements currently in the sequence.
        uint8_t m_maxNumberOfElements; // maximum number of elements in the sequence (currently allocated space)
    };

  friend class ValveSequence;
  ValveSequenceData &getValveSequenceData();
  ValveStateChange *getValveStateChangesArray();

  uint8_t getNumberOfAllocatedElements(); // how much space is allocated for the sequence?

  SharedMemoryArray m_sma;
};

class ValveSequence {
public:
  // create a valve sequence with space for 'numberOfValveActivations'.
  //  one valve activation is a single valve turning on then off
  // Must check isAllocated() after creation to check whether the allocation was successful or not.
  explicit ValveSequence(uint8_t numberOfValveActivations) : m_smaToken(numberOfValveActivations) {}
  ValveSequence();  // return an empty (invalid) sequence
  
  ValveSequence (const ValveSequence &old_obj) = delete;
  ValveSequence &operator=(const ValveSequence &src) = delete;

  // transfers the sequence information from src to this.  src is invalidated and *this is freed first (if not already invalid)
  void transferFrom(ValveSequence &src);

  // resizes the allocated space for the sequence and erases all its contents
  SuccessCode resize(uint8_t numberOfValveActivations);

  // returns true if the sequence was allocated successfully or not
  bool isAllocated() {return m_smaToken.isValid();}

  // return the time elapsed since the sequence has started.
  // If the sequence has not started yet, returns 0
  // If the sequence is empty, returns 0
  // If the sequence is finished, returns the duration of the entire sequence.
  // The elapsed time doesn't include the periods when the sequence was paused, if any
  long getElapsedTimeSeconds();

  // return the time remaining until the sequence finishes.
  //   If the sequence has not started yet, returns the duration of the entire sequence.
  //   If the sequence is already finished, or if the sequence is empty, returns 0.
  //   If the sequence is paused, the remaining time is calculated as if the sequence is about to be unpaused immediately
  long getRemainingTimeSeconds();

  // return the duration of the sequence if it were run without pauses
  // returns 0 if the sequence is empty
  uint16_t getSequenceDurationSeconds();

  // returns the timestamp of the start of the sequence
  // if the sequence isn't running, returns the current time
  // if the sequence is finished, returns the start time
  // if the sequence has been paused the start time will be adjusted forward by the length of the pause
  TimeStamp getStartTime();

  // returns the timestamp of the end of the sequence
  // if the sequence isn't running, returns the current time
  // if the sequence is finished, returns the time that it finished
  // if the sequence is paused, returns the current time plus the remaining sequence time
  TimeStamp getEndTime();

  // returns the last time that the sequence was started, regardless of whether the sequence is
  //   currently running or not, or whether it's paused or not
  // if the sequence was paused and resumed, the time may be later than the actual start time.
  TimeStamp getLastStartTime();

  void start();
  void stop();

  // pauses the sequence (shuts all the valves in this sequence which are open)
  void pause();
  // resumes the sequence (including - reopening any valves which were closed during the pause)
  void resume();

  //  getValveList(on, off, all) - returns a list of valves which match the given valve status
//4) getCurrentExpectedFlowrate() - what is the expected flowrate of the currently open valves?
//5) getMaximumExpectedFlowrate() - what is the expected maximum flowrate for the current sequence?
//6) checkForErrors(max flowrate) - are there any logic sequence errors? maximum flowrate exceeded, too many valves at once (current draw), maximum duration exceeded

  // add an opening period for the given valve.  startTime plus duration must fit into a uint16_t
  // returns error if it couldn't be added
  SuccessCode addValveOpenPeriod(Valve valve, uint16_t startTimeOffsetSeconds, uint16_t durationSeconds);

  // tick the current sequence (turn valves on/off)
  void tick(TimeStamp timenow);

private:
  void checkInvariants();
  SuccessCode insertValveStateChange(uint16_t timeSeconds, const Valve& valve, bool state);
  ValveSequenceSMAtoken m_smaToken;  // dynamically allocated data for this ValveSequence

  static TimeStamp s_timenow; // the current time (last time that tick() was called for any ValveSequence)
};


#endif
