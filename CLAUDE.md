# CLAUDE.md - AI Development Guide

## Project Overview

**adventure-engine-v2** is a C-based text adventure game engine with both single-player and multiplayer capabilities. It features a clean architecture, smart terminal UI, flexible world scripting system, and collaborative gameplay mechanics.

**Current Status**: v3.0 - Single-player complete, multiplayer infrastructure built
**Language**: C (C11 standard)
**Build System**: Make
**Dependencies**: ncurses, readline
**Test Framework**: Custom test suite in `tests/`

## Quick Context for AI Assistants

This project is well-structured and documented. When working on this codebase:

1. **Architecture is Clean**: Single-player engine is production-ready. Multiplayer infrastructure exists but needs integration.
2. **Tests Exist**: Run `make run-tests` to validate changes
3. **Build is Fast**: `make all` builds everything in ~1 second
4. **Code Quality**: Compiles with `-Wall -Wextra` with zero warnings
5. **Documentation**: Comprehensive docs in `/docs` and good inline comments

## Codebase Structure

```
adventure-engine-v2/
├── src/              # Core C source files
│   ├── main.c              # Single-player game loop (499 LOC)
│   ├── parser.c            # Command parser (99 LOC)
│   ├── world.c             # World/room/item system (260 LOC)
│   ├── world_loader.c      # .world file parser (376 LOC)
│   ├── save_load.c         # Game persistence (296 LOC)
│   ├── session.c           # Multiplayer session mgmt (514 LOC)
│   ├── player.c            # Player roles & states (550 LOC)
│   ├── ipc.c               # Inter-process comm (602 LOC)
│   └── session_coordinator.c  # Multiplayer daemon (298 LOC)
├── include/          # Header files (.h)
├── lib/              # External libraries
│   └── smartterm_simple.c  # Terminal UI library (194 LOC)
├── worlds/           # Example .world files (4 complete adventures)
├── campaigns/        # Multiplayer campaign definitions
├── realms/           # Multiplayer realm definitions
├── panels/           # Tmux dashboard update scripts
├── bin/              # Executable scripts (campaign launcher, join session)
├── tests/            # Test suite
│   ├── test_parser.c       # Parser tests (7/8 passing)
│   ├── test_world.c        # World tests (11/11 passing)
│   ├── test_save_load.c    # Save/load tests (7/7 passing)
│   ├── test_locked_exits.c # Locked exits tests (8/8 passing)
│   └── test_use_command.c  # Use command tests (8/8 passing)
├── docs/             # Comprehensive documentation
├── Makefile          # Build system
└── README.md         # User-facing documentation
```

## Key Components

### Single-Player Engine (Complete ✅)

**Parser** (`parser.{h,c}`):
- Verb+noun command parsing
- Case-insensitive matching
- Multi-word noun support
- Direction shortcuts (n/s/e/w/u/d)

**World System** (`world.{h,c}`):
- Room and item management
- Inventory system (20 item capacity)
- Navigation with 6 directions
- Item visibility and discovery

**World Loader** (`world_loader.{h,c}`):
- Parses .world files
- Validates references
- Error reporting with line numbers
- Supports rooms, items, exits, descriptions

**Save/Load** (`save_load.{h,c}`):
- Game state persistence to ~/.adventure-saves/
- Multiple save slots
- Stores: room position, inventory, item locations, world name, visited rooms

**Terminal UI** (`lib/smartterm_simple.{h,c}`):
- Scrolling output buffer
- Context-aware coloring (5 contexts)
- Status bar
- Readline integration for command history

### Multiplayer Infrastructure (Foundation Built ⚠️)

**Session Management** (`session.{h,c}`):
- Session lifecycle (LOBBY → ACTIVE → COMPLETED)
- 2-8 player capacity
- Session registry with persistence

**Player System** (`player.{h,c}`):
- 6 roles: LEADER, SCOUT, ENGINEER, MEDIC, DIPLOMAT, SPECIALIST
- Player states and connection tracking
- Team operations

**IPC** (`ipc.{h,c}`):
- 9 message types (CMD, STATE, EVENT, SYNC, SIGNAL, CHAT, HEARTBEAT, ERROR, ACK)
- Message queue implementation
- Priority-based messaging

**Session Coordinator** (`session_coordinator.c`):
- Central daemon for multiplayer
- **NOTE**: Core tick processing has TODO stubs (not integrated with game engine)

## Build System

### Makefile Targets

```bash
make all              # Build lib, engine, multiplayer
make lib              # Build smartterm_simple.a
make engine           # Build single-player adventure-engine
make multiplayer      # Build session-coordinator daemon
make tests            # Build all test programs
make run              # Build and run single-player engine
make run-coordinator  # Build and run multiplayer coordinator
make run-tests        # Run all test suites
make clean            # Remove build artifacts
make help             # Show all targets
```

### Compiler Flags

- `CC = gcc`
- `CFLAGS = -Wall -Wextra -std=c11 -Iinclude`
- `LDFLAGS = -lncurses -lreadline`

## Testing

### Running Tests

```bash
make run-tests
```

### Test Coverage

- **Parser**: Command parsing, multi-word support, case handling, whitespace
- **World**: Initialization, room/item creation, navigation, inventory
- **Save/Load**: Persistence, multiple slots, directory creation

### Current Test Results

- Parser: 7/8 passing (tab handling is expected behavior)
- World: 11/11 passing ✓
- Save/Load: 7/7 passing ✓
- Security: 16/16 passing ✓
- Locked Exits: 8/8 passing ✓
- Use Command: 8/8 passing ✓

## Development Guidelines

### Code Style

- C11 standard
- Descriptive variable names
- Functions < 100 lines preferred
- Comments for complex logic
- Error handling on all I/O operations

### Adding New Commands

1. Add command to `handle_command()` in `main.c`
2. Implement command function (e.g., `cmd_take()`)
3. Update parser if needed
4. Add test case in `tests/test_parser.c`
5. Update help text

### Creating New Worlds

See `docs/WORLD-FORMAT.md` for complete specification.

Example:
```
[WORLD]
name: My World
start: entrance

[ROOM:entrance]
name: Entrance Hall
description: A grand entrance.
exits: north=hall

[ITEM:key]
name: rusty key
description: An old key.
takeable: yes
location: entrance
```

### Adding Test Cases

1. Create test file in `tests/`
2. Include necessary headers from `include/`
3. Use existing test macros (TEST, PASS, FAIL, ASSERT_*)
4. Update Makefile test targets
5. Run `make run-tests` to verify

## Common Tasks

### Finding Code

- **Command handling**: `src/main.c` (handle_command function)
- **Parser logic**: `src/parser.c`
- **World operations**: `src/world.c`
- **File I/O**: `src/save_load.c`, `src/world_loader.c`
- **Multiplayer**: `src/session*.c`, `src/player.c`, `src/ipc.c`

### Debugging

```bash
# Build with debug symbols
gcc -g -Wall -Wextra -std=c11 ...

# Run with gdb
gdb build/adventure-engine

# Check for memory leaks
valgrind --leak-check=full ./build/adventure-engine

# Verbose build
make V=1 all
```

### Common Issues

1. **ncurses/readline not found**: Install dev packages
   ```bash
   sudo apt-get install libncurses-dev libreadline-dev
   ```

2. **Tests failing**: Check if running from project root
   ```bash
   cd /path/to/adventure-engine-v2
   make run-tests
   ```

3. **Save directory**: Created automatically at `~/.adventure-saves/`

## Known Limitations

1. **Multiplayer not integrated** with game engine (infrastructure exists)
2. **Tab characters in commands** not split by parser (use spaces)
3. **No NPC dialogue system** (world files are static)

## Documentation

### For Users

- `README.md` - Main user documentation
- `docs/guides/QUICK-START-MULTIPLAYER.md` - 60-second multiplayer guide
- `docs/reference/WORLD-FORMAT.md` - World file format specification
- `docs/guides/MULTIPLAYER-SETUP.md` - Multiplayer architecture and setup

### For Developers

- `CLAUDE.md` (this file) - AI development guide
- `CONTRIBUTING.md` - Contribution guidelines
- `llms.txt` - AI discoverability metadata
- `docs/architecture/ARCHITECTURE.md` - System architecture
- `docs/architecture/DEEP-DIVE-*.md` - Detailed component documentation
- `SESSION-SUMMARY.md` - Development history
- `ROADMAP-MULTIPLAYER-TMUX.md` - Future vision

## Project History

- **Session 1 (Nov 17, 2025)**: MVP - parser, world system, basic gameplay
- **Session 2 (Nov 18, 2025)**: World scripting, save/load, 4 example worlds
- **Session 3 (Nov 19, 2025)**: Multiplayer infrastructure, tmux integration
- **Phase 2 (Nov 23, 2025)**: Tests, CI/CD, documentation, code quality

## Quick Reference

### Important Files

- `src/main.c` - Game loop, command dispatch
- `include/world.h` - Data structures
- `Makefile` - Build configuration
- `tests/` - Test suite
- `worlds/*.world` - Example adventures

### Key Constants

- `MAX_ROOMS = 50`
- `MAX_ITEMS = 50`
- `MAX_INVENTORY = 20`
- `MAX_PLAYERS = 8` (multiplayer)

### Useful Commands

```bash
# Quick build and test
make clean && make all && make run-tests

# Run single-player
make run

# Check code
grep -r "TODO\|FIXME" src/ include/

# Count lines of code
find src include lib -name '*.c' -o -name '*.h' | xargs wc -l
```

## Contributing

See `CONTRIBUTING.md` for detailed contribution guidelines.

Quick checklist:
- [ ] Code compiles with no warnings
- [ ] Tests pass (`make run-tests`)
- [ ] Added tests for new features
- [ ] Updated documentation
- [ ] Followed existing code style

## Contact & Resources

- **Repository**: https://github.com/jcaldwell-labs/adventure-engine-v2
- **Issues**: GitHub Issues
- **Related Projects**: smartterm-prototype, terminal-stars

---

*This document is designed to help AI assistants (like Claude) quickly understand and work with this codebase. Last updated: Jan 23, 2026*
