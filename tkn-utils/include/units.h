#ifndef TOKEN_UNITS_H
#define TOKEN_UNITS_H

#include <cmath>

namespace token{
// size constants
#define BYTES 1
#define KILOBYTES (1024 * BYTES)
#define MEGABYTES (1024 * KILOBYTES)
#define GIGABYTES (1024 * MEGABYTES)
#define TERABYTES (1024 * GIGABYTES)

static inline std::string
FormatFileSizeHumanReadable(double size){
  static const double kLN1024 = log(1024);
  static const char* kUnits[] = {
     "B",
     "KB",
     "MB",
     "GB",
     "TB"
  };
  double exp = floor(fmin(log(size)/kLN1024, 3));
  double divisor = pow(1024, exp);
  char buff[16];
  snprintf(buff, 16, "%10.2f %s", floor(size/divisor), kUnits[(int)exp]);
  return std::string(buff);
}

// time constants
#define MILLISECONDS 1
#define SECONDS (1000 * MILLISECONDS)
#define MINUTES (60 * SECONDS)
#define HOURS (60 * MINUTES)
#define DAYS (24 * HOURS)
#define WEEKS (7 * DAYS)
#define YEARS (52 * WEEKS)
}

#endif//TOKEN_UNITS_H