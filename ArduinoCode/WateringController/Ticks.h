#ifndef WATERINGSYSTEMTESTHARNESS_TICKS_H
#define WATERINGSYSTEMTESTHARNESS_TICKS_H
#include <Arduino.h>

class Ticks {
public:
  Ticks() : m_milliseconds(0){};

  void updateFromInternalClock() {m_milliseconds = millis();}
  
  bool operator<(const Ticks &rhs) const {
    return m_milliseconds < rhs.m_milliseconds;
  }

  bool operator>(const Ticks &rhs) const {
    return rhs < *this;
  }

  bool operator<=(const Ticks &rhs) const {
    return !(rhs < *this);
  }

  bool operator>=(const Ticks &rhs) const {
    return !(*this < rhs);
  }

  bool operator==(const Ticks &rhs) const {
    return m_milliseconds == rhs.m_milliseconds;
  }

  bool operator!=(const Ticks &rhs) const {
    return !(rhs == *this);
  }

    // returns the difference between the two timestamps, in milliseconds
  long operator-(const Ticks &rhs) const {
    return (m_milliseconds - rhs.m_milliseconds);
  }

  Ticks operator+(long milliseconds) const {
    Ticks retval(*this);
    retval.m_milliseconds += milliseconds;
    return retval;
  }

  Ticks operator-(long milliseconds) const {
    Ticks retval(*this);
    retval.m_milliseconds -= milliseconds;
    return retval;
  }

  Ticks& operator+=(long milliseconds)  {
    m_milliseconds += milliseconds;
    return *this;
  }

  Ticks& operator-=(long milliseconds)  {
    m_milliseconds -= milliseconds;
    return *this;
  }
millis
private:
  uint32_t m_milliseconds;  // seconds since Jan 1 2020 00:00:00 UTC+0:00
};
#endif //WATERINGSYSTEMTESTHARNESS_TICKS_H
