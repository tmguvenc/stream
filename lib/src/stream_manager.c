#include "stream_manager.h"
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdatomic.h>
#include "stream.h"

typedef struct stream_manager
{
  stream_t** streams;
  unsigned int max_stream_cnt;
  unsigned int curr_cnt;
  atomic_bool running;
} stream_manager_t;

stream_manager_t* stream_manager_create(unsigned int max_stream_cnt)
{
  stream_manager_t* manager = calloc(1, sizeof(stream_manager_t));
  if (!manager)
  {
    fprintf(stderr, "cannot create stream manager: [%s]\n", strerror(errno));
    return NULL;
  }

  manager->streams = calloc(max_stream_cnt, sizeof(stream_t*));
  if (!manager->streams)
  {
    fprintf(stderr, "cannot create streams: [%s]\n", strerror(errno));
    stream_manager_destroy(manager);
    return NULL;
  }

  manager->max_stream_cnt = max_stream_cnt;
  atomic_store(&manager->running, 0);

  return manager;
}

int stream_manager_add_stream(stream_manager_t* manager, stream_t* stream)
{
  if (!manager)
  {
    fprintf(stderr, "invalid stream manager\n");
    return -1;
  }

  if (!stream)
  {
    fprintf(stderr, "invalid stream\n");
    return -1;
  }

  if (manager->curr_cnt == (manager->max_stream_cnt - 1))
  {
    fprintf(stderr, "maximum number of streams has already been added.\n");
    return -1;
  }

  manager->streams[manager->curr_cnt] = stream;
  manager->curr_cnt += 1;

  return 0;
}

static int epoll_ctl_add(int epfd, int fd, uint32_t events)
{
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
  {
    fprintf(stderr, "epoll_ctl error: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

static int create_fd_list(const stream_manager_t* manager)
{
  int epfd = epoll_create(1);
  unsigned int idx;
  for (idx = 0; idx < manager->curr_cnt; ++idx)
  {
    const stream_t* stream = manager->streams[idx];
    if (epoll_ctl_add(epfd, stream->fd, EPOLLIN) == -1)
      goto fd_list_err;

    if (epoll_ctl_add(epfd, stream->timer_fd, EPOLLIN) == -1)
      goto fd_list_err;
  }

  return epfd;

fd_list_err:
  close(epfd);
  return -1;
}

static stream_t* get_stream(const stream_manager_t* manager, int fd)
{
  unsigned int idx;
  for (idx = 0; idx < manager->curr_cnt; ++idx)
  {
    if (manager->streams[idx]->fd == fd ||
        manager->streams[idx]->timer_fd == fd)
      return manager->streams[idx];
  }

  fprintf(stderr, "cannot find stream with fd: [%d]\n", fd);
  return NULL;
}

int stream_manager_run(stream_manager_t* manager)
{
  if (!manager)
  {
    fprintf(stderr, "invalid stream manager\n");
    return -1;
  }

  int epfd = create_fd_list(manager);
  if (epfd == -1)
    return -1;

  struct epoll_event* events =
      calloc(manager->curr_cnt, sizeof(struct epoll_event));
  if (!events)
  {
    close(epfd);
    fprintf(stderr, "cannot create event list: [%s]\n", strerror(errno));
    return -1;
  }

  char buffer[4 * 1024];
  int nfds, i;

  atomic_store(&manager->running, 1);

  while (atomic_load(&manager->running))
  {
    nfds = epoll_wait(epfd, events, manager->curr_cnt, -1);
    if (nfds == -1)
    {
      fprintf(stderr, "poll error: [%s]\n", strerror(errno));
      break;
    }

    for (i = 0; i < nfds; i++)
    {
      if (events[i].events & EPOLLIN)
      {
        const int fd = events[i].data.fd;
        stream_t* stream = get_stream(manager, fd);
        if (!stream)
        {
          fprintf(stderr, "stream not found\n");
          continue;
        }

        const int ret = read(fd, buffer, sizeof(buffer));
        if (ret == -1)
        {
          fprintf(stderr, "read error: [%s]\n", strerror(errno));
          continue;
        }

        const stream_evt_type_t evt_type =
            (fd == stream->fd) ? STREAM_EVT_RECV : STREAM_EVT_TIMEOUT;

        stream_disarm_timer(stream);
        stream->callback(evt_type, buffer, ret);
        stream_arm_timer(stream);
      }
    }
  }

  free(events);
  close(epfd);

  return 0;
}

int stream_manager_stop(stream_manager_t* manager)
{
  if (!manager)
  {
    fprintf(stderr, "invalid stream manager\n");
    return -1;
  }

  atomic_store(&manager->running, 0);
  return 0;
}

int stream_manager_destroy(stream_manager_t* manager)
{
  if (!manager)
  {
    fprintf(stderr, "invalid stream manager\n");
    return -1;
  }

  if (manager->streams)
    free(manager->streams);
  free(manager);

  return 0;
}
