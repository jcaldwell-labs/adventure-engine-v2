# GitHub Copilot Instructions

This file provides guidance to GitHub Copilot when working with code in this repository.

## Project Overview

Adventure Engine v2 is a C-based text adventure game engine with both single-player and multiplayer capabilities. Features include a smart terminal UI, flexible world scripting system, save/load functionality, and collaborative gameplay mechanics via tmux.

**Status**: v3.0 - Single-player complete, multiplayer infrastructure built
**Language**: C (C11 standard)
**Dependencies**: ncurses, readline

Key features:
- Verb+noun command parser with multi-word support
- .world file format for adventure scripting
- Save/load with multiple slots
- 6 player roles for multiplayer (LEADER, SCOUT, ENGINEER, MEDIC, DIPLOMAT, SPECIALIST)
- Smart terminal UI with scrolling, colors, and readline integration

## Build System

```bash
make all              # Build library, engine, and multiplayer (default)
make lib              # Build smartterm_simple library only
make engine           # Build single-player adventure-engine
make multiplayer      # Build session-coordinator daemon
make tests            # Build all test programs
make run              # Build and run single-player engine
make run-coordinator  # Build and run multiplayer coordinator
make run-tests        # Run all test suites
make clean            # Remove build artifacts
make help             # Show all targets
```

**Dependencies:**
- **Required**: GCC compiler, ncurses (`libncurses-dev`), readline (`libreadline-dev`)

**Compiler Flags**: `-Wall -Wextra -std=c11`

**Build Outputs:**
- `build/adventure-engine` - Single-player game
- `build/session-coordinator` - Multiplayer daemon
- `build/libsmartterm_simple.a` - Terminal UI library

## Architecture

### Directory Structure

```
adventure-engine-v2/
├── src/                  # Core C source files
│   ├── main.c            # Single-player game loop (499 LOC)
│   ├── parser.c          # Verb+noun command parser (99 LOC)
│   ├── world.c           # World/room/item system (260 LOC)
│   ├── world_loader.c    # .world file parser (376 LOC)
│   ├── save_load.c       # Game persistence (296 LOC)
│   ├── session.c         # Multiplayer sessions (514 LOC)
│   ├── player.c          # Player roles & states (550 LOC)
│   ├── ipc.c             # Inter-process communication (602 LOC)
│   └── session_coordinator.c  # Multiplayer daemon (298 LOC)
├── include/              # Header files
│   ├── smartterm_simple.h
│   ├── parser.h
│   ├── world.h
│   ├── world_loader.h
│   ├── save_load.h
│   ├── session.h
│   ├── player.h
│   └── ipc.h
├── lib/                  # External libraries
│   └── smartterm_simple.c  # Terminal UI library (194 LOC)
├── worlds/               # Example .world files (4 complete adventures)
│   ├── dark_tower.world
│   ├── haunted_mansion.world
│   ├── crystal_caverns.world
│   └── sky_pirates.world
├── tests/                # Test suite
│   ├── test_parser.c     # Parser tests (7/8 passing)
│   ├── test_world.c      # World tests (11/11 passing)
│   └── test_save_load.c  # Save/load tests (5/7 passing)
└── docs/                 # Documentation
```

### Core Components

**Single-Player Engine (Complete):**
1. **Parser** (`parser.c`): Verb+noun parsing, case-insensitive, direction shortcuts
2. **World System** (`world.c`): Rooms, items, inventory (20 capacity), 6-direction navigation
3. **World Loader** (`world_loader.c`): Parses .world files with validation
4. **Save/Load** (`save_load.c`): Persistence to `~/.adventure-saves/`
5. **Terminal UI** (`lib/smartterm_simple.c`): Scrolling output, coloring, readline

**Multiplayer Infrastructure (Foundation):**
1. **Session** (`session.c`): Session lifecycle (LOBBY -> ACTIVE -> COMPLETED)
2. **Player** (`player.c`): 6 roles with unique abilities
3. **IPC** (`ipc.c`): 9 message types, priority queue
4. **Coordinator** (`session_coordinator.c`): Central daemon (has TODO stubs)

### Key Constants

```c
MAX_ROOMS = 50
MAX_ITEMS = 50
MAX_INVENTORY = 20
MAX_PLAYERS = 8  // multiplayer
```

## Code Style and Conventions

- **Standard**: C11
- **Naming**: snake_case for functions/variables
- **Functions**: < 100 lines preferred
- **Comments**: Required for complex logic
- **Error handling**: Check all I/O operations
- **Memory**: Clean allocation/deallocation patterns

## .world File Format

Adventures are defined in `.world` files with INI-style syntax:

```ini
[WORLD]
name: My World
start: entrance

[ROOM:entrance]
name: Entrance Hall
description: A grand entrance with marble floors.
exits: north=hall, east=garden

[ITEM:key]
name: rusty key
description: An old brass key covered in patina.
takeable: yes
location: entrance
```

See `docs/WORLD-FORMAT.md` for complete specification.

## Before Committing (Required Steps)

Run these commands before every commit:

1. **Build**: `make clean && make all` - Ensure zero warnings
2. **Test**: `make run-tests` - All tests must pass
3. **Manual test**: `make run` - Verify basic gameplay

```bash
# Quick pre-commit check
make clean && make all && make run-tests
```

## Common Development Tasks

### Adding New Commands
1. Add command to `handle_command()` in `main.c`
2. Implement command function (e.g., `cmd_take()`)
3. Update parser if needed
4. Add test case in `tests/test_parser.c`
5. Update help text

### Creating New Worlds
1. Create `.world` file in `worlds/` directory
2. Define `[WORLD]` section with name and start room
3. Add `[ROOM:id]` sections with names, descriptions, exits
4. Add `[ITEM:id]` sections with locations
5. Test by running: `make run` and selecting your world

### Adding Test Cases
1. Create test file in `tests/`
2. Use existing test macros (TEST, PASS, FAIL, ASSERT_*)
3. Update Makefile test targets
4. Run `make run-tests` to verify

## Pull Request Standards

When creating PRs, follow these rules:

1. **Always link the issue**: Use `Fixes #N` or `Closes #N`
2. **Fill in all sections**: Never leave placeholder text

**Required PR format:**
```markdown
## Summary
[2-3 sentences describing what and why]

Fixes #[issue-number]

## Changes
- [Actual change 1]
- [Actual change 2]

## Testing
- [x] All tests pass (`make run-tests`)
- [x] Manual gameplay tested

## Type
- [x] New feature | Bug fix | Refactor | Docs | CI
```

## Known Limitations

1. **Visited room state not persisted** in saves
2. **Multiplayer not integrated** with game engine (infrastructure exists)
3. **Tab characters in commands** not split by parser (use spaces)
4. **No NPC dialogue system** (world files are static)
5. **No puzzle mechanics** (keys, locks, conditional exits not implemented)

## Debugging

```bash
# Build with debug symbols
gcc -g -Wall -Wextra -std=c11 ...

# Run with gdb
gdb build/adventure-engine

# Check for memory leaks
valgrind --leak-check=full ./build/adventure-engine
```

## Additional Documentation

- **docs/WORLD-FORMAT.md** - World file format specification
- **docs/QUICK-START-MULTIPLAYER.md** - 60-second multiplayer guide
- **docs/ARCHITECTURE.md** - System architecture
- **docs/MULTIPLAYER-SETUP.md** - Multiplayer setup guide
- **README.md** - User-facing documentation
