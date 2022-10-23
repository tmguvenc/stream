#ifndef LIB_STREAM_H_
#define LIB_STREAM_H_

#include <stdint.h>

#define MAX_PATH_LEN 128

typedef enum evt_types
{
  STREAM_EVT_RECV = 0,
  STREAM_EVT_TIMEOUT,
} stream_evt_type_t;

typedef int (*on_event_f)(const stream_evt_type_t type, const char* data,
                          const uint32_t data_len);

typedef struct stream_params
{
  char path[MAX_PATH_LEN];
  int timeout_ms;
  on_event_f callback;
} stream_params_t;

void* stream_create(const stream_params_t* params);
int stream_arm_timer(void* stream);
int stream_disarm_timer(void* stream);
int stream_get_timeout_ms(void* stream);
int stream_get_fd(void* stream);
int stream_get_timer_fd(void* stream);
int stream_destroy(void* stream);

#endif  //! LIB_STREAM_H_
