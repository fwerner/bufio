#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"


// Opening "-" sets O_NONBLOCK on fd 0/1 process-wide. The stream operates on a
// duplicate, so bufio_close leaves the standard streams themselves open, and
// restores the original flags. Both are asserted below.
int main(void)
{
  const char testname[] = "bufio_test_stdio_flags";

  // --- stdin ---
  int p[2];
  assert(pipe(p) == 0);
  int orig = fcntl(p[0], F_GETFL);
  assert(orig != -1);

  assert(dup2(p[0], STDIN_FILENO) == STDIN_FILENO);

  bufio_stream *si = bufio_open("-", "r", 100, 256, testname);
  assert(si != NULL);
  assert((fcntl(STDIN_FILENO, F_GETFL) & O_NONBLOCK) != 0);

  assert(bufio_close(si) == 0);
  // The standard stream itself survives, with flags restored
  assert(fcntl(STDIN_FILENO, F_GETFL) == orig);
  assert(fcntl(p[0], F_GETFL) == orig);

  // Second open/close cycle (guards the saved-flags reset)
  assert(dup2(p[0], STDIN_FILENO) == STDIN_FILENO);
  si = bufio_open("-", "r", 100, 256, testname);
  assert(si != NULL);
  assert(bufio_close(si) == 0);
  assert(fcntl(STDIN_FILENO, F_GETFL) == orig);
  assert(fcntl(p[0], F_GETFL) == orig);

  assert(close(p[0]) == 0);
  assert(close(p[1]) == 0);

  // --- stdout ---
  int q[2];
  assert(pipe(q) == 0);
  orig = fcntl(q[1], F_GETFL);
  assert(orig != -1);

  assert(dup2(q[1], STDOUT_FILENO) == STDOUT_FILENO);

  bufio_stream *so = bufio_open("-", "w", 100, 256, testname);
  assert(so != NULL);
  assert((fcntl(STDOUT_FILENO, F_GETFL) & O_NONBLOCK) != 0);

  assert(bufio_close(so) == 0);
  assert(fcntl(STDOUT_FILENO, F_GETFL) == orig);
  assert(fcntl(q[1], F_GETFL) == orig);

  assert(close(q[0]) == 0);
  assert(close(q[1]) == 0);

  return 0;
}
