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

void ValveSequence::addValveOpenTime(Valve valve, int startTimeSeconds, int durationSeconds) {
// add the valve into time-sorted order
  checkInvariants();
  ValveTime *newValveTime = new ValveTime(valve, startTimeSeconds, durationSeconds);
  ValveTime *ptr = m_valveTimes;
  ValveTime *prev = null;
  if (ptr == null || *newValveTime < *ptr) {
    m_valveTimes = newValveTime;
    newValveTime->setNext(ptr);
    return;
  }
  prev = ptr;
  ptr = ptr->getNext();
  while (ptr != null) {
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

void ValveSequence::checkInvariants() {
  ValveTime *ptr = m_valveTimes;
  ValveTime *prev = null;

  while (ptr != null) {
    if (prev != null) {
      if (*prev > *ptr) {
        assertFailureCode = ASSERT_INVARIANT_FAILED;
        return;
      }
    }
    prev = ptr;
    ptr = ptr->getNext();    
  }
}

ValveSequence::ValveSequence (const ValveSequence &old_obj) {
  m_ValveTimesHead = null;
  deepCopyValveTimes(old_obj.m_ValveTimesHead);
}

ValveSequence::~ValveSequence() {
  freeAllValveTimes();
}

ValveSequence::ValveSequence &operator=(const ValveSequence &src) {
  if (&src == this) return *this;

  deepCopyValveTimes(src.m_ValveTimesHead);
  return *this;
}


void ValveSequence::freeAllValveTimes() {
  ValveTime *ptr;
  ValveTime *next;

  ptr = m_ValveTimesHead;   
  while (ptr != null) {
    next = ptr->getNext();
    delete ptr;
    ptr = next;
  }
}

}
void ValveSequence::deepCopyValveTimes(ValveTime *srcHead) {
  freeAllValveTimes();
  
  ValveTime *old_ptr;
  ValveTime *new_ptr;

  old_ptr = srcHead;   
  if (old_ptr == null) return;
  new_ptr = new ValveTimes(*old_ptr);
  m_ValveTimesHead = new_ptr;
  old_ptr = old_ptr->getNext();
  while (old_ptr != null) {
    new_ptr->setNext(new ValveTimes(*old_ptr));
    old_ptr = old_ptr->getNext();
    new_ptr = new_ptr->getNext();
  }
}
