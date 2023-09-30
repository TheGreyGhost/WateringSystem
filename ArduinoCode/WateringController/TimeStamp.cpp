#include "TimeStamp.h"
#include "SystemStatus.h"

const uint8_t days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

TimeStamp::TimeStamp(int year, int month, int dayofmonth, int hours, int minutes, int seconds, float timezone) {
  if (month<1 || month > 12 || dayofmonth < 1 || dayofmonth > 31 || 
      hours < 0 or hours >= 24 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60 || year < 2020) {
    assertFailureCode = ASSERT_PARAMETER_OUT_OF_RANGE;
    return;	
  }	
    int i;
    int d;
    int y;

    y = year - 2020;

    d = dayofmonth - 1;
    for (i=1; i<month; i++) {
        d += days_in_month[i - 1];
    }
    if (month > 2 && y % 4 == 0) {
        d++;
    }
    // count leap days
    d += (365 * y + (y + 3) / 4);
    m_secondsSinceZero = ((d * 24UL + hours) * 60 + minutes) * 60 + seconds - (int)(timezone * 60L * 60L);
}

//uint8_t TimeStamp::getWeekDayLocalTime() const {
//  uint32_t localSecondsSinceZero = m_secondsSinceZero + g_timezoneMinutes;
//  uint32_t daysSince01012020 = localSecondsSinceZero / 60 / 60 / 24;
//  uint8_t retval = (uint8_t)(daysSince01012020 - 3) % 7;  // 01 Jan 2020 was a Wednesday
//  return retval;
//}
//
//uint16_t TimeStamp::getMinuteOfTheDayLocalTime() const {
//  uint32_t localSecondsSinceZero = m_secondsSinceZero + g_timezoneMinutes;
//  uint32_t secondsToday = (localSecondsSinceZero) % (24 * 60 * 60);
//  return (uint16_t)secondsToday;
//}

TimeStamp TimeStamp::getNextWeeklyScheduleTimepoint(uint8_t dayOfTheWeek, uint16_t minuteOfTheDay) {
// find the 'origin' corresponding to the dayOfTheWeek and minute of the day
//  round up to next multiple
//  01 Jan 2020 (0 seconds m_secondSinceZero) was a Wednesday so the origin is 4 days later
  uint32_t originDaySeconds = (dayOfTheWeek + 4) * 60L * 60L * 24L;
  uint32_t originLocalSecondsSinceZero = originDaySeconds + minuteOfTheDay + g_timezoneMinutes;
  uint32_t offsetSeconds = (m_secondsSinceZero - originLocalSecondsSinceZero) % (7L * 24L * 60L * 60L);
  return *this + (7L * 24L * 60L * 60L - offsetSeconds);
}

TimeStamp TimeStamp::getNextDailyRepeatScheduleTimepoint(TimeStamp origin, uint8_t periodInDays, uint16_t minuteOfTheDay) {
// find the 'origin' corresponding to the day period and minute of the day
//  round up to next multiple
  uint32_t originDaySeconds = origin.m_secondsSinceZero - (origin.m_secondsSinceZero) % (60L * 60L * 24L);
  uint32_t originLocalSecondsSinceZero = originDaySeconds + minuteOfTheDay + g_timezoneMinutes;
  uint32_t offsetSeconds = (m_secondsSinceZero - originLocalSecondsSinceZero)
                           % ((uint32_t)periodInDays * 24L * 60L * 60L);
  return *this + ((uint32_t)periodInDays * 24L * 60L * 60L  - offsetSeconds);
}

int16_t g_timezoneMinutes = 0;
