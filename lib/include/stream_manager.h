#ifndef LIB_STREAM_MANAGER_H_
#define LIB_STREAM_MANAGER_H_

typedef struct stream stream_t;
typedef struct stream_manager stream_manager_t;

stream_manager_t* stream_manager_create(unsigned int max_stream_cnt);
int stream_manager_add_stream(stream_manager_t* manager, stream_t* stream);
int stream_manager_run(stream_manager_t* manager);
int stream_manager_stop(stream_manager_t* manager);
int stream_manager_destroy(stream_manager_t* manager);

#endif  //! LIB_STREAM_MANAGER_H_
