#ifndef LIB_UTILS_H_
#define LIB_UTILS_H_

#include <stdint.h>
#include <time.h>

void convert_to_ts(const int64_t millisec, struct timespec* ts);

#endif  //! LIB_UTILS_H_
