//#include <iostream>
#include "Sequence.h"
#include "SystemStatus.h"

//using std::cout;

SharedMemory g_sharedMemorySequencesSchedules(1000);

TimeStamp ValveSequence::s_timenow; // the current time (last time that tick() was called for any ValveSequence)

ValveStateChange::ValveStateChange(const Valve& valve, uint16_t timeSeconds, bool state) : m_valveAndState(valve, state){
  m_timeSeconds = timeSeconds;
}

ValveStateChange ValveStateChange::emptyValveStateChange() {
  return ValveStateChange(Valve::emptyValve(), 0, false);
}

bool ValveStateChange::isValid() const {
  return m_valveAndState.getValve().isValid();
}
//------------
SuccessCode ValveSequence::addValveOpenPeriod(Valve valve, uint16_t startTimeOffsetSeconds, uint16_t durationSeconds) {
  checkInvariants();
  // add the valve into time-sorted order
  if ((int)startTimeOffsetSeconds + durationSeconds > UINT16_MAX) {
    return SuccessCode(ErrorCode::SequenceDurationExceedsMaximum);
  }
  if (durationSeconds > valve.getMaxOnTimeSeconds()) {
    return SuccessCode(ErrorCode::ValveOnTimeTooLong);
  }
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  SuccessCode successCode = insertValveStateChange(startTimeOffsetSeconds, valve, true);
  if (!successCode.succeeded()) return successCode;
  successCode = insertValveStateChange(startTimeOffsetSeconds + durationSeconds, valve, false);
  return successCode;
}

long ValveSequence::getElapsedTimeSeconds() {
  checkInvariants();
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (!vsd.m_running) {
    return 0;
  }
  if (vsd.m_paused) {
    return (vsd.m_pausetime - vsd.m_zerotime);
  }
  uint16_t sequenceDuration = getSequenceDurationSeconds();
  if (s_timenow - vsd.m_zerotime >= sequenceDuration) {
    return sequenceDuration;
  }
  return s_timenow - vsd.m_zerotime;
}

uint16_t ValveSequence::getSequenceDurationSeconds() {
  int count = m_smaToken.getValveSequenceData().m_numberOfElements;
  int i;
  for (i = count-1; i >= 0 && !m_smaToken[i].isValid(); --i);
  return (i >= 0) ? m_smaToken[i].getTimeSeconds() : 0;
//  uint16_t latestTime = 0;
//  for (int i = 0; i < count; ++i) {
//    if (m_smaToken[i].isValid()) {
//      latestTime = m_smaToken[i].getTimeSeconds();
//    }
//  }
//  return latestTime;
}

long ValveSequence::getRemainingTimeSeconds() {
  checkInvariants();
  if (!m_smaToken.getValveSequenceData().m_running) {
    return 0;
  }
  return getSequenceDurationSeconds() - getElapsedTimeSeconds();
}

void ValveSequence::tick(TimeStamp timenow) {
  s_timenow = timenow;
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (!vsd.m_running || vsd.m_paused) return;
  long secondsSinceStart = timenow - vsd.m_zerotime;
  int count = m_smaToken.getValveSequenceData().m_numberOfElements;
  if (m_smaToken[count - 1].isValid() && secondsSinceStart >= m_smaToken[count-1].getTimeSeconds()) {
    return;
  }

  //algorithm is:
  // 1) find where we are up to in the sequence, then
  // 2) work backwards to find the latest state for each valve.
  //   a) where we find a valve, set a flag
  //     so that we ignore all earlier states for that valve
  //   b) we only change the valve state if the new state is true (this is to ensure that
  //      if multiple sequences are running which share the same valve, then the valve is on
  //      whenever either of the sequences has it on)
  int timeidx;
  for (timeidx = 0;
       timeidx < count && m_smaToken[timeidx].getTimeSeconds() <= secondsSinceStart; ++timeidx);
  AllValveStates allValveStates;  // re-used as:  true = have found this valve once already

  for (int i = timeidx - 1; i >= 0; --i) {
    Valve valve = m_smaToken[i].getValve();
    if (!allValveStates.getValveState(valve)) {
      allValveStates.setValveState(valve, true);
      if (m_smaToken[i].getValveAndState().getState()) {
        valve.setValveNewState(true);
      }
    }
  }
}

TimeStamp ValveSequence::getStartTime() {
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (!vsd.m_running) {
    return s_timenow;
  }
  if (vsd.m_paused) {
    return s_timenow - (vsd.m_pausetime - vsd.m_zerotime);
  }
  return vsd.m_zerotime;
}

TimeStamp ValveSequence::getEndTime() {
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (!vsd.m_running) {
    return s_timenow;
  }
  return getStartTime() + getSequenceDurationSeconds();
}

TimeStamp ValveSequence::getLastStartTime() {
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  return vsd.m_zerotime;
}

void ValveSequence::start() {
  if (m_smaToken.getValveSequenceData().m_running) {
    stop();
  }
  m_smaToken.getValveSequenceData().m_running = true;
  m_smaToken.getValveSequenceData().m_paused = false;
  m_smaToken.getValveSequenceData().m_zerotime = s_timenow;
}

void ValveSequence::stop() {
//  if (!m_smaToken.getValveSequenceData().m_running) {
//    return;
//  }
  m_smaToken.getValveSequenceData().m_running = false;
//  int count = m_smaToken.getValveSequenceData().m_numberOfElements;
//  for (int i = 0; i < count; ++i) {
//    Valve valve = m_smaToken[i].getValve();
//    valve.setValveNewState(false);
//  }
}

void ValveSequence::pause() {
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (vsd.m_paused) return;
  vsd.m_paused = true;
  vsd.m_pausetime = s_timenow;
}

void ValveSequence::resume() {
  ValveSequenceSMAtoken::ValveSequenceData &vsd = m_smaToken.getValveSequenceData();
  if (!vsd.m_paused) return;
  vsd.m_paused = false;
  vsd.m_zerotime += s_timenow - vsd.m_pausetime;
}

void ValveSequence::checkInvariants() {
  int count = m_smaToken.getValveSequenceData().m_numberOfElements;
  uint16_t transTime = 0;
  for (int i = 0; i < count; ++i) {
    if (m_smaToken[i].isValid()) {
      if (m_smaToken[i].getTimeSeconds() < transTime) {
        assertFailureCode = ASSERT_INVARIANT_FAILED;
      } else {
        transTime = m_smaToken[i].getTimeSeconds();
      }
    }
  }
}

SuccessCode ValveSequence::insertValveStateChange(uint16_t timeSeconds, const Valve& valve, bool state) {
  int count = m_smaToken.getValveSequenceData().m_numberOfElements;
  int reservedspace = m_smaToken.getNumberOfAllocatedElements();
  if (count >= reservedspace) {
    SuccessCode successCode = m_smaToken.resize(count + 1);
    if (!successCode.succeeded()) return successCode;
  }
  int startIdx = 0;
  while (startIdx < count && m_smaToken[startIdx].isValid() && m_smaToken[startIdx].getTimeSeconds() < timeSeconds) {
    ++startIdx;
  }
  m_smaToken.getValveSequenceData().m_numberOfElements += 1;
  for (int i = count; i > startIdx; --i) {
    m_smaToken[i] = m_smaToken[i-1];
  }
  m_smaToken[startIdx] = ValveStateChange(valve, timeSeconds, state);
  return SuccessCode::success();
}

void ValveSequence::transferFrom(ValveSequence &src) {
  m_smaToken.transferFrom(src.m_smaToken);
}

ValveSequence::ValveSequence() : m_smaToken() {
}

SuccessCode ValveSequence::resize(uint8_t numberOfValveActivations) {
  SuccessCode successCode = m_smaToken.resize(numberOfValveActivations * 2);
  if (!successCode.succeeded()) return successCode;
  m_smaToken.getValveSequenceData().m_numberOfElements = 0;
  stop();
  return ErrorCode::OK;
}

//--------------------------

ValveSequenceSMAtoken::ValveSequenceSMAtoken(uint8_t numberOfValveActivations) {
  int numberOfElements = numberOfValveActivations * 2;  // each valve has an on, then an off
  if (numberOfElements >= 256) {
    assertFailureCode = ASSERT_PARAMETER_OUT_OF_RANGE;
    m_sma = SharedMemoryArray();
    return;
  }
  m_sma = g_sharedMemorySequencesSchedules.allocate(
          sizeof(ValveSequenceData), sizeof (ValveStateChange), numberOfElements);
  if (m_sma.isValid()) {
    ValveSequenceData &vsd = getValveSequenceData();
    ValveStateChange *vsc = getValveStateChangesArray();
    vsd.m_numberOfElements = 0;
    vsd.m_maxNumberOfElements = numberOfElements;
    vsd.m_paused = false;
    vsd.m_running = false;
    for (int i = 0; i < numberOfElements; ++i) {
      vsc[i] = ValveStateChange::emptyValveStateChange();
    }
  }
}

SuccessCode ValveSequenceSMAtoken::resize(uint8_t newNumberOfElements) {
  uint8_t oldNumberOfElements = getNumberOfAllocatedElements();
  SuccessCode result = g_sharedMemorySequencesSchedules.resize(m_sma, newNumberOfElements);
  if (result.succeeded()) {
    ValveSequenceData &vsd = getValveSequenceData();
    vsd.m_maxNumberOfElements = newNumberOfElements;
    if (newNumberOfElements > oldNumberOfElements) {
      ValveStateChange *vsc = getValveStateChangesArray();
      for (int i = oldNumberOfElements; i < newNumberOfElements; ++i) {
        vsc[i] = ValveStateChange::emptyValveStateChange();
      }
    }
  }
  return result;
}

ValveStateChange &ValveSequenceSMAtoken::operator[](uint8_t index) {
  ValveSequenceData &vsd = getValveSequenceData();
  if (index >= vsd.m_numberOfElements) {
    assertFailureCode = ASSERT_INDEX_OUT_OF_BOUNDS;
    return *(ValveStateChange *)(m_sma.getElement(g_sharedMemorySequencesSchedules, 0));
  }
  return *(ValveStateChange *)(m_sma.getElement(g_sharedMemorySequencesSchedules, index));
}

ValveSequenceSMAtoken::ValveSequenceData &ValveSequenceSMAtoken::getValveSequenceData() {
  uint8_t *baseAddress = m_sma.getInfoBlock(g_sharedMemorySequencesSchedules);
  return *(ValveSequenceData *)baseAddress;
}

ValveStateChange *ValveSequenceSMAtoken::getValveStateChangesArray() {
  uint8_t *baseAddress = m_sma.getElement(g_sharedMemorySequencesSchedules, 0);
  return (ValveStateChange *)(baseAddress);
}

ValveSequenceSMAtoken::~ValveSequenceSMAtoken() {
  g_sharedMemorySequencesSchedules.deallocate(m_sma);
}

uint8_t ValveSequenceSMAtoken::getNumberOfAllocatedElements() {
  ValveSequenceData &vsd = getValveSequenceData();
  return vsd.m_maxNumberOfElements;
}

void ValveSequenceSMAtoken::transferFrom(ValveSequenceSMAtoken &src) {
  if (m_sma == src.m_sma) return;
  g_sharedMemorySequencesSchedules.deallocate(m_sma);
  m_sma = src.m_sma;
  src.m_sma = SharedMemoryArray();  // invalidate src
}

ValveSequenceSMAtoken::ValveSequenceSMAtoken() : m_sma() {  // starts off invalid
}
