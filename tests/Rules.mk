LDFLAGS := -L${TARGET_DIR}/lib
LDLIBS  := -lbufio
PREREQS := lib/libbufio.a

SUBMAKEFILES := $(shell ls ${DIR}/bufio_test_*.mk | sed s,^${DIR}/,,g)
