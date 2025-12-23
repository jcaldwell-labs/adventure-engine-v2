# Adventure Engine v3.0 - Multiplayer Team Building Platform

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C11](https://img.shields.io/badge/C-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)](Makefile)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

**Status**: Multiplayer & Tmux Integration Complete ✅
**Version**: 3.0 with Multiplayer, Tmux Dashboards & Team Building
**Latest Update**: Nov 2025

---

## Why Adventure Engine?

| Feature | Traditional Games | Adventure Engine |
|---------|-------------------|------------------|
| **Team Building** | Requires specialized software | Built-in role-based collaborative gameplay |
| **Deployment** | Complex installs, dependencies | Single binary, SSH-friendly, zero UI deps |
| **Customization** | Scripting knowledge required | Simple `.world` text files anyone can write |
| **Platform** | GUI-dependent | Pure terminal - works everywhere |
| **Multiplayer** | Often requires servers | Tmux-based, local network ready |

**Adventure Engine is for you if:**
- You want collaborative team-building activities without complex software
- You need terminal-based games that work over SSH
- You want to create custom adventures without coding
- You're building training scenarios for corporate environments
- You appreciate clean, auditable C code

---

## Quick Start

### Single-Player (30 seconds)

```bash
# Clone and build
git clone https://github.com/jcaldwell-labs/adventure-engine-v2.git
cd adventure-engine-v2
make all

# Play!
make run
```

### Multiplayer Team Session (60 seconds)

```bash
# 1. Build everything
make all

# 2. Launch a 4-player campaign
./bin/start-campaign intro_training 4

# 3. Players join from separate terminals
./bin/join-session SESS-XXXXXX-XXXX alice LEADER
./bin/join-session SESS-XXXXXX-XXXX bob SCOUT
./bin/join-session SESS-XXXXXX-XXXX carol ENGINEER
./bin/join-session SESS-XXXXXX-XXXX dave MEDIC
```

See [docs/guides/QUICK-START-MULTIPLAYER.md](docs/guides/QUICK-START-MULTIPLAYER.md) for the full guide.

---

## Overview

Adventure Engine v3.0 is a revolutionary **multiplayer text-based adventure platform** designed for team building, corporate training, and collaborative entertainment. Built in C with tmux-based real-time dashboards, it enables 2-8 players to work together in immersive text adventures.

### 🎯 What Makes v3.0 Special

- **🖥️ Tmux Multi-Panel Dashboards**: Real-time narrative, map, stats, and team log
- **👥 Multiplayer Sessions**: 2-8 players in synchronized adventures
- **🎭 Role-Based Gameplay**: LEADER, SCOUT, ENGINEER, MEDIC, DIPLOMAT, SPECIALIST
- **🤝 Collaborative Puzzles**: Challenges requiring teamwork and coordination
- **📊 Team Analytics**: Track communication, problem-solving, and collaboration
- **🎓 Training Ready**: Perfect for corporate team building and education
- **🔧 Zero UI Dependency**: Pure terminal, scriptable, SSH-friendly

### What This Is

**Single-Player Features (v1.0 - v2.0):**
- ✅ Text-based adventure engine with parser, world system, and inventory
- ✅ World file loader - define adventures in simple text files
- ✅ Save/load system with multiple save slots
- ✅ 4 complete playable single-player worlds
- ✅ Smart terminal UI with scrolling output, context coloring
- ✅ Command history, line editing (via readline)

**Multiplayer Features (v3.0 - NEW!):**
- ✅ **Session-based multiplayer** - coordinate daemon manages sessions
- ✅ **Tmux integration** - 4-panel real-time dashboard
- ✅ **Role-based mechanics** - 6 distinct player roles with unique abilities
- ✅ **Collaborative puzzles** - challenges requiring teamwork
- ✅ **Team communication** - shared log and chat system
- ✅ **Campaign system** - multi-realm progression
- ✅ **GM dashboard** - facilitator controls and monitoring
- ✅ **Team analytics** - metrics for debriefing
- ✅ **Named pipe IPC** - real-time state synchronization

---

## Multiplayer Dashboard

When you launch a campaign, you'll see a 4-panel tmux session:

```
┌─────────────────┬──────────────┐
│   NARRATIVE     │     MAP      │
│   (story)       │  (players)   │
├─────────────────┼──────────────┤
│   TEAM STATS    │   LOG/CHAT   │
│   (health,inv)  │  (actions)   │
└─────────────────┴──────────────┘
```

---

## Building

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt-get install tmux libncurses-dev libreadline-dev gcc make

# macOS
brew install tmux ncurses readline
```

### Compile & Run

```bash
make all              # Build everything (single + multiplayer)
make run              # Run single-player engine
make run-coordinator  # Run multiplayer session coordinator
make clean            # Clean build artifacts
```

---

## Player Roles (Multiplayer)

| Role | Abilities | Best For |
|------|-----------|----------|
| **LEADER** | See objectives, inspire team, make decisions | Organizers, natural leaders |
| **SCOUT** | Reveal hidden areas, detect secrets, fast movement | Explorers, detail-oriented |
| **ENGINEER** | Unlock mechanisms, solve puzzles, build tools | Problem-solvers, technical |
| **MEDIC** | Heal teammates, remove debuffs, revive | Supporters, team-focused |
| **DIPLOMAT** | Talk to NPCs, negotiate, read ancient texts | Communicators, social |
| **SPECIALIST** | Wildcard - any ability (reduced effectiveness) | Versatile, small teams |

---

## Playing the Game (Single-Player)

### Starting a Game

When you run the engine, you'll see a world selection menu:

```
Available worlds:
  1. dark_tower
  2. haunted_mansion
  3. crystal_caverns
  4. sky_pirates

Select world (or 'load <slot>'):
```

Choose a world by number (1-4) or name, or load a saved game with `load <slot>`.

### Commands

**Movement & Exploration:**
- **look, l** - Look around current room
- **go \<dir\>, \<dir\>** - Move (north/south/east/west/up/down)
- **examine \<item\>, x \<item\>** - Examine an item closely

**Inventory:**
- **take \<item\>, get \<item\>** - Pick up an item
- **drop \<item\>** - Drop an item from inventory
- **inventory, i** - Show what you're carrying

**Save/Load:**
- **save \<slot\>** - Save game to a slot (e.g., `save slot1`)
- **load \<slot\>** - Load game from a slot
- **saves** - List all available save slots

**Help:**
- **help, ?** - Show command list
- **quit, exit** - Quit game

### Multi-Word Items

The engine supports multi-word item names. You can use partial matches:

```
> take rusty key    # Works
> take key          # Also works
> examine torch     # Works
> x burning torch   # Also works
```

---

## Available Worlds

### 1. The Dark Tower (3 rooms)
A mysterious tower adventure. Perfect for learning the basics.
- **Theme**: Dark fantasy
- **Difficulty**: Beginner
- **Items**: Rusty key, burning torch, stone statue, glowing gem

### 2. The Haunted Mansion (7 rooms)
Explore a creepy abandoned mansion filled with secrets.
- **Theme**: Horror
- **Difficulty**: Intermediate
- **Items**: Silver candlestick, journal, knife, mirror, amulet, and more
- **Features**: Multiple floors, basement, interconnected rooms

### 3. The Crystal Caverns (12 rooms)
A vast underground cave system with glowing crystals.
- **Theme**: Exploration
- **Difficulty**: Advanced
- **Items**: Crystal lantern, rope, pickaxe, rare crystals, treasure
- **Features**: Largest world, underground river, secret grotto

### 4. The Sky Pirates (7 rooms)
Adventure aboard a flying pirate ship in the clouds.
- **Theme**: Adventure
- **Difficulty**: Intermediate
- **Items**: Spyglass, cutlass, compass, treasure map, rum
- **Features**: Multi-level ship, crow's nest, cargo hold

---

## Creating Your Own Worlds

Worlds are defined in simple text files using the `.world` format. See [docs/reference/WORLD-FORMAT.md](docs/reference/WORLD-FORMAT.md) for complete documentation.

### Quick Example

```
[WORLD]
name: My Adventure
start: entrance

[ROOM:entrance]
name: Starting Room
description: You are in a small room.
exits: north=hall

[ROOM:hall]
name: Great Hall
description: A vast hall opens before you.
exits: south=entrance

[ITEM:key]
name: golden key
description: A shiny golden key.
takeable: yes
location: entrance
```

Save as `worlds/my_adventure.world` and play with:
```bash
./build/adventure-engine my_adventure
```

---

## Architecture

```
adventure-engine-v2/
├── include/
│   ├── smartterm_simple.h    # UI library API
│   ├── parser.h               # Command parser
│   ├── world.h                # World/room/item system
│   ├── world_loader.h         # World file loader
│   └── save_load.h            # Save/load system
├── lib/
│   └── smartterm_simple.c     # UI library implementation
├── src/
│   ├── main.c                 # Game loop and commands
│   ├── parser.c               # Parser implementation
│   ├── world.c                # World implementation
│   ├── world_loader.c         # World file parsing
│   └── save_load.c            # Game persistence
├── worlds/                    # World definition files
│   ├── dark_tower.world
│   ├── haunted_mansion.world
│   ├── crystal_caverns.world
│   └── sky_pirates.world
├── docs/
│   ├── guides/                # User guides and tutorials
│   ├── architecture/          # Technical deep-dives
│   └── reference/             # Format specifications
├── build/                     # Build artifacts (generated)
└── Makefile
```

### Components

**smartterm_simple** (lib/)
- Extracted from SmartTerm POC
- Output buffer (no prompt duplication)
- Context-aware coloring
- Status bar
- Readline integration
- ~350 LOC, simple and sufficient

**Parser** (parser.{h,c})
- Tokenizes input
- Extracts verb+noun patterns
- Case-insensitive matching
- Supports shortcuts (n/s/e/w)
- Multi-word item support

**World** (world.{h,c})
- Room/item data structures
- Movement system
- Inventory management
- Item placement and discovery
- Room connections (exits)

**World Loader** (world_loader.{h,c})
- Parses .world files
- Validates room and item references
- Error reporting with line numbers
- Dynamic world creation

**Save/Load** (save_load.{h,c})
- Game state persistence
- Multiple save slots
- Saves to ~/.adventure-saves/
- Stores inventory, position, world state

**Main** (main.c)
- Game loop
- Command dispatch
- World selection
- Save/load integration
- UI integration

---

## Code Stats

| Component | LOC | Purpose |
|-----------|-----|---------|
| smartterm_simple.{h,c} | ~350 | UI library |
| parser.{h,c} | ~150 | Command parsing |
| world.{h,c} | ~400 | Game world system |
| world_loader.{h,c} | ~400 | World file loading |
| save_load.{h,c} | ~350 | Game persistence |
| main.c | ~500 | Game loop & commands |
| **Total** | **~2150** | **Complete engine** |

**Build time**: ~1 second
**Dependencies**: ncurses, readline

---

## Development Timeline

**Nov 17, 2025** (Session 1 - 3 hours):
- ✅ Created repository structure
- ✅ Extracted POC to smartterm_simple library
- ✅ Implemented parser system (verb+noun)
- ✅ Built world system (rooms, items, inventory)
- ✅ Created main game loop with demo world
- ✅ MVP complete

**Nov 18, 2025** (Session 2 - World Scripting):
- ✅ Designed world file format (.world)
- ✅ Implemented world loader with validation
- ✅ Created save/load system with slots
- ✅ Enhanced parser for multi-word items
- ✅ Added examine command
- ✅ Created 4 complete playable worlds
- ✅ Wrote complete documentation

**Total time**: ~8 hours
**Result**: Full-featured adventure engine with persistence

---

## Features Comparison

| Feature | MVP (v1.0) | Current (v2.0) |
|---------|-----------|---------------|
| World definition | Hardcoded | File-based |
| Save/Load | ❌ | ✅ |
| Number of worlds | 1 | 4 |
| Item examination | ❌ | ✅ |
| Multi-word items | ❌ | ✅ |
| Documentation | Basic | Complete |
| World creation | Requires C coding | Simple text files |

---

## Save System

Saves are stored in `~/.adventure-saves/` as `.sav` files. Each save contains:
- Current room position
- Inventory contents
- Room visited states
- Item placements
- World name (for loading correct world file)

**Example usage:**
```
> save mysave
Game saved to slot 'mysave'

> saves
=== SAVE SLOTS ===
  - mysave
  - quicksave
  - checkpoint1

> load mysave
Game loaded successfully!
```

---

## Future Enhancements

### Planned Features
- [ ] NPC system with dialogue
- [ ] Locked doors requiring keys
- [ ] Item usage system (use key on door)
- [ ] Conditional exits and triggers
- [ ] World scripting (events, variables)
- [ ] Sound effects (optional)

### Community
- Share your worlds in `worlds/` directory
- Submit world files via pull requests
- See [docs/reference/WORLD-FORMAT.md](docs/reference/WORLD-FORMAT.md) for format spec

---

## Related jcaldwell-labs Projects

Adventure Engine is part of the jcaldwell-labs ecosystem of terminal and CLI tools.

### Terminal/TUI Projects

| Project | Description | Synergy with Adventure Engine |
|---------|-------------|-------------------------------|
| [smartterm-prototype](https://github.com/jcaldwell-labs/smartterm-prototype) | Smart terminal with readline-like features | **Origin** - smartterm_simple library was extracted from this POC |
| [my-grid](https://github.com/jcaldwell-labs/my-grid) | ASCII canvas editor with vim-style navigation, zones, PTY support | Could provide ASCII map editor for world visualization |
| [boxes-live](https://github.com/jcaldwell-labs/boxes-live) | Real-time ASCII box drawing with joystick support | UI components for dashboards and panels |
| [terminal-stars](https://github.com/jcaldwell-labs/terminal-stars) | Starfield animation for terminals | Visual effects for space-themed adventures |
| [atari-style](https://github.com/jcaldwell-labs/atari-style) | Retro visual effects and shaders for terminal apps | CRT/retro aesthetic for nostalgic adventure themes |

### CLI Tools

| Project | Description | Synergy with Adventure Engine |
|---------|-------------|-------------------------------|
| [my-context](https://github.com/jcaldwell-labs/my-context) | Context tracking for development sessions (Go) | Track game design decisions and world-building sessions |
| [fintrack](https://github.com/jcaldwell-labs/fintrack) | Personal finance tracking CLI (Go) | Shared CLI patterns and readline integration |

### Game Engines

| Project | Description | Synergy with Adventure Engine |
|---------|-------------|-------------------------------|
| [tario](https://github.com/jcaldwell-labs/tario) | Terminal-based platformer game (Go) | Cross-pollinate game mechanics and terminal rendering |

### Meta/Organization

| Project | Description |
|---------|-------------|
| [capability-catalog](https://github.com/jcaldwell-labs/capability-catalog) | Skill/capability definitions for AI agents |

### Key Synergies

1. **Visual Enhancement Pipeline**: `atari-style` shaders + `terminal-stars` effects could create immersive space adventure backgrounds
2. **Map Editing Workflow**: `my-grid` ASCII editor could export directly to `.world` format room layouts
3. **Dashboard Components**: `boxes-live` real-time drawing could enhance multiplayer tmux dashboards
4. **Cross-Engine Learning**: `tario` platformer and Adventure Engine can share terminal rendering optimizations

---

## License

MIT License

---

## Credits

Built by jcaldwell-labs as part of the SmartTerm POC → Adventure Engine workflow.

**Status**: ✅ v2.0 Complete - World Scripting & Save/Load Ready!
