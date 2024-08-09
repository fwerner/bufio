#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"


int main(void)
{
  char buf[16];

  unlink("test_bufio_namedpipe.fifo");
  assert(mkfifo("test_bufio_namedpipe.fifo", S_IRUSR | S_IWUSR) == 0);

  // Attempt to open a writer: this should fail after the timeout because no one is listening
  struct timeval before, after;
  assert(gettimeofday(&before, NULL) == 0);
  bufio_stream *so = bufio_open("test_bufio_namedpipe.fifo", "w", 1000, 256, "bufio_test_namedpipe");
  assert(so == NULL);
  assert(gettimeofday(&after, NULL) == 0);
  assert(after.tv_sec + 1e-6 * after.tv_usec - before.tv_sec - 1e-6 * before.tv_usec >= 1.0);

  // Open a reader
  bufio_stream *si = bufio_open("test_bufio_namedpipe.fifo", "r", 1000, 256, "bufio_test_namedpipe");
  assert(si != NULL);

  // Open a writer: this should be much quicker than before
  assert(gettimeofday(&before, NULL) == 0);
  so = bufio_open("test_bufio_namedpipe.fifo", "w", 1000, 256, "bufio_test_namedpipe");
  assert(so != NULL);
  assert(gettimeofday(&after, NULL) == 0);
  assert(after.tv_sec + 1e-6 * after.tv_usec - before.tv_sec - 1e-6 * before.tv_usec < 1.0);

  // Assert no initial data
  assert(bufio_read(si, buf, 16) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);
  assert(bufio_wait(si, 0) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);
  assert(bufio_wait(si, 100) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);

  // Clean up
  assert(bufio_close(so) == 0);
  assert(bufio_close(si) == 0);
  assert(unlink("test_bufio_namedpipe.fifo") == 0);

  return 0;
}
