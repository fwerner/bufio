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
#include <unistd.h>

#include "bufio.h"
#include "test.h"

// SYNs dropped by the network (TEST-NET-1, RFC 5737) must fail within about
// the requested timeout. A blocking connect() would instead stay in SYN_SENT
// until the kernel exhausts its SYN retries (system-dependent, very long).
//
// Environment-dependent: a sandbox or CI proxy may intercept and accept
// connections to TEST-NET-1 (then the connect succeeds for reasons unrelated
// to bufio), or there may be no route at all (then it fails fast with
// ENETUNREACH/EHOSTUNREACH without exercising the timeout). Both cases skip
// the timing check.
int main(void)
{
  int timeout = 300;
  const char *peer = "tcp://connect/12345/192.0.2.1";

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *s = bufio_open(peer, "r", timeout, 0, "bufio_test_tcp_dropped_timeout");
  clock_gettime(CLOCK_MONOTONIC, &end);

  long ms = (end.tv_sec - start.tv_sec) * 1000
          + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "dropped: timeout=%d elapsed=%ld ms\n", timeout, ms);
  if (s != NULL) {
    fprintf(stderr, "dropped: proxy intercepted TEST-NET-1, skipping timing check\n");
    assert(bufio_close(s) == 0);
    return 0;
  }
  if (ms < 100) {
    // No route to TEST-NET-1 on this runner (fast ENETUNREACH/EHOSTUNREACH)
    fprintf(stderr, "dropped: no route, failing fast is fine\n");
    return 0;
  }
  assert(ms <= timeout * 2);

  return 0;
}
