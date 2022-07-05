#ifndef WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
#define WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
#include <Arduino.h>
//#include <cstdint>
//
//struct ts {
//    uint8_t sec;         /* seconds */
//    uint8_t min;         /* minutes */
//    uint8_t hour;        /* hours */
//    uint8_t mday;        /* day of the month */
//    uint8_t mon;         /* month */
//    int16_t year;        /* year */
//    uint8_t wday;        /* day of the week */
//    uint8_t yday;        /* day in the year */
//    uint8_t isdst;       /* daylight saving time */
//    uint8_t year_s;      /* year in short notation*/
//#ifdef CONFIG_UNIXTIME
//    uint32_t unixtime;   /* seconds since 01.01.1970 00:00:00 UTC*/
//#endif
//};

class TimeStamp {
public:
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
    return (m_secondsSinceZero - rhs.m_secondsSinceZero);
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

private:
  uint32_t m_secondsSinceZero;  // seconds since Jan 1 2020 00:00:00 UTC+0:00
};
#endif //WATERINGSYSTEMTESTHARNESS_TIMESTAMP_H
