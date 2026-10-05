#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bufio.h"
#include "test.h"

// A listen with nobody connecting must respect the timeout. This exercises
// the deadline-based accept_timeout, which no other default-suite test hits
// (listens there always get a client).
int main(void)
{
  int port = test_free_loopback_port();
  char peer[64];
  snprintf(peer, sizeof(peer), "tcp://listen/%d/127.0.0.1", port);

  int timeout = 500;
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *s = bufio_open(peer, "r", timeout, 0, "bufio_test_tcp_listen_timeout");
  clock_gettime(CLOCK_MONOTONIC, &end);

  long ms = (end.tv_sec - start.tv_sec) * 1000
          + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "listen: timeout=%d elapsed=%ld ms peer=%s\n", timeout, ms, peer);
  assert(s == NULL);
  // Must wait out (roughly) the whole timeout, but not overrun it grossly.
  // Bounds are generous for loaded CI.
  assert(ms >= timeout - 150);
  assert(ms <= timeout * 2);

  return 0;
}
