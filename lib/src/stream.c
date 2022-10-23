#include "stream.h"
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <time.h>
#include "utils.h"

typedef struct stream
{
  int fd;
  int timer_fd;
  int timeout_ms;
  on_event_f callback;
} stream_t;

void* stream_create(const stream_params_t* params)
{
  if (!params)
  {
    fprintf(stderr, "invalid stream param object\n");
    return NULL;
  }

  stream_t* stream = calloc(1, sizeof(stream_t));
  if (!stream)
  {
    fprintf(stderr, "cannot create stream: [%s]\n", strerror(errno));
    return NULL;
  }

  stream->fd = open(params->path, O_RDWR | O_NOCTTY);
  if (stream->fd == -1)
  {
    fprintf(stderr, "cannot open %s: [%s]\n", params->path, strerror(errno));
    goto err;
  }

  stream->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (stream->timer_fd == -1)
  {
    fprintf(stderr, "cannot create timer: [%s]\n", strerror(errno));
    goto err;
  }

  stream->callback = params->callback;

  return stream;

err:
  stream_destroy(stream);
  return NULL;
}

int stream_arm_timer(void* stream)
{
  if (!stream)
  {
    fprintf(stderr, "invalid stream object\n");
    return -1;
  }

  struct timespec spec;
  struct timespec now;

  convert_to_ts(stream_get_timeout_ms(stream), &spec);

  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
  {
    fprintf(stderr, "stream arm failed: [%s]\n", strerror(errno));
    return -1;
  }

  // clang-format off
  struct itimerspec new_value = {
    // expiration interval
    .it_interval = {
      .tv_sec = spec.tv_sec,
      .tv_nsec = spec.tv_nsec
    },

    // initial expiration time
    .it_value = {
      .tv_sec = now.tv_sec + spec.tv_sec,
      .tv_nsec = now.tv_nsec + spec.tv_nsec
    }
  };
  // clang-format on

  if (timerfd_settime(stream_get_timer_fd(stream), TFD_TIMER_ABSTIME,
                      &new_value, NULL) == -1)
  {
    fprintf(stderr, "cannot arm timer: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int stream_disarm_timer(void* stream)
{
  if (!stream)
  {
    fprintf(stderr, "invalid stream object\n");
    return -1;
  }

  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
  {
    fprintf(stderr, "stream arm failed: [%s]\n", strerror(errno));
    return -1;
  }

  struct itimerspec new_value = {};

  if (timerfd_settime(stream_get_timer_fd(stream), TFD_TIMER_ABSTIME,
                      &new_value, NULL) == -1)
  {
    fprintf(stderr, "cannot disarm timer: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int stream_get_timeout_ms(void* stream) { ((stream_t*)stream)->timeout_ms; }

int stream_get_fd(void* stream) { return ((stream_t*)stream)->fd; }

int stream_get_timer_fd(void* stream) { return ((stream_t*)stream)->timer_fd; }

int stream_destroy(void* stream)
{
  if (!stream)
  {
    fprintf(stderr, "invalid stream object\n");
    return -1;
  }

  stream_t* str = (stream_t*)stream;

  if (str->fd != -1)
    close(str->fd);

  if (str->timer_fd != -1)
    close(str->timer_fd);

  free(str);
  return 0;
}
