#ifdef __linux__
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#else
#undef _POSIX_C_SOURCE
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bufio.h"
#include "test.h"


// "-" opens stdin as BUFIO_PIPE, but stdin may be backed by a regular file (e.g. `./prog
// < input.txt`) or /dev/null. Hitting EOF there must report BUFIO_EOF (retryable, like
// BUFIO_FILE), not BUFIO_EPIPE (terminal hangup).
int main(void)
{
  const char fname[] = "test_bufio_stdin_file.dat";
  const char testname[] = "bufio_test_stdin_file";
  char buf[16];

  // Create a file with known content
  int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  assert(fd != -1);
  assert(write(fd, "hello", 5) == 5);
  assert(close(fd) == 0);

  // Redirect it to stdin
  fd = open(fname, O_RDONLY);
  assert(fd != -1);
  if (fd != STDIN_FILENO) {
    assert(dup2(fd, STDIN_FILENO) == STDIN_FILENO);
    assert(close(fd) == 0);
  }

  bufio_stream *si = bufio_open("-", "r", 100, 256, testname);
  assert(si != NULL);

  // Read the content back
  assert(bufio_read(si, buf, 5) == 5);
  assert(memcmp(buf, "hello", 5) == 0);

  // Past EOF: desired BUFIO_EOF, not BUFIO_EPIPE
  assert(bufio_read(si, buf, 16) == 0 && bufio_status(si) == BUFIO_EOF);
  assert(bufio_wait(si, 0) == 0 && bufio_status(si) == BUFIO_EOF);

  assert(bufio_close(si) == 0);  // note: also closes STDIN_FILENO

  // Same for /dev/null: immediate EOF, not EPIPE
  fd = open("/dev/null", O_RDONLY);
  assert(fd != -1);
  if (fd != STDIN_FILENO) {
    assert(dup2(fd, STDIN_FILENO) == STDIN_FILENO);
    assert(close(fd) == 0);
  }

  si = bufio_open("-", "r", 100, 256, testname);
  assert(si != NULL);

  assert(bufio_read(si, buf, 16) == 0 && bufio_status(si) == BUFIO_EOF);
  assert(bufio_wait(si, 0) == 0 && bufio_status(si) == BUFIO_EOF);

  assert(bufio_close(si) == 0);
  assert(unlink(fname) == 0);

  return 0;
}
