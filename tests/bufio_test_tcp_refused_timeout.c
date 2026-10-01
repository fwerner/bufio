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

static long elapsed_ms(const struct timespec *a, const struct timespec *b)
{
  return (b->tv_sec - a->tv_sec) * 1000
       + (b->tv_nsec - a->tv_nsec) / 1000000;
}

// A refused connection must respect the timeout overall. Note the limits of this check:
// on loopback a refusal comes back almost instantly, so the per-retry is tiny here and a
// tight bound would be flaky on loaded CI. Strict per-retry accounting is enforced by
// construction via a single monotonic deadline (see bufio.c); reproducing real network
// round-trip delay would need tc netem and root. This test therefore guards gross
// overruns and that the retry loop waits out (roughly) the whole timeout.
int main(void)
{
  int port = test_free_loopback_port();
  char peer[64];
  snprintf(peer, sizeof(peer), "tcp://connect/%d/127.0.0.1", port);

  int timeout = 500;
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *s = bufio_open(peer, "r", timeout, 0, "bufio_test_tcp_refused_timeout");
  clock_gettime(CLOCK_MONOTONIC, &end);

  long ms = elapsed_ms(&start, &end);
  fprintf(stderr, "refused: timeout=%d elapsed=%ld ms peer=%s\n", timeout, ms, peer);
  assert(s == NULL);
  // Must wait out (roughly) the whole timeout, but not overrun it grossly.
  // Bounds are generous for loaded CI (see comment above).
  assert(ms >= timeout - 150);
  assert(ms <= timeout * 2);

  return 0;
}
