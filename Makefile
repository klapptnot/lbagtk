PROJECT := lbagtk
CC := clang
TARGETS := main
CSTD := c23

PHOME := $(shell pwd)
INCLUDE_DIRS := -I$(PHOME)/include -I$(PHOME)/src

COMMON_CFLAGS := -std=$(CSTD) \
	-Wall \
	-Wextra \
	-pedantic \
	$(shell pkg-config --cflags --libs gtk4)

# -01 needed with -D_FORTIFY_SOURCE for <features.h>
DEBUG_CFLAGS := $(COMMON_CFLAGS) -g \
	-O1 \
	-fstack-protector-strong \
	-D_FORTIFY_SOURCE=2 \
  -fsanitize=address,undefined \
	-fno-omit-frame-pointer

DEBUG_LDFLAGS := -fsanitize=address,undefined \
	-fno-omit-frame-pointer

RELEASE_CFLAGS := $(COMMON_CFLAGS) -O3 \
	-DNDEBUG \
	-Werror \
	-fomit-frame-pointer

RELEASE_LDFLAGS := -s \
	-flto

BUILD_DIR := $(PHOME)/build

.PHONY: all
all: release

.PHONY: run
run: debug
	@echo "Running $(DEFAULT_DEBUG_TARGET)..."
	$(foreach target, $(TARGETS), $(BUILD_DIR)/$(target)_debug $(filter-out $@,$(MAKECMDGOALS));)

.PHONY: rund
rund: debug
	@echo "Running target with GTK debugger $(DEFAULT_DEBUG_TARGET)..."
	$(foreach target, $(TARGETS), GTK_DEBUG=interactive $(BUILD_DIR)/$(target)_debug;)

.PHONY: debug
debug: $(addprefix $(BUILD_DIR)/, $(addsuffix _debug, $(TARGETS)))

$(BUILD_DIR)/%_debug: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE_DIRS) $< $(LIB_DIRS) $(LIBS) $(DEBUG_LDFLAGS) -o $@

.PHONY: release
release: $(addprefix $(BUILD_DIR)/, $(TARGETS))

$(BUILD_DIR)/%: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(RELEASE_CFLAGS) $(INCLUDE_DIRS) $< $(LIB_DIRS) $(LIBS) $(RELEASE_LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# This prevents make from trying to build the args as targets
%:
	@:
