TGT_LDFLAGS  := -L${TARGET_DIR}/lib
TGT_LDLIBS   := -l:libbufio.a
TGT_PREREQS  := lib/libbufio.a
SUBMAKEFILES := $(shell ls ${DIR}/bufio_test_*.mk | sed s,^${DIR}/,,g)

TESTS := follow lockedfile
# SKIPPED_TESTS := bufio_test_follow_chunk# needs manual Ctrl+C

.PHONY: check
check: $(addprefix bin/bufio_test_,${TESTS})
	@echo "Running tests:"
	@for TEST in $^; do \
		printf -- "- $$TEST... "; \
		$$TEST && echo "pass" || exit 1; \
	done \