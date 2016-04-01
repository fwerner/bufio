MSG = @printf "\033[1;33mBuilding $@\033[0m\n"
CFLAGS = -std=c99 -O2 -Isrc -Wall -Wextra -W -pedantic -march=native
LDFLAGS =

LIBS = lib/bufio.a
TESTS = $(shell grep -l "^int main" tests/*.c | sed s,\.c$$,, )
EXAMPLES = $(shell grep -l "^int main" examples/*.c | sed s,\.c$$,, )

.PHONY: all libs tests examples clean

all: libs tests examples

clean:
	rm -f -- $(LIBS) $(TESTS) $(EXAMPLES)
	rm -rf -- lib/

paths:
	@mkdir -p lib || test -d lib

lib/%.a: src/%.c | paths
	$(MSG)
	$(CC) $(CFLAGS) -c -o $@ $+

libs: $(LIBS)

tests: $(TESTS)

examples: $(EXAMPLES)

tests/%: tests/%.c $(LIBS)
	$(MSG)
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

examples/%: examples/%.c $(LIBS)
	$(MSG)
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)
