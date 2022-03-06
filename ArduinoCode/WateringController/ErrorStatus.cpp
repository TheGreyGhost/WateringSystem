#include "ErrorStatus.h"

// ErrorStatus is used to return status information from a function instead of using exceptions.
// Typical usage:
// 1a) ErrorStatus es = new returnValue(ErrorStatus::OK);
// 1b) ErrorStatus es = new returnValue(ErrorStatus::OK, "No error occurred");
// 2) return es;
// 3) if (es.isError()) {
//      cout << es.getDescription();  // eg prints "OK" 
//      cout << es.getExtraInfo();  // eg prints "No error occurred" 
//      es.print(console);

ErrorStatus::ErrorStatus(ErrorCode errorcode) {
  m_errorcode = errorcode;
  m_extraInfo = "";
}

ErrorStatus::ErrorStatus(ErrorCode errorcode, const String &extrainfo) {
  m_errorcode = errorcode;
  m_extraInfo = extrainfo;
}

bool ErrorStatus::isError() {
  return (m_errorcode != ErrorCode::OK);  
}
    
String ErrorStatus::getDescription() {
  switch (m_errorcode) {
    case ErrorCode::OK: return "OK";
    case ErrorCode::TIMEOUT: return "Timeout";
  }
  return "Unknown (internal error)";
}

String ErrorStatus::getExtraInfo() {
  return m_extraInfo;
}

void ErrorStatus::print(Print *errorconsole) {
  if (errorconsole == nullptr) return;
  errorconsole->print(getDescription());
  if (m_extraInfo.length() > 0) {
    errorconsole->print("[");
    errorconsole->print(m_extraInfo);
    errorconsole->print("]");
  }
}
  
