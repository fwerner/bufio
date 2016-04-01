TARGET := bin/bufio_benchmark
SOURCES := bufio_benchmark.c

LDFLAGS := -L${TARGET_DIR}/lib
LDLIBS  := -lbufio
PREREQS := lib/libbufio.a
