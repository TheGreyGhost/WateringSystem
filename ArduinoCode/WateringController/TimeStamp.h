#ifndef WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
#define WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
#include <Arduino.h>
#include "stdint.h"

// Use to represent a date+time
// Only valid for dates >= 1 jan 2020

class TimeStamp {
public:

  // creates a timestamp for the given date and time
  // year must be >= 2020
  // hours, minutes, seconds is the local time, not UTC
  // timezone is in hours eg +1.0 is Vienna UTC+01:00
  TimeStamp(int year, int month, int dayofmonth, int hours, int minutes, int seconds, float timezone);

  TimeStamp() : m_secondsSinceZero(0){};

  bool operator<(const TimeStamp &rhs) const {
    return m_secondsSinceZero < rhs.m_secondsSinceZero;
  }

  bool operator>(const TimeStamp &rhs) const {
    return rhs < *this;
  }

  bool operator<=(const TimeStamp &rhs) const {
    return !(rhs < *this);
  }

  bool operator>=(const TimeStamp &rhs) const {
    return !(*this < rhs);
  }

  bool operator==(const TimeStamp &rhs) const {
    return m_secondsSinceZero == rhs.m_secondsSinceZero;
  }

  bool operator!=(const TimeStamp &rhs) const {
    return !(rhs == *this);
  }

    // returns the difference between the two timestamps, in seconds
  long operator-(const TimeStamp &rhs) const {
    return (m_secondsSinceZero - rhs.m_secondsSinceZero <= INT32_MAX)
            ? (long)(m_secondsSinceZero - rhs.m_secondsSinceZero ) : -(long)(rhs.m_secondsSinceZero - m_secondsSinceZero);
  }

  TimeStamp operator+(long seconds) const {
    TimeStamp retval(*this);
    retval.m_secondsSinceZero += seconds;
    return retval;
  }

  TimeStamp operator-(long seconds) const {
    TimeStamp retval(*this);
    retval.m_secondsSinceZero -= seconds;
    return retval;
  }

  TimeStamp& operator+=(long seconds)  {
    m_secondsSinceZero += seconds;
    return *this;
  }

  TimeStamp& operator-=(long seconds)  {
    m_secondsSinceZero -= seconds;
    return *this;
  }

//  // return the day of the week.  0 = Sunday
//  uint8_t getWeekDayLocalTime() const;
//
//  // return the minute of the day eg 1am = 60.
//  uint16_t getMinuteOfTheDayLocalTime() const;

  // returns the time of the next scheduled timepoint, for a weekly schedule
  // i.e. at a given minute of a day of the week
  // uses local time based on the timezone in g_timezoneMinutes
  TimeStamp getNextWeeklyScheduleTimepoint(uint8_t dayOfTheWeek, uint16_t minuteOfTheDay);

  // returns the time of the next scheduled timepoint, for a every-N-days schedule
  // i.e. at a given minute of the day, repeated every N days
  // uses local time based on the timezone in g_timezoneMinutes
  TimeStamp getNextDailyRepeatScheduleTimepoint(TimeStamp origin, uint8_t periodInDays, uint16_t minuteOfTheDay);

private:
  uint32_t m_secondsSinceZero;  // seconds since Jan 1 2020 00:00:00 UTC+0:00
};

extern int16_t g_timezoneMinutes; // the local timezone, in minutes.  eg +9:30 = 9*60 + 30

#endif //WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
