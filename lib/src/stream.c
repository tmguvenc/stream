#include "stream.h"
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <time.h>
#include "utils.h"

stream_t* stream_create(const stream_params_t* params)
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

int stream_arm_timer(stream_t* stream)
{
  if (!stream)
  {
    fprintf(stderr, "invalid stream object\n");
    return -1;
  }

  struct timespec spec;
  struct timespec now;

  convert_to_ts(stream->timeout_ms, &spec);

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

  if (timerfd_settime(stream->timer_fd, TFD_TIMER_ABSTIME, &new_value, NULL) ==
      -1)
  {
    fprintf(stderr, "cannot arm timer: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int stream_disarm_timer(stream_t* stream)
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

  if (timerfd_settime(stream->timer_fd, TFD_TIMER_ABSTIME, &new_value, NULL) ==
      -1)
  {
    fprintf(stderr, "cannot disarm timer: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int stream_destroy(stream_t* stream)
{
  if (!stream)
  {
    fprintf(stderr, "invalid stream object\n");
    return -1;
  }

  if (stream->fd != -1)
    close(stream->fd);

  if (stream->timer_fd != -1)
    close(stream->timer_fd);

  free(stream);
  return 0;
}
