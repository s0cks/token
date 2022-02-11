#ifndef TOKEN_TIMESTAMP_H
#define TOKEN_TIMESTAMP_H

#include <chrono>
#include <sstream>
#include <iomanip>

namespace token{
  typedef uint64_t RawTimestamp;

  typedef std::chrono::system_clock Clock;
  typedef Clock::time_point Timestamp;
  typedef Clock::duration Duration;

  static inline uint64_t
  GetElapsedTimeMilliseconds(const Timestamp& start, const Timestamp& stop=Clock::now()){
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  }

  static inline RawTimestamp
  ToUnixTimestamp(const Timestamp& ts=Clock::now()){
    using namespace std::chrono;
    return duration_cast<milliseconds>(ts.time_since_epoch()).count();
  }

  static inline Timestamp
  FromUnixTimestamp(const RawTimestamp& ms){
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
  FormatTimestampElastic(const Timestamp& ts, const std::string& format="%Y-%m-%dT%H:%M:%S"){
    return FormatTimestamp(ts, format);
  }

  static inline std::string
  FormatTimestampFileSafe(const Timestamp& ts, const std::string& format="%Y%m%d-%H%M%S"){
    return FormatTimestamp(ts, format);
  }
}

#endif//TOKEN_TIMESTAMP_H