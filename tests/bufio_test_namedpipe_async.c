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

  FORK_CHILD
    // Open a reader
    bufio_stream *si = bufio_open("test_bufio_namedpipe.fifo", "r", 1000, 256, "bufio_test_namedpipe");
    assert(si != NULL);

    // Wait until writer is connected
    bufio_timeout(si, 1000);
    size_t nread = 0;
    while ((nread = bufio_read(si, buf, 16)) == 0)
        assert(bufio_status(si) == BUFIO_EOF);

    assert(nread == 16);

    // Clean up
    assert(bufio_close(si) == 0);

  FORK_PARENT
    // Open a writer - this waits until a reader is present
    bufio_stream *so = bufio_open("test_bufio_namedpipe.fifo", "w", 2000, 256, "bufio_test_namedpipe");
    assert(so != NULL);

    // Delay a bit such that bufio_read enters poll()
    usleep(100000);

    // Write something
    assert(bufio_write(so, buf, 16) == 16 && bufio_flush(so) == 0);

    // Clean up
    assert(bufio_close(so) == 0);

  FORK_JOIN

  // Test poll - which requires a workaround on macOS
  FORK_CHILD
    // Open a reader
    bufio_stream *si = bufio_open("test_bufio_namedpipe.fifo", "r", 1000, 256, "bufio_test_namedpipe");
    assert(si != NULL);

    // Attempt to read immediately - which will enter poll()
    assert(bufio_read(si, buf, 16) == 16);

    // Clean up
    assert(bufio_close(si) == 0);

  FORK_PARENT
    // Open a writer - this waits until a reader is present
    bufio_stream *so = bufio_open("test_bufio_namedpipe.fifo", "w", 2000, 256, "bufio_test_namedpipe");
    assert(so != NULL);

    // Delay a bit such that bufio_read enters poll()
    usleep(100000);

    // Write something
    assert(bufio_write(so, buf, 16) == 16 && bufio_flush(so) == 0);

    // Keep open until the reader would time out (if poll were broken)
    usleep(1500000);
  
    // Clean up
    assert(bufio_close(so) == 0);

  FORK_JOIN


  assert(unlink("test_bufio_namedpipe.fifo") == 0);
  return 0;
}
