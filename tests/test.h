#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define assert(e) if (!(e)) { fprintf(stderr, "Assertion failed: %s, function %s, file %s, line %d.\n", #e, __func__, __FILE__, __LINE__); _exit(1); }

// Find a currently free loopback TCP port by binding port 0. The caller must
// use it promptly; there is a small race until bind/listen.
static inline int test_free_loopback_port(void)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) { fprintf(stderr, "socket failed\n"); _exit(1); }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) { fprintf(stderr, "bind failed\n"); _exit(1); }
  socklen_t len = sizeof(addr);
  if (getsockname(fd, (struct sockaddr *) &addr, &len) != 0) { fprintf(stderr, "getsockname failed\n"); _exit(1); }
  int port = ntohs(addr.sin_port);
  close(fd);
  return port;
}

// Three macros to simplify creation of forks and to check for a clean exit of the child process; use in this order
#define FORK_CHILD if (fork() == 0) {
#define FORK_PARENT _exit(0); } else {
#define FORK_JOIN int status; wait(&status); assert(WIFEXITED(status)); if (WEXITSTATUS(status) != 0) return 1; }
