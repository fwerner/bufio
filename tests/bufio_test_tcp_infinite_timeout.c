#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"

// timeout -1 waits indefinitely: a client started before its (delayed) server
// must still connect. This exercises the no-deadline path (indefinite poll
// and sleep without decrementing). Port 12347 is reserved for this test;
// other TCP tests use 12345/12346.
int main(void)
{
  char buf[16];

  FORK_CHILD

  // No deadline: retries until the delayed server listens below.
  bufio_stream *output = bufio_open("tcp://connect/12347/localhost", "w", -1, 0, "bufio_test_tcp_infinite_timeout");
  assert(output != NULL);

  // Transmit 4 bytes, flush & close
  assert(bufio_write(output, buf, 4) == 4);
  assert(bufio_close(output) == 0);

  FORK_PARENT
  usleep(200000); // delay the server
  bufio_stream *input = bufio_open("tcp://listen/12347/localhost", "r", 10000, 0, "bufio_test_tcp_infinite_timeout");
  assert(input != NULL);

  // 4 bytes available
  assert(bufio_wait(input, 1000) == 1);
  assert(bufio_read(input, buf, 4) == 4);
  assert(bufio_close(input) == 0);

  FORK_JOIN

  return 0;
}
