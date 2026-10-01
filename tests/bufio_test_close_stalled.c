#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"


// Closing a writer whose reader stalls must honour the I/O timeout instead
// of blocking forever: flags are restored only after the (non-blocking)
// flush. Deterministically fills the pipe first, then leaves buffered data
// pending for close() to flush.
int main(void)
{
  alarm(5);  // fail fast (SIGALRM) instead of hanging if close blocks
  const char testname[] = "bufio_test_close_stalled";
  char buf[65536];
  memset(buf, 'x', sizeof buf);

  // Anonymous pipe; the read end is held open but never read from
  int p[2];
  assert(pipe(p) == 0);
  assert(dup2(p[1], STDOUT_FILENO) == STDOUT_FILENO);

  bufio_stream *so = bufio_open("-", "w", 100, 4096, testname);
  assert(so != NULL);
  bufio_timeout(so, 200);

  // Fill the pipe buffer (64 KiB on Linux and macOS); the final write
  // comes back short once no progress is possible within the timeout
  while (bufio_write(so, buf, sizeof buf) == sizeof buf)
    ;

  // Buffer data without flushing: close() must push this out (and fail
  // fast) rather than hang on a blocking flush
  assert(bufio_write(so, buf, 1000) == 1000);

  struct timeval before, after;
  assert(gettimeofday(&before, NULL) == 0);
  int rc = bufio_close(so);
  assert(gettimeofday(&after, NULL) == 0);
  double elapsed = after.tv_sec + 1e-6 * after.tv_usec
                 - before.tv_sec - 1e-6 * before.tv_usec;
  assert(rc == -1);
  assert(elapsed < 5.0);

  assert(close(p[0]) == 0);
  assert(close(p[1]) == 0);

  return 0;
}
