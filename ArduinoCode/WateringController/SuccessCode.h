//
// Created by TGG on 28/07/2022.
//

#ifndef WATERINGSYSTEMTESTHARNESS_SUCCESSCODE_H
#define WATERINGSYSTEMTESTHARNESS_SUCCESSCODE_H

enum class ErrorCode{OK=0, ValveOnTimeTooLong=1, InsufficientMemory=2, AssertionFailed=3,
        SequenceDurationExceedsMaximum=4, AlreadyAllocated=5, RequestedCountTooLarge=6};

class SuccessCode {
public:
  SuccessCode(ErrorCode errorCode) : m_errorcode(errorCode) {}
  SuccessCode() : m_errorcode(ErrorCode::OK) {}
  bool succeeded() {return m_errorcode == ErrorCode::OK;}
  ErrorCode getErrorCode() {return m_errorcode;}
  int getErrorID() {return (int)m_errorcode;}
  static SuccessCode success() {return SuccessCode(ErrorCode::OK);}
private:
  ErrorCode m_errorcode;
};

#endif //WATERINGSYSTEMTESTHARNESS_SUCCESSCODE_H
