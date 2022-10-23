#ifndef LIB_STREAM_MANAGER_H_
#define LIB_STREAM_MANAGER_H_

#include "stream.h"

typedef struct stream_manager
{
  stream_t** streams;
  unsigned int max_stream_cnt;
  unsigned int curr_cnt;
} stream_manager_t;

stream_manager_t* stream_manager_create(unsigned int max_stream_cnt);
int stream_manager_add_stream(stream_manager_t* manager, stream_t* stream);

int stream_manager_run(const stream_manager_t* manager);

int stream_manager_destroy(stream_manager_t* manager);

#endif  //! LIB_STREAM_MANAGER_H_
