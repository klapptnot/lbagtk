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
DEBUG_DIR := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

.PHONY: all
all: release

.PHONY: run
run: debug
	@echo "Running $(DEFAULT_DEBUG_TARGET)..."
	$(foreach target, $(TARGETS), $(DEBUG_DIR)/$(target)_debug $(filter-out $@,$(MAKECMDGOALS));)

.PHONY: rund
rund: debug
	@echo "Running target with GTK debugger $(DEFAULT_DEBUG_TARGET)..."
	$(foreach target, $(TARGETS), GTK_DEBUG=interactive $(DEBUG_DIR)/$(target)_debug;)

.PHONY: debug
debug: $(addprefix $(DEBUG_DIR)/, $(addsuffix _debug, $(TARGETS)))

$(DEBUG_DIR)/%_debug: src/%.c
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE_DIRS) $< $(LIB_DIRS) $(LIBS) $(DEBUG_LDFLAGS) -o $@

.PHONY: release
release: $(addprefix $(RELEASE_DIR)/, $(TARGETS))

$(RELEASE_DIR)/%: src/%.c
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(RELEASE_CFLAGS) $(INCLUDE_DIRS) $< $(LIB_DIRS) $(LIBS) $(RELEASE_LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf $(DEBUG_DIR) $(RELEASE_DIR)

# This prevents make from trying to build the args as targets
%:
	@:
