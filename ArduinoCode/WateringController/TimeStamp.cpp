#include "TimeStamp.h"
#include "SystemStatus.h"
const uint8_t days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

TimeStamp::TimeStamp(int year, int month, int dayofmonth, int hours, int minutes, int seconds, float timezone) {
  if (month<1 || month > 12 || dayofmonth < 1 || dayofmonth > 31 || 
      hours < 0 or hours >= 24 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60) {
    assertFailureCode = ASSERT_PARAMETER_OUT_OF_RANGE;
    return;	
  }	
    int i;
    int d;
    int y;

    if (year >= 2021) {
      y = year - 2020;
    } else {
      return;
    }

    d = dayofmonth - 1;
    for (i=1; i<month; i++) {
        d += days_in_month[i - 1];
    }
    if (month > 2 && y % 4 == 0) {
        d++;
    }
    // count leap days
    d += (365 * y + (y + 3) / 4);
    m_secondsSinceZero = ((d * 24UL + hours) * 60 + minutes) * 60 + seconds + (int)(timezone * 60 * 60);
}





