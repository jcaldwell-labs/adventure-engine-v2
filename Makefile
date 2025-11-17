# Adventure Engine v2 - Makefile
# Uses smartterm_simple library extracted from POC

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lncurses -lreadline

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build
INCLUDE_DIR = include

# Library
LIB_NAME = libsmartterm_simple.a
LIB_SRC = $(LIB_DIR)/smartterm_simple.c
LIB_OBJ = $(BUILD_DIR)/smartterm_simple.o
LIB_PATH = $(BUILD_DIR)/$(LIB_NAME)

# Adventure engine
ENGINE_NAME = adventure-engine
ENGINE_SRC = $(SRC_DIR)/main.c $(SRC_DIR)/parser.c $(SRC_DIR)/world.c
ENGINE_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(ENGINE_SRC))
ENGINE_BIN = $(BUILD_DIR)/$(ENGINE_NAME)

# Test program
TEST_NAME = test_smartterm
TEST_SRC = $(SRC_DIR)/test_smartterm.c
TEST_OBJ = $(BUILD_DIR)/test_smartterm.o
TEST_BIN = $(BUILD_DIR)/$(TEST_NAME)

.PHONY: all clean lib engine test run run-test

all: lib engine

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build library
lib: $(LIB_PATH)

$(LIB_PATH): $(LIB_OBJ) | $(BUILD_DIR)
	ar rcs $@ $^

$(LIB_OBJ): $(LIB_SRC) $(INCLUDE_DIR)/smartterm_simple.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build test program
test: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJ) $(LIB_PATH) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_OBJ): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build adventure engine
engine: $(ENGINE_BIN)

$(ENGINE_BIN): $(ENGINE_OBJ) $(LIB_PATH) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run programs
run: engine
	$(ENGINE_BIN)

run-test: test
	$(TEST_BIN)

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Help
help:
	@echo "Adventure Engine v2 - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build library and engine (default)"
	@echo "  lib       - Build smartterm_simple library"
	@echo "  engine    - Build adventure engine"
	@echo "  test      - Build test program"
	@echo "  run       - Build and run adventure engine"
	@echo "  run-test  - Build and run test program"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help"
