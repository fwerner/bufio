# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `bufio_open` in mode `"w"` on a named pipe (FIFO) waits up to `timeout` for a
  reader to attach (in 50 ms steps) instead of failing immediately.
- SIGPIPE is ignored for pipes and named pipes, not only sockets.

### Changed

- Count hostname resolution against connect timeout.
- **Breaking:** `bufio_open` with `timeout` from 0 to 100 ms on TCP now waits
  up to 100 ms: values in that range are raised to the 100 ms minimum because
  a connect cannot complete in zero time. Previously `timeout == 0` attempted a
  single blocking `connect()`, which waited as long as the OS allowed.
- **Breaking:** TCP connect errors other than connection refused (e.g.
  `ENETUNREACH`, `EHOSTUNREACH`, `EADDRNOTAVAIL`) now fail immediately instead
  of retrying until the timeout expires. A client started before its network
  interface is up therefore no longer waits for the network to appear.
- Log message for unresolvable hosts changed from `"no such host"` to
  `"can not resolve host"` (now including the `getaddrinfo` error string).
- **Breaking:** poll and I/O operations block indefinitely (`io_timeout_ms = -1`)
  by default for all stream types. Files, pipes and FIFOs used to be
  non-blocking (`0`). Call `bufio_timeout()` to restore the old behaviour.
- **Breaking:** clearer EOF vs. EPIPE semantics, consistent on Linux and macOS:
  - anonymous pipe whose writer hung up: `BUFIO_EPIPE` from `bufio_read` and
    `bufio_wait` (previously platform-dependent, `BUFIO_EOF` on macOS);
  - socket shut down by the peer: `BUFIO_EPIPE` from both `bufio_read` and
    `bufio_wait` (`bufio_read` used to report `BUFIO_EOF`);
  - FIFO with no writer attached: `BUFIO_EOF` (retryable);
  - `"-"` redirected from a regular file or device: `BUFIO_EOF`.
- **Breaking:** `"-"` streams operate on a `dup` of stdin/stdout, so
  `bufio_close` no longer closes the standard streams. `O_NONBLOCK` is set
  while the stream is open and the original flags are restored on close.
- `bufio_status_str` returns "broken pipe" for `BUFIO_EPIPE`.
- Enabling non-blocking mode preserves existing file status flags.

### Fixed

- Enforced the TCP connect timeout for hosts that drop SYNs, which previously
  caused system-dependent, very long delays.
- Reads and waits on named pipes timing out on macOS although data arrived
  (works around a `poll` bug).
- `bufio_wait(stream, -1)` on an EOF stream passing a negative value to `usleep`.
- Wrong operator precedence when checking the result of `stat` in `bufio_open`.
- Extensive TCP tests timing out under meson.
