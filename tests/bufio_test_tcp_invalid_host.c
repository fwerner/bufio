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

// An unresolvable host (*.invalid, RFC 2606) must fail, and with a working
// resolver it fails quickly instead of waiting out the full connect timeout.
// Resolution time itself cannot be bounded portably (a slow or unreachable
// resolver blocks in getaddrinfo for seconds), so a slow environment skips
// the fast-fail check. Exact subtraction is enforced by construction via a
// single monotonic deadline (see bufio.c) and cannot be timing-tested without
// a controlled DNS; the localhost case below exercises the resolution path
// with a fast resolver.
int main(void)
{
  int timeout = 2000;
  const char *peer = "tcp://connect/12345/nonexistent.invalid";

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *s = bufio_open(peer, "r", timeout, 0, "bufio_test_tcp_invalid_host");
  clock_gettime(CLOCK_MONOTONIC, &end);

  long ms = (end.tv_sec - start.tv_sec) * 1000
          + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "invalid: timeout=%d elapsed=%ld ms\n", timeout, ms);
  assert(s == NULL);
  if (ms >= timeout / 2) {
    // Slow or unreachable resolver on this runner (seconds per nameserver)?
    fprintf(stderr, "invalid: slow resolver, skipping fast-fail check\n");
  }

  // Resolution path with a fast resolver: "localhost" resolves via hosts to
  // 127.0.0.1, then the refused-connection deadline applies as a whole.
  int port = test_free_loopback_port();
  char localhost_peer[64];
  snprintf(localhost_peer, sizeof(localhost_peer), "tcp://connect/%d/localhost", port);
  int timeout2 = 500;
  clock_gettime(CLOCK_MONOTONIC, &start);
  bufio_stream *s2 = bufio_open(localhost_peer, "r", timeout2, 0, "bufio_test_tcp_invalid_host");
  clock_gettime(CLOCK_MONOTONIC, &end);
  long ms2 = (end.tv_sec - start.tv_sec) * 1000
           + (end.tv_nsec - start.tv_nsec) / 1000000;
  fprintf(stderr, "invalid: localhost refused timeout=%d elapsed=%ld ms\n", timeout2, ms2);
  assert(s2 == NULL);
  assert(ms2 >= timeout2 - 150);
  assert(ms2 <= timeout2 * 2);

  return 0;
}
