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

int main(void)
{
  char buf[16];

  FORK_CHILD

  usleep(100000); // delay the client
  bufio_stream *output = bufio_open("tcp://connect/12345/localhost", "w", 1000, 0, "bufio_test_tcp_serve: connect");
  assert(output != NULL);

  // Transmit 4 bytes, flush & close
  assert(bufio_write(output, buf, 4) == 4);
  assert(bufio_close(output) == 0);

  FORK_PARENT
  bufio_stream *input = bufio_open("tcp://serve/12345/localhost", "r", 0, 0, "bufio_test_tcp_serve: serve");
  assert(input != NULL);

  assert(bufio_wait(input, 200) == 1);

  // 4 bytes available
  assert(bufio_read(input, buf, 4) == 4);
  assert(bufio_close(input) == 0);

  FORK_JOIN

  return 0;
}
