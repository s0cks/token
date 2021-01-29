#ifndef TOKEN_TIMESTAMP_H
#define TOKEN_TIMESTAMP_H

#include <chrono>

namespace Token{
  typedef std::chrono::system_clock Clock;
  typedef Clock::time_point Timestamp;
  typedef Clock::duration Duration;

  static inline int64_t
  ToUnixTimestamp(const Timestamp& ts=Clock::now()){
    using namespace std::chrono;
    return duration_cast<milliseconds>(ts.time_since_epoch()).count();
  }

  static inline Timestamp
  FromUnixTimestamp(const int64_t& ms){
    using namespace std::chrono;
    return Timestamp(milliseconds(ms));
  }

  static inline std::string
  FormatTimestamp(const Timestamp& ts, const std::string& format){
    std::time_t tt = Clock::to_time_t(ts);
    std::tm tm = *std::gmtime(&tt);
    std::stringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
  }

  static inline std::string
  FormatTimestampReadable(const Timestamp& ts, const std::string& format="%m/%d/%Y %H:%M:%S"){
    return FormatTimestamp(ts, format);
  }

  static inline std::string
  FormatTimestampFileSafe(const Timestamp& ts, const std::string& format="%Y%m%d-%H%M%S"){
    return FormatTimestamp(ts, format);
  }
}

#endif//TOKEN_TIMESTAMP_H