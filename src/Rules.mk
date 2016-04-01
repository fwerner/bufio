TARGET := lib/libbufio.a
SOURCES := bufio.c

define MOVE_HEADER
	@mkdir -p $(TARGET_DIR)/include
	@cp -a $(DIR)/bufio.h $(TARGET_DIR)/include
endef

TGT_POSTMAKE := ${MOVE_HEADER}
