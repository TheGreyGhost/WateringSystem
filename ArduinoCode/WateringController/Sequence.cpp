//#include <iostream>
#include "Sequence.h"
#include "SystemStatus.h"

//using std::cout;

ValveTime::ValveTime(Valve &valve, TimeStamp startTimeStamp, long durationSeconds) :
                      m_valve(valve), m_startTimeStamp(startTimeStamp), m_endTimeStamp(startTimeStamp + durationSeconds),
                      m_next(nullptr) {
}

// adjust the start and end time to account for a pause and unpause
void ValveTime::resumeAfterPause(TimeStamp pauseTime, TimeStamp resumeTime) {
  if (resumeTime < m_startTimeStamp || pauseTime > m_endTimeStamp) return;
  if (pauseTime < m_startTimeStamp) {
    m_startTimeStamp += resumeTime - pauseTime;
  }
  m_endTimeStamp += resumeTime - pauseTime;
}

//ValveTime::ValveTime(const ValveTime &src) : m_valve(src.m_valve) {
//  m_startTimeStamp = src.m_startTimeStamp;
//  m_endTimeStamp = src.m_endTimeStamp;
//}
//
//ValveTime &ValveTime::operator=(const ValveTime &src) {
//  if (&src == this) return *this;
//  m_valve = src.m_valve;
//  m_startTimeStamp = src.m_startTimeStamp;
//  m_endTimeStamp = src.m_endTimeStamp;
//  return *this;
//}

void ValveSequence::addValveOpenTime(Valve valve, TimeStamp startTimeStamp, int durationSeconds) {
// add the valve into time-sorted order
  checkInvariants();
  ValveTime *newValveTime = new ValveTime(valve, startTimeStamp, durationSeconds);
  ValveTime *ptr = m_ValveTimesHead;
  ValveTime *prev = nullptr;
  if (ptr == nullptr || *newValveTime < *ptr) {
    m_ValveTimesHead = newValveTime;
    newValveTime->setNext(ptr);
    return;
  }
  prev = ptr;
  ptr = ptr->getNext();
  while (ptr != nullptr) {
    if (*newValveTime < *ptr) {
      newValveTime->setNext(ptr);      
      prev->setNext(newValveTime);
      return;
    }   
    prev = ptr;
    ptr = ptr->getNext();    
  }
  prev->setNext(newValveTime);
}

long ValveSequence::getElapsedTimeSeconds() {
  checkInvariants();
  TimeStamp startTime = getStartTime();
  TimeStamp endTime = getEndTime();
  if (m_timenow > endTime) return endTime - startTime;
  return m_timenow - startTime;
}

long ValveSequence::getRemainingTimeSeconds() {
  checkInvariants();
  TimeStamp startTime = getStartTime();
  TimeStamp endTime = getEndTime();
  if (m_timenow > endTime) return 0;
  if (m_timenow < startTime) return endTime - startTime;
  return endTime - m_timenow;
}

void ValveSequence::tick(TimeStamp timenow) {
  m_timenow = timenow;
}

TimeStamp ValveSequence::getStartTime() {
  ValveTime *ptr = m_ValveTimesHead;
  if (ptr == nullptr)
    return m_timenow;
  else
    return ptr->getStartTimeStamp();;
}

TimeStamp ValveSequence::getEndTime() {
  ValveTime *ptr = m_ValveTimesHead;
  if (ptr == nullptr) return m_timenow;

  TimeStamp endTime = ptr->getEndTimeStamp();
  while (ptr->getNext() != nullptr) {
    ptr = ptr->getNext();
    if (endTime < ptr->getEndTimeStamp()) endTime = ptr->getEndTimeStamp();
  }
  return endTime;
}

// adds (merges) the src sequence into this one.
void ValveSequence::addSequence(ValveSequence &src) {
  ValveTime *ptr = src.m_ValveTimesHead;
  if (ptr == nullptr) return;
  addSequence(src, ptr->getStartTimeStamp());
}

// adds (merges) the src sequence into this one; adjusts the timestamps so that the first timestamp in the sequence is
//   advanced or delayed to newStartTime and all other timestamps advance or delay by the same amount.
// No attempt to shorten the sequence by merging overlapping times for the same valve; not worth the effort.
void ValveSequence::addSequence(ValveSequence &src, TimeStamp newStartTime) {
  ValveTime *ptr = src.m_ValveTimesHead;
  if (ptr == nullptr) return;
  long offset = newStartTime - ptr->getStartTimeStamp();
  while (ptr != nullptr) {
    addValveOpenTime(ptr->getValve(), ptr->getStartTimeStamp() + offset, ptr->getEndTimeStamp() - ptr->getStartTimeStamp());
    ptr = ptr->getNext();
  }
}

void ValveSequence::pause() {
  if (m_paused) return;
  m_paused = true;
  m_pausetime = m_timenow;
}

void ValveSequence::resume() {
  if (!m_paused) return;
  ValveTime *ptr = m_ValveTimesHead;
  while (ptr != nullptr) {
    ptr->resumeAfterPause(m_pausetime, m_timenow);
    ptr = ptr->getNext();
  }
}

void ValveSequence::checkInvariants() {
  ValveTime *ptr = m_ValveTimesHead;
  ValveTime *prev = nullptr;

  while (ptr != nullptr) {
    if (prev != nullptr) {
      if (*prev > *ptr) {
        assertFailureCode = ASSERT_INVARIANT_FAILED;
        return;
      }
    }
    prev = ptr;
    ptr = ptr->getNext();    
  }
}

ValveSequence::ValveSequence(const ValveSequence &old_obj) {
  m_ValveTimesHead = nullptr;
  deepCopyValveTimes(old_obj.m_ValveTimesHead);
}

ValveSequence::~ValveSequence() {
  freeAllValveTimes();
}

ValveSequence &ValveSequence::operator=(const ValveSequence &src) {
  if (&src == this) return *this;
  deepCopyValveTimes(src.m_ValveTimesHead);
  return *this;
}

void ValveSequence::freeAllValveTimes() {
  ValveTime *ptr;
  ValveTime *next;

  ptr = m_ValveTimesHead;   
  while (ptr != nullptr) {
    next = ptr->getNext();
    delete ptr;
    ptr = next;
  }
}

void ValveSequence::deepCopyValveTimes(ValveTime *srcHead) {
  freeAllValveTimes();
  
  ValveTime *old_ptr;
  ValveTime *new_ptr;

  old_ptr = srcHead;   
  if (old_ptr == nullptr) return;
  new_ptr = new ValveTime(*old_ptr);
  m_ValveTimesHead = new_ptr;
  old_ptr = old_ptr->getNext();
  while (old_ptr != nullptr) {
    new_ptr->setNext(new ValveTime(*old_ptr));
    old_ptr = old_ptr->getNext();
    new_ptr = new_ptr->getNext();
  }
}

#ifdef TESTHARNESS
void ValveSequence::printChain() {
  ValveTime *vtptr = m_ValveTimesHead;
  TimeStamp zeroTime(2022, 1, 1, 0, 0, 0, 0.0F);

  while (vtptr != nullptr) {
    cout << "ID:" << vtptr->getValve().getID() << ", ontime:" << vtptr->getStartTimeStamp()-zeroTime << ", offtime:" << vtptr->getEndTimeStamp()-zeroTime << std::endl;
    vtptr = vtptr->getNext();
  }
}
#endif
