# Adventure Engine v2 - Makefile
# Uses smartterm_simple library extracted from POC

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lncurses -lreadline

# Debug build with AddressSanitizer (use: make DEBUG=1)
ifdef DEBUG
	CFLAGS += -g -fsanitize=address,undefined -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address,undefined
endif

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
ENGINE_SRC = $(SRC_DIR)/main.c $(SRC_DIR)/parser.c $(SRC_DIR)/world.c $(SRC_DIR)/world_loader.c $(SRC_DIR)/save_load.c
ENGINE_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(ENGINE_SRC))
ENGINE_BIN = $(BUILD_DIR)/$(ENGINE_NAME)

# Multiplayer components
MP_NAME = session-coordinator
MP_SRC = $(SRC_DIR)/session_coordinator.c $(SRC_DIR)/session.c $(SRC_DIR)/player.c $(SRC_DIR)/ipc.c
MP_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(MP_SRC))
MP_BIN = $(BUILD_DIR)/$(MP_NAME)

# Test programs
TEST_DIR = tests
TEST_SMARTTERM = $(BUILD_DIR)/test_smartterm
TEST_PARSER = $(BUILD_DIR)/test_parser
TEST_WORLD = $(BUILD_DIR)/test_world
TEST_SAVE_LOAD = $(BUILD_DIR)/test_save_load
TEST_PATH_TRAVERSAL = $(BUILD_DIR)/test_path_traversal
TEST_SECURITY = $(BUILD_DIR)/test_security

.PHONY: all clean lib engine multiplayer test tests run run-test run-coordinator run-tests debug

all: lib engine multiplayer

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build library
lib: $(LIB_PATH)

$(LIB_PATH): $(LIB_OBJ) | $(BUILD_DIR)
	ar rcs $@ $^

$(LIB_OBJ): $(LIB_SRC) $(INCLUDE_DIR)/smartterm_simple.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build test programs
test: tests

tests: $(TEST_PARSER) $(TEST_WORLD) $(TEST_SAVE_LOAD) $(TEST_PATH_TRAVERSAL) $(TEST_SECURITY)

# Parser tests
$(TEST_PARSER): $(TEST_DIR)/test_parser.c $(BUILD_DIR)/parser.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# World tests
$(TEST_WORLD): $(TEST_DIR)/test_world.c $(BUILD_DIR)/world.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Save/Load tests
$(TEST_SAVE_LOAD): $(TEST_DIR)/test_save_load.c $(BUILD_DIR)/world.o $(BUILD_DIR)/save_load.o $(BUILD_DIR)/world_loader.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Path traversal protection tests
$(TEST_PATH_TRAVERSAL): $(TEST_DIR)/test_path_traversal.c $(BUILD_DIR)/save_load.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Security tests (Issue #16 fixes)
$(TEST_SECURITY): $(TEST_DIR)/test_security.c $(BUILD_DIR)/player.o $(BUILD_DIR)/session.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build adventure engine
engine: $(ENGINE_BIN)

$(ENGINE_BIN): $(ENGINE_OBJ) $(LIB_PATH) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build multiplayer coordinator
multiplayer: $(MP_BIN)

$(MP_BIN): $(MP_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run programs
run: engine
	$(ENGINE_BIN)

run-test: tests
	@echo "Running Parser Tests..."
	@$(TEST_PARSER) || true
	@echo ""
	@echo "Running World Tests..."
	@$(TEST_WORLD) || true
	@echo ""
	@echo "Running Save/Load Tests..."
	@$(TEST_SAVE_LOAD) || true
	@echo ""
	@echo "Running Path Traversal Protection Tests..."
	@$(TEST_PATH_TRAVERSAL) || true
	@echo ""
	@echo "Running Security Tests (Issue #16 fixes)..."
	@$(TEST_SECURITY) || true

run-tests: run-test

run-coordinator: multiplayer
	$(MP_BIN)

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Code quality targets
.PHONY: review security-audit

review:
	@./scripts/code-review.sh --quick

security-audit:
	@./scripts/code-review.sh --security

full-review:
	@./scripts/code-review.sh --full

memory-audit:
	@./scripts/code-review.sh --memory

# Help
help:
	@echo "Adventure Engine v2 - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build library, engine, and multiplayer (default)"
	@echo "  lib              - Build smartterm_simple library"
	@echo "  engine           - Build adventure engine"
	@echo "  multiplayer      - Build session coordinator"
	@echo "  test, tests      - Build all test programs"
	@echo "  run              - Build and run adventure engine"
	@echo "  run-coordinator  - Build and run session coordinator"
	@echo "  run-test         - Build and run all tests"
	@echo "  run-tests        - Alias for run-test"
	@echo "  clean            - Remove build artifacts"
	@echo "  debug            - Build with AddressSanitizer (use: make DEBUG=1)"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Code Quality:"
	@echo "  review           - Quick code review (quality + security)"
	@echo "  security-audit   - Security-focused review only"
	@echo "  full-review      - Complete review (all agents)"
	@echo "  memory-audit     - C memory safety analysis"
	@echo ""
	@echo "Debug mode:"
	@echo "  make DEBUG=1 all - Build all with AddressSanitizer"
	@echo "  make DEBUG=1 run - Run engine with memory safety checks"
