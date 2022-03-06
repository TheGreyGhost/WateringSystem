#ifndef ERROR_STATUS_H
#define ERROR_STATUS_H
#include "Arduino.h"

// ErrorStatus is used to return status information from a function instead of using exceptions.
// Typical usage:
// 1a) ErrorStatus es = new returnValue(ErrorStatus::OK);
// 1b) ErrorStatus es = new returnValue(ErrorStatus::OK, "No error occurred");
// 2) return es;
// 3) if (es.isError()) {
//      cout << es.getDescription();  // eg prints "OK" 
//      cout << es.getExtraInfo();  // eg prints "No error occurred" 
//      es.print(console);

class ErrorStatus {
   public:
    enum class ErrorCode {OK, TIMEOUT};

    ErrorStatus(ErrorCode errorcode);
    ErrorStatus(ErrorCode errorcode, const String &extrainfo);

    boolean isError();    
    String getDescription();
    String getExtraInfo();
    void print(Print *errorconsole);

  private:
    ErrorCode m_errorcode;
    String m_extraInfo;
};

#endif
