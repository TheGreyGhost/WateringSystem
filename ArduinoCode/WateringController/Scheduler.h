//
// Created by TGG on 7/08/2022.
//

#ifndef WATERINGSYSTEMTESTHARNESS_SCHEDULER_H
#define WATERINGSYSTEMTESTHARNESS_SCHEDULER_H
#include "Arduino.h"
#include "Sequence.h"

class Scheduler {
public:
  Scheduler();
  ~Scheduler();

  struct WeeklySchedule {
    uint16_t dayStartTimeMinutes[7]; // eg 60 = 1 am.  [0] = Sunday
    uint8_t sequenceIdx;
  };

  struct DailyRepeatSchedule {
    uint16_t startTimeMinutes;  // eg 60 = 1 am
    uint8_t periodDays;  // eg 3 = run every 3rd day
    TimeStamp origin;
    uint8_t sequenceIdx;
  };

  // resizeXXX = create the given number of entries (ValveSequences, WeeklySchedules, DailySchedules).
  //   Initialises the new ones to default (empty).
  //   Preserves the existing contents.
  // If Schedule entries refer to ValveSequences which are removed, the schedule will run an empty sequence.

  SuccessCode resizeValveSequencesArray(uint8_t newCount);
  ValveSequence &getValveSequence(uint8_t index);
  void deleteValveSequence(uint8_t index);

  SuccessCode resizeWeeklySchedulesArray(uint8_t newCount);
  WeeklySchedule &getWeeklySchedule(uint8_t index);
  void deleteWeeklySchedule(uint8_t index);

  SuccessCode resizeDailySchedulesArray(uint8_t newCount);
  DailyRepeatSchedule &getDailySchedule(uint8_t index);
  void deleteDailySchedule(uint8_t index);

  void deleteOrphanSequences();

  void tick(TimeStamp timenow);

private:
  SharedMemoryArray m_sma_sequences;  // dynamically allocated space for the array of sequences
  SharedMemoryArray m_sma_weeklySchedules; // dynamically allocated data for the array of weekly schedules
  SharedMemoryArray m_sma_dailySchedules; // dynamically allocated data for the array of daily schedules

  static WeeklySchedule s_dummyWeeklySchedule;
  static DailyRepeatSchedule s_dummyDailyRepeatSchedule;
  static ValveSequence s_dummyValveSequence;
};

#endif //WATERINGSYSTEMTESTHARNESS_SCHEDULER_H
