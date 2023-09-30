//
// Created by TGG on 7/08/2022.
//

#include "Scheduler.h"
#include "SystemStatus.h"
#include "new.h"

const uint8_t UNUSED_SCHEDULE_IDX = 0xff;

ValveSequence Scheduler::s_dummyValveSequence(0);

//// The placement new is not normally defined, do it here
//void *operator new( size_t size, void *ptr ) {
//  return ptr;
//}

SuccessCode Scheduler::resizeValveSequencesArray(uint8_t newCount) {
  if (newCount == 0xff) {
    return ErrorCode::RequestedCountTooLarge;
  }
  uint8_t oldCount = m_sma_sequences.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  if (newCount == oldCount) return ErrorCode::OK;
  for (uint8_t i = newCount; i < oldCount; ++i) {
    uint8_t *raw = m_sma_sequences.getElement(g_sharedMemorySequencesSchedules, i);
    if (raw != nullptr) {
      ((ValveSequence *)raw)->~ValveSequence();
    }
  }
  SuccessCode successCode = g_sharedMemorySequencesSchedules.resize(m_sma_sequences, newCount);
  if (!successCode.succeeded()) return successCode;
  for (uint8_t i = oldCount; i < newCount; ++i) {
    uint8_t *raw = m_sma_sequences.getElement(g_sharedMemorySequencesSchedules, i);
    if (raw != nullptr) {
      new(raw)ValveSequence(1);
    }
  }
  return successCode;
}

ValveSequence &Scheduler::getValveSequence(uint8_t index) {
  uint8_t *raw = m_sma_sequences.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    return *(ValveSequence *)raw;
  } else {
    return s_dummyValveSequence;
  }
}

void Scheduler::deleteValveSequence(uint8_t index) {
  uint8_t *raw = m_sma_sequences.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    ((ValveSequence *)raw)->resize(0);
  }
}

Scheduler::WeeklySchedule &Scheduler::getWeeklySchedule(uint8_t index) {
  uint8_t *raw = m_sma_weeklySchedules.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    return *(WeeklySchedule *)raw;
  } else {
    return s_dummyWeeklySchedule;
  }
}

void Scheduler::deleteWeeklySchedule(uint8_t index) {
  uint8_t *raw = m_sma_weeklySchedules.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    ((WeeklySchedule *)raw)->sequenceIdx = UNUSED_SCHEDULE_IDX;
  }
}

Scheduler::DailyRepeatSchedule &Scheduler::getDailySchedule(uint8_t index) {
  uint8_t *raw = m_sma_dailySchedules.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    return *(DailyRepeatSchedule *)raw;
  } else {
    return s_dummyDailyRepeatSchedule;
  }
}

void Scheduler::deleteOrphanSequences() {
  uint8_t seqCount = m_sma_sequences.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  uint8_t weeklyCount = m_sma_weeklySchedules.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  uint8_t dailyCount = m_sma_dailySchedules.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  uint8_t highestUsed = 0;
  for (uint8_t i = seqCount; i > 0; --i) {
    uint8_t seqIdx = i - 1;
    bool used = false;
    for (uint8_t j = 0; j < weeklyCount; ++j) {
      WeeklySchedule &ws = *(WeeklySchedule *)
              m_sma_weeklySchedules.getElement(g_sharedMemorySequencesSchedules, j);
      if (ws.sequenceIdx == seqIdx) {
        used = true;
        break;
      }
    }
    if (!used) {
      for (uint8_t j = 0; j < dailyCount; ++j) {
        DailyRepeatSchedule &ws = *(DailyRepeatSchedule *)
                m_sma_dailySchedules.getElement(g_sharedMemorySequencesSchedules, j);
        if (ws.sequenceIdx == seqIdx) {
          used = true;
          break;
        }
      }
    }
    if (used) {
      if (seqIdx > highestUsed) highestUsed = seqIdx;
    } else{
      ValveSequence &vs = *(ValveSequence *)
              m_sma_sequences.getElement(g_sharedMemorySequencesSchedules, i);
      vs.resize(0);
    }
  }
  if (highestUsed + 1 < seqCount) {
    resizeValveSequencesArray(highestUsed + 1);
  }
}

Scheduler::Scheduler() {
  m_sma_sequences = g_sharedMemorySequencesSchedules.allocate(0, sizeof (ValveSequence), 0);
  m_sma_dailySchedules = g_sharedMemorySequencesSchedules.allocate(0, sizeof(DailyRepeatSchedule), 0);
  m_sma_weeklySchedules = g_sharedMemorySequencesSchedules.allocate(0, sizeof(WeeklySchedule), 0);
  if (!(m_sma_sequences.isValid() && m_sma_weeklySchedules.isValid() && m_sma_dailySchedules.isValid())) {
    assertFailureCode = ASSERT_SMA_ALLOCATION_FAILED;
  }
}

Scheduler::~Scheduler() {
  g_sharedMemorySequencesSchedules.deallocate(m_sma_sequences);
  g_sharedMemorySequencesSchedules.deallocate(m_sma_dailySchedules);
  g_sharedMemorySequencesSchedules.deallocate(m_sma_weeklySchedules);
}

SuccessCode Scheduler::resizeWeeklySchedulesArray(uint8_t newCount) {
  if (newCount == 0xff) {
    return ErrorCode::RequestedCountTooLarge;
  }
  uint8_t oldCount = m_sma_weeklySchedules.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  if (newCount == oldCount) return ErrorCode::OK;

  SuccessCode successCode = g_sharedMemorySequencesSchedules.resize(m_sma_weeklySchedules, newCount);
  if (!successCode.succeeded()) return successCode;
  for (uint8_t i = oldCount; i < newCount; ++i) {
    uint8_t *raw = m_sma_weeklySchedules.getElement(g_sharedMemorySequencesSchedules, i);
    if (raw != nullptr) {
      ((WeeklySchedule *)raw)->sequenceIdx = UNUSED_SCHEDULE_IDX;
    }
  }
  return successCode;
}

SuccessCode Scheduler::resizeDailySchedulesArray(uint8_t newCount) {
  if (newCount == 0xff) {
    return ErrorCode::RequestedCountTooLarge;
  }
  uint8_t oldCount = m_sma_dailySchedules.getAllocatedNumberOfElements(g_sharedMemorySequencesSchedules);
  if (newCount == oldCount) return ErrorCode::OK;

  SuccessCode successCode = g_sharedMemorySequencesSchedules.resize(m_sma_dailySchedules, newCount);
  if (!successCode.succeeded()) return successCode;
  for (uint8_t i = oldCount; i < newCount; ++i) {
    uint8_t *raw = m_sma_dailySchedules.getElement(g_sharedMemorySequencesSchedules, i);
    if (raw != nullptr) {
      ((DailyRepeatSchedule *)raw)->sequenceIdx = UNUSED_SCHEDULE_IDX;
    }
  }
  return successCode;
}

void Scheduler::deleteDailySchedule(uint8_t index) {
  uint8_t *raw = m_sma_dailySchedules.getElement(g_sharedMemorySequencesSchedules, index);
  if (raw != nullptr) {
    ((DailyRepeatSchedule *)raw)->sequenceIdx = UNUSED_SCHEDULE_IDX;
  }
}

void Scheduler::tick(TimeStamp timenow) {

}
