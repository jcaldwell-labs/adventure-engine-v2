# Adventure Engine v2.0

**Status**: MVP Complete ✅
**Timeline**: Built in ~3 hours (Nov 17, 2025)
**Decision**: Path B→A - Ship first, library later

---

## Overview

Adventure Engine v2 is a text-based adventure game engine built in C with a smart terminal UI. It demonstrates the **smartterm_simple** library extracted from the SmartTerm POC.

### What This Is

- ✅ Working adventure game engine with parser, world system, and inventory
- ✅ Demo game: "The Dark Tower" (3 rooms, 4 items, playable)
- ✅ Smart UI with scrolling output, context coloring, status bar
- ✅ Command history, line editing (via readline)
- ✅ Clean architecture: parser, world, UI separated

---

## Building

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt-get install libncurses-dev libreadline-dev gcc make

# macOS
brew install ncurses readline
```

### Compile & Run

```bash
make        # Build everything
make run    # Build and run adventure engine
make clean  # Clean build artifacts
```

---

## Playing the Game

### Commands

- **look, l** - Look around current room
- **go \<dir\>, \<dir\>** - Move (north/south/east/west/up/down)
- **take \<item\>** - Pick up an item
- **drop \<item\>** - Drop an item from inventory
- **inventory, i** - Show what you're carrying
- **help, ?** - Show commands
- **quit, exit** - Quit game

### Demo Game: "The Dark Tower"

Explore a mysterious tower with:
- 3 interconnected rooms
- 4 items to find (some takeable, some not)
- Basic puzzle: explore and collect items

**Starting location**: Tower Entrance

---

## Architecture

```
adventure-engine-v2/
├── include/
│   ├── smartterm_simple.h    # UI library API
│   ├── parser.h               # Command parser
│   └── world.h                # World/room/item system
├── lib/
│   └── smartterm_simple.c     # UI library implementation
├── src/
│   ├── main.c                 # Game loop and commands
│   ├── parser.c               # Parser implementation
│   └── world.c                # World implementation
├── build/                      # Build artifacts (generated)
└── Makefile
```

### Components

**smartterm_simple** (lib/)
- Extracted from SmartTerm POC
- Output buffer (no prompt duplication)
- Context-aware coloring
- Status bar
- Readline integration
- ~250 LOC, simple and sufficient

**Parser** (parser.{h,c})
- Tokenizes input
- Extracts verb+noun patterns
- Case-insensitive matching
- Supports shortcuts (n/s/e/w)

**World** (world.{h,c})
- Room/item data structures
- Movement system
- Inventory management
- Item placement and discovery
- Room connections (exits)

**Main** (main.c)
- Game loop
- Command dispatch
- Demo world creation
- UI integration

---

## Code Stats

| Component | LOC | Purpose |
|-----------|-----|---------|
| smartterm_simple.{h,c} | ~350 | UI library |
| parser.{h,c} | ~150 | Command parsing |
| world.{h,c} | ~400 | Game world system |
| main.c | ~350 | Game loop & demo |
| **Total** | **~1250** | **Complete engine** |

**Build time**: ~0.5 seconds
**Development time**: ~3 hours
**Dependencies**: ncurses, readline

---

## Development Timeline

**Nov 17, 2025** (Session 1 - 3 hours):
- ✅ Created repository structure
- ✅ Extracted POC to smartterm_simple library
- ✅ Implemented parser system (verb+noun)
- ✅ Built world system (rooms, items, inventory)
- ✅ Created main game loop with demo world
- ✅ Tested build (compiles clean, works)

**Total time**: 3 hours
**Result**: Working adventure engine MVP

---

## Learnings

### What Worked Well ✅

1. **POC extraction was fast** - Took 30 min to create library
2. **C is productive** - 1250 LOC in 3 hours is solid
3. **Parser is simple but powerful** - Verb+noun handles most commands
4. **World system is flexible** - Easy to add content
5. **smartterm_simple is sufficient** - No missing features yet

### What Could Improve ⚠️

1. **No persistence** - Can't save/load games yet
2. **Hardcoded world** - Should load from files
3. **Limited commands** - Need examine, use, etc.
4. **No NPCs** - Just rooms and items currently
5. **No scripting** - Room descriptions are static

---

## Next Steps

### Immediate (This Week)

- [ ] Add "examine \<item\>" command (detailed descriptions)
- [ ] Implement "use \<item\>" command (item interactions)
- [ ] Create 2-3 more worlds (different themes)
- [ ] Add more items and puzzles

### Short Term (Next 2 Weeks)

- [ ] World scripting (load from files, not hardcoded)
- [ ] Save/load game state
- [ ] More commands (open, close, unlock, etc.)
- [ ] NPC system (characters to interact with)

---

## Related Projects

- [smartterm-prototype](https://github.com/jcaldwell-labs/smartterm-prototype) - Original POC
- [terminal-stars](https://github.com/jcaldwell-labs/terminal-stars) - Space dogfight game

---

## License

MIT License

---

## Credits

Built by jcaldwell-labs as part of the SmartTerm POC → Adventure Engine workflow.

**Status**: ✅ MVP Complete - Ready for content creation!
