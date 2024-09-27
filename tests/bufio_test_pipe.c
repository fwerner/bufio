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

  // Create pipe
  int p[2];
  assert(pipe(p) == 0);
  assert(close(STDIN_FILENO) == 0);
  assert(close(STDOUT_FILENO) == 0);

  FORK_CHILD
    // Redirect pipe to stdin
    assert(close(p[1]) == 0);
    assert(dup2(p[0], STDIN_FILENO) == STDIN_FILENO);

    // Open a reader on stdin
    bufio_stream *si = bufio_open("-", "r", 100, 256, "bufio_test_pipe");
    assert(si != NULL);

    // Assert no initial data
    assert(bufio_read(si, buf, 16) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);
    assert(bufio_wait(si, 0) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);
    assert(bufio_wait(si, 100) == 0 && bufio_status(si) == BUFIO_TIMEDOUT);

    sleep(1);

    // Test read after write
    assert(bufio_read(si, buf, 4) == 4);

    sleep(2);

    // Assert EOF after writer hung up
    struct timeval before, after;
    assert(bufio_read(si, buf, 16) == 0 && bufio_status(si) == BUFIO_EPIPE);
    assert(bufio_wait(si, 0) == -1 && bufio_status(si) == BUFIO_EPIPE);
    assert(gettimeofday(&before, NULL) == 0);
    assert(bufio_wait(si, 1000) == -1 && bufio_status(si) == BUFIO_EPIPE);
    assert(gettimeofday(&after, NULL) == 0);
    assert(after.tv_sec + 1e-6 * after.tv_usec - before.tv_sec - 1e-6 * before.tv_usec < 1.0);

    assert(bufio_close(si) == 0);

  FORK_PARENT
    // Redirect stdout to pipe
    assert(close(p[0]) == 0);
    assert(dup2(p[1], STDOUT_FILENO) == STDOUT_FILENO);

    // Open a writer to stdout
    bufio_stream *so = bufio_open("-", "w", 100, 256, "bufio_test_pipe");
    assert(so != NULL);

    // Test read after write
    sleep(1);
    assert(bufio_write(so, buf, 4) == 4 && bufio_flush(so) == 0);

    // Close writer (and the duplicate fd)
    sleep(1);
    assert(bufio_close(so) == 0);
    close(p[1]);

  FORK_JOIN
  return 0;
}
