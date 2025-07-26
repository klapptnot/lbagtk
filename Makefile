PROJECT := lbagtk
CC := clang
CSTD := c23

PHOME := $(shell pwd)
INCLUDE_DIRS := -I$(PHOME)/include -I$(PHOME)/src

SOURCES := src/main.c src/arg_parser.c

COMMON_CFLAGS := -std=$(CSTD) \
	-Wall \
	-Wextra \
	-pedantic \
	$(shell pkg-config --cflags gtk4)

LIBS := $(shell pkg-config --libs gtk4)

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

# Object files
DEBUG_OBJECTS := $(SOURCES:src/%.c=$(BUILD_DIR)/debug/%.o)
RELEASE_OBJECTS := $(SOURCES:src/%.c=$(BUILD_DIR)/release/%.o)

.PHONY: all
all: release

.PHONY: install
install: release
	@echo "Moving to /usr/bin/lbagtk..."
	sudo mv ./build/$(PROJECT) /usr/bin/lbagtk
	@echo "Installation complete! Run with: lbagtk"

.PHONY: run
run: debug
	@echo "Running $(DEFAULT_DEBUG_TARGET)..."
	@exec $(BUILD_DIR)/$(PROJECT)_debug $(filter-out $@,$(MAKECMDGOALS))

.PHONY: rund
rund: debug
	@echo "Running target with GTK debugger $(DEFAULT_DEBUG_TARGET)..."
	@GTK_DEBUG=interactive exec $(BUILD_DIR)/$(PROJECT)_debug;

.PHONY: debug
debug: $(addprefix $(BUILD_DIR)/, $(addsuffix _debug, $(PROJECT)))

# Debug executable linking
$(BUILD_DIR)/%_debug: $(DEBUG_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(DEBUG_OBJECTS) $(LIBS) $(DEBUG_LDFLAGS) -o $@

# Debug object files
$(BUILD_DIR)/debug/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)/debug
	$(CC) $(DEBUG_CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

.PHONY: release
release: $(addprefix $(BUILD_DIR)/, $(PROJECT))

# Release executable linking
$(BUILD_DIR)/%: $(RELEASE_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(RELEASE_OBJECTS) $(LIBS) $(RELEASE_LDFLAGS) -o $@

# Release object files
$(BUILD_DIR)/release/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)/release
	$(CC) $(RELEASE_CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# This prevents make from trying to build the args as PROJECT
%:
	@:
