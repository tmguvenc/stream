#include "utils.h"

#define SEC_TO_MSEC  1e3
#define MSEC_TO_NSEC 1e6

void convert_to_ts(const int64_t millisec, struct timespec* ts)
{
  ts->tv_sec = millisec / SEC_TO_MSEC;
  ts->tv_nsec =
      (millisec - (millisec / SEC_TO_MSEC) * SEC_TO_MSEC) * MSEC_TO_NSEC;
}
