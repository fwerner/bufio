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


// Opening "-" sets O_NONBLOCK on fd 0/1 process-wide. bufio_close must restore
// the original flags. Observable via a dup'd fd sharing the description, since
// fd 0/1 themselves are closed by bufio_close.
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
  // fd 0 is closed now; the surviving dup shows the restored flags
  assert(fcntl(p[0], F_GETFL) == orig);

  // Second open/close cycle (guards the saved-slot reset)
  assert(dup2(p[0], STDIN_FILENO) == STDIN_FILENO);
  si = bufio_open("-", "r", 100, 256, testname);
  assert(si != NULL);
  assert(bufio_close(si) == 0);
  assert(fcntl(p[0], F_GETFL) == orig);

  assert(close(p[0]) == 0);
  assert(close(p[1]) == 0);

  // --- stdout (last: fd 1 stays closed afterwards, the process exits) ---
  int q[2];
  assert(pipe(q) == 0);
  orig = fcntl(q[1], F_GETFL);
  assert(orig != -1);

  assert(dup2(q[1], STDOUT_FILENO) == STDOUT_FILENO);

  bufio_stream *so = bufio_open("-", "w", 100, 256, testname);
  assert(so != NULL);
  assert((fcntl(STDOUT_FILENO, F_GETFL) & O_NONBLOCK) != 0);

  assert(bufio_close(so) == 0);
  assert(fcntl(q[1], F_GETFL) == orig);

  assert(close(q[0]) == 0);
  assert(close(q[1]) == 0);

  return 0;
}
