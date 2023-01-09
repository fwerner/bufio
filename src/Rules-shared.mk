TARGET      := lib/libbufio.so
SOURCES     := bufio.c
TGT_LDFLAGS := -shared
TGT_PREREQS := lib/libbufio.a
