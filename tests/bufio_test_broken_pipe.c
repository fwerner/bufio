#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"


// Writes to a pipe or FIFO whose reader is gone must fail with EPIPE and
// leave a usable status (no SIGPIPE).
int main(void)
{
  const char testname[] = "bufio_test_broken_pipe";
  char buf[64];
  memset(buf, 'x', sizeof buf);

  // Anonymous pipe with the reader already closed
  int p[2];
  assert(pipe(p) == 0);
  assert(close(p[0]) == 0);  // no reader
  assert(dup2(p[1], STDOUT_FILENO) == STDOUT_FILENO);

  bufio_stream *so = bufio_open("-", "w", 100, 16, testname);
  assert(so != NULL);
  int so_fd = bufio_fileno(so);

  // Buffered write fits, so force the syscall with flush
  assert(bufio_write(so, buf, 4) == 4);
  assert(bufio_flush(so) == -1 && bufio_status(so) == BUFIO_EPIPE);
  assert(strcmp(bufio_status_str(so), "broken pipe") == 0);

  // Scattered write larger than the buffer takes the writev path
  assert(bufio_write(so, buf, 64) == 0 && bufio_status(so) == BUFIO_EPIPE);
  assert(strcmp(bufio_status_str(so), "broken pipe") == 0);

  // The failed flush propagates through close, but the fd is still released
  assert(bufio_close(so) == -1);
  assert(fcntl(so_fd, F_GETFL) == -1);

  assert(close(p[1]) == 0);

  // Named pipe: attach a reader so open succeeds, then remove it
  const char fname[] = "test_bufio_broken_pipe.fifo";
  unlink(fname);
  assert(mkfifo(fname, S_IRUSR | S_IWUSR) == 0);

  bufio_stream *si = bufio_open(fname, "r", 1000, 256, testname);
  assert(si != NULL);
  bufio_stream *fo = bufio_open(fname, "w", 2000, 256, testname);
  assert(fo != NULL);
  assert(bufio_close(si) == 0);  // last reader gone

  assert(bufio_write(fo, buf, 4) == 4);
  assert(bufio_flush(fo) == -1 && bufio_status(fo) == BUFIO_EPIPE);
  assert(strcmp(bufio_status_str(fo), "broken pipe") == 0);
  assert(bufio_close(fo) == -1);

  assert(unlink(fname) == 0);

  return 0;
}
