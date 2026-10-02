#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bufio.h"
#include "test.h"

// timeout 0 on TCP is raised to the 100 ms minimum, because a connect cannot
// complete in zero time (previously it attempted a single blocking connect).
// No fork or sleep is needed for the listening case: the test holds a backlog
// listener itself (raw socket/bind/listen, never accept) and the kernel
// completes the loopback handshake from the backlog. The 100 ms window also
// absorbs async loopback completion (e.g. macOS), so success here is
// deterministic, unlike a true zero-time poll.
int main(void)
{
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  assert(listen_fd != -1);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  assert(bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr)) == 0);
  assert(listen(listen_fd, 5) == 0);

  socklen_t len = sizeof(addr);
  assert(getsockname(listen_fd, (struct sockaddr *) &addr, &len) == 0);
  int port = ntohs(addr.sin_port);

  char connect_peer[64];
  snprintf(connect_peer, sizeof(connect_peer), "tcp://connect/%d/127.0.0.1", port);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *ok = bufio_open(connect_peer, "r", 0, 0, "bufio_test_tcp_zero_timeout");
  clock_gettime(CLOCK_MONOTONIC, &end);
  long ms_ok = (end.tv_sec - start.tv_sec) * 1000
             + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "zero-timeout to listening port: elapsed=%ld ms\n", ms_ok);
  assert(ok != NULL);
  assert(bufio_close(ok) == 0);
  close(listen_fd);

  // Closed port: 0 is raised to 100 ms, so the refused retries wait out about
  // 100 ms. The old single blocking attempt failed in ~0 ms.
  int closed_port = test_free_loopback_port();
  if (closed_port == port)
    closed_port = test_free_loopback_port();

  char refused_peer[64];
  snprintf(refused_peer, sizeof(refused_peer), "tcp://connect/%d/127.0.0.1", closed_port);
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *fail = bufio_open(refused_peer, "r", 0, 0, "bufio_test_tcp_zero_timeout");
  clock_gettime(CLOCK_MONOTONIC, &end);
  long ms_fail = (end.tv_sec - start.tv_sec) * 1000
               + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "zero-timeout to closed port: elapsed=%ld ms\n", ms_fail);
  assert(fail == NULL);
  assert(ms_fail >= 50);
  assert(ms_fail <= 500);

  return 0;
}
