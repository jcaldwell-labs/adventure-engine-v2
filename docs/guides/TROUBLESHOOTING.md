# Troubleshooting Guide

This guide helps you diagnose and fix common issues with Adventure Engine v2.

## Table of Contents

- [Build Issues](#build-issues)
- [Runtime Issues](#runtime-issues)
- [World Loading Issues](#world-loading-issues)
- [Save/Load Issues](#save-load-issues)
- [Multiplayer Issues](#multiplayer-issues)
- [Terminal/UI Issues](#terminal-ui-issues)
- [General Debugging](#general-debugging)

## Build Issues

### Error: `ncurses.h: No such file or directory`

**Problem**: ncurses development libraries not installed

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libncurses-dev

# macOS
brew install ncurses
```

### Error: `readline/readline.h: No such file or directory`

**Problem**: readline development libraries not installed

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libreadline-dev

# macOS
brew install readline
```

### Error: `make: gcc: Command not found`

**Problem**: GCC compiler not installed

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential gcc

# macOS
xcode-select --install
# or
brew install gcc
```

### Build succeeds but warnings appear

**Check**:
```bash
make clean && make all 2>&1 | grep warning
```

**Common warnings**:
- `-Wsign-compare`: Should be fixed (report as bug)
- `-Wformat-truncation`: Usually safe, but review buffer sizes
- `-Wunused-*`: Remove unused variables/functions

### Linker errors about undefined references

**Problem**: Missing library link or wrong link order

**Check Makefile**:
```makefile
LDFLAGS = -lncurses -lreadline  # Must be in correct order
```

**Solution**: Ensure libraries are linked in correct order (dependencies last)

## Runtime Issues

### Crash on startup

**Diagnostic steps**:

1. Run with debugger:
   ```bash
   gdb ./build/adventure-engine
   (gdb) run
   (gdb) backtrace  # When it crashes
   ```

2. Check for segfaults:
   ```bash
   dmesg | tail
   ```

3. Check memory:
   ```bash
   valgrind ./build/adventure-engine
   ```

### "Segmentation fault" during gameplay

**Common causes**:
1. Null pointer dereference
2. Array out of bounds
3. Stack overflow
4. Corrupt game state

**Debug**:
```bash
# Build with debug symbols
make clean
gcc -g -Wall -Wextra -std=c11 -Iinclude src/*.c lib/*.c -o build/adventure-engine -lncurses -lreadline

# Run with debugger
gdb ./build/adventure-engine
(gdb) run
# Play until crash
(gdb) backtrace
(gdb) print variable_name
```

### Program hangs/freezes

**Possible causes**:
1. Infinite loop
2. Blocked I/O
3. Deadlock (multiplayer)

**Debug**:
```bash
# Run in background
./build/adventure-engine &
PID=$!

# If it hangs, attach debugger
gdb -p $PID
(gdb) info threads
(gdb) thread apply all backtrace
(gdb) quit
kill $PID
```

### Memory leaks

**Check with valgrind**:
```bash
valgrind --leak-check=full --show-leak-kinds=all \
    --track-origins=yes ./build/adventure-engine
```

**Common sources**:
- Unclosed files
- Unreleased readline buffers
- Unreleased ncurses resources

**Fix**: Ensure `st_cleanup()` is called before exit

## World Loading Issues

### Error: "Could not open world file"

**Causes**:
1. World file doesn't exist
2. Wrong file permissions
3. Incorrect path

**Check**:
```bash
ls -la worlds/
ls -la worlds/your_world.world
```

**Solution**:
```bash
# Ensure file exists
ls worlds/*.world

# Check permissions
chmod 644 worlds/your_world.world

# Check file is not empty
file worlds/your_world.world
cat worlds/your_world.world
```

### Error: "Parse error at line X"

**Problem**: Syntax error in .world file

**Debug**:
1. Check line number in error message
2. Open world file and go to that line
3. Common issues:
   - Missing `=` in key-value pairs
   - Invalid section headers
   - Typos in field names
   - Missing required fields

**Example errors**:
```
# Wrong
[ROOM:entrance
name: Hall

# Correct
[ROOM:entrance]
name: Hall
```

### Error: "Invalid exit to non-existent room"

**Problem**: Room referenced in exits but not defined

**Check**:
```bash
grep "exits:" worlds/your_world.world
grep "ROOM:" worlds/your_world.world
```

**Fix**: Ensure every room referenced in `exits:` has a `[ROOM:id]` section

### World loads but rooms/items missing

**Check**:
1. Room and item limits:
   - MAX_ROOMS = 50
   - MAX_ITEMS = 50

2. Verify counts:
   ```c
   // Add debug output in world_loader.c
   printf("Loaded %d rooms, %d items\n", world->room_count, world->item_count);
   ```

3. Check for duplicate IDs (later definition overwrites earlier)

## Save/Load Issues

### Error: "Could not save game"

**Possible causes**:
1. No write permissions
2. Disk full
3. Invalid save slot name

**Check**:
```bash
# Check save directory
ls -la ~/.adventure-saves/

# Create if missing
mkdir -p ~/.adventure-saves/

# Check permissions
ls -ld ~/.adventure-saves/

# Check disk space
df -h ~
```

### Error: "Could not load game"

**Causes**:
1. Save file doesn't exist
2. Corrupted save file
3. Version mismatch

**Debug**:
```bash
# List saves
ls -la ~/.adventure-saves/

# Examine save file
cat ~/.adventure-saves/your_save.sav

# Check format
head -n 5 ~/.adventure-saves/your_save.sav
```

**Valid save format**:
```
VERSION=1
WORLD=world_name
CURRENT_ROOM=0
...
```

### Loaded game has wrong state

**Known limitations**:
- Visited room state NOT persisted (by design)
- Room connections NOT persisted (loaded from world file)

**Persisted state**:
- Current room position ✓
- Inventory items ✓
- Item locations ✓
- World name ✓

## Multiplayer Issues

### Session coordinator won't start

**Check**:
```bash
# Try to run directly
./build/session-coordinator

# Check for port conflicts
lsof -i :PORT  # Replace PORT with coordinator port

# Check socket file
ls -la /tmp/adventure-engine/
```

### Can't connect to session

**Diagnostic**:
```bash
# Check coordinator is running
ps aux | grep session-coordinator

# Check socket exists
ls -la /tmp/adventure-engine/coordinator.sock

# Check permissions
ls -la /tmp/adventure-engine/
```

### Multiplayer features not working

**Current status**: ⚠️ Multiplayer infrastructure exists but is NOT integrated with game engine

**What works**:
- Session creation ✓
- Player registration ✓
- IPC message passing ✓

**What doesn't work**:
- Actual multiplayer gameplay ✗
- State synchronization ✗
- Real-time updates ✗

**See**: `docs/MULTIPLAYER-SETUP.md` for integration status

## Terminal/UI Issues

### Terminal display is corrupted

**Solutions**:
```bash
# Reset terminal
reset

# Clear screen
clear

# Resize terminal
# (if terminal was resized during game)
```

### Colors not showing

**Check**:
1. Terminal supports colors:
   ```bash
   echo $TERM
   # Should be xterm-256color or similar
   ```

2. Export if needed:
   ```bash
   export TERM=xterm-256color
   ```

### Input not responding

**Causes**:
1. Terminal in wrong mode
2. Readline issue
3. ncurses not initialized

**Fix**:
```bash
# Kill the process
Ctrl+C

# Reset terminal
reset

# Restart
./build/adventure-engine
```

### Status bar not updating

**Check**: Ensure `st_render()` is called after updates

**Debug**: Add debug output:
```c
printf("DEBUG: Status updated\n");
fflush(stdout);
```

### Command history not working

**Problem**: Readline not properly initialized

**Check**: Ensure `st_init()` is called before any input

## General Debugging

### Enable verbose output

**Add debug prints**:
```c
#define DEBUG 1

#if DEBUG
#define DBG(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif
```

**Usage**:
```c
DBG("Current room: %d", world->current_room);
DBG("Inventory item count: %d", count);
```

### Use GDB effectively

**Common commands**:
```gdb
# Run program
(gdb) run arg1 arg2

# Set breakpoint
(gdb) break src/main.c:150
(gdb) break cmd_go

# Inspect variables
(gdb) print world->current_room
(gdb) print *world
(gdb) print world->rooms[0]

# Step through code
(gdb) next       # Next line
(gdb) step       # Step into function
(gdb) continue   # Continue to next breakpoint

# Backtrace
(gdb) backtrace
(gdb) frame 2    # Examine specific frame

# Watchpoints
(gdb) watch world->current_room  # Break when value changes
```

### Memory debugging with Valgrind

**Check for leaks**:
```bash
valgrind --leak-check=full ./build/adventure-engine
```

**Check for invalid access**:
```bash
valgrind --track-origins=yes ./build/adventure-engine
```

**Suppress known issues**:
```bash
valgrind --suppressions=valgrind.supp ./build/adventure-engine
```

### Performance profiling

**Using gprof**:
```bash
# Compile with profiling
gcc -pg -Wall -Wextra -std=c11 ...

# Run program
./build/adventure-engine

# Generate report
gprof adventure-engine gmon.out > analysis.txt
```

**Using perf** (Linux):
```bash
perf record ./build/adventure-engine
perf report
```

### Logging

**Add file logging**:
```c
FILE *logfile = fopen("debug.log", "a");
fprintf(logfile, "[%s] Event: %s\n", timestamp, event);
fclose(logfile);
```

**Use syslog**:
```c
#include <syslog.h>

openlog("adventure-engine", LOG_PID, LOG_USER);
syslog(LOG_INFO, "Game started");
closelog();
```

## Getting Help

If you still have issues:

1. **Search existing issues**: https://github.com/jcaldwell-labs/adventure-engine-v2/issues

2. **Create a new issue** with:
   - Clear description of problem
   - Steps to reproduce
   - Error messages (full text)
   - Environment details:
     ```bash
     uname -a
     gcc --version
     make --version
     ```
   - What you've tried already

3. **Include relevant logs**:
   ```bash
   make clean && make all 2>&1 | tee build.log
   ./build/adventure-engine 2>&1 | tee run.log
   ```

## Common Environment Issues

### macOS specific

**ncurses issues**:
```bash
# Use homebrew ncurses
export CFLAGS="-I/usr/local/opt/ncurses/include"
export LDFLAGS="-L/usr/local/opt/ncurses/lib"
make clean && make all
```

**readline issues**:
```bash
# Use homebrew readline
export CFLAGS="-I/usr/local/opt/readline/include"
export LDFLAGS="-L/usr/local/opt/readline/lib"
make clean && make all
```

### Linux specific

**Missing 32-bit libraries** (if compiling 32-bit on 64-bit):
```bash
sudo apt-get install gcc-multilib
```

**SELinux issues**:
```bash
# Temporarily disable
sudo setenforce 0

# Check logs
sudo ausearch -m avc -ts recent
```

### WSL (Windows Subsystem for Linux)

**X11 not available**: Terminal-only is fine for this project

**File permissions**:
```bash
# If files have wrong permissions
git config core.fileMode false
```

## Test Failures

### Tests don't compile

**Check**:
```bash
make clean && make tests 2>&1 | less
```

**Common issues**:
- Missing include paths
- Wrong function signatures
- Linker order

### Tests compile but fail

**Run individual tests**:
```bash
./build/test_parser
./build/test_world
./build/test_save_load
```

**Add debug output**:
```c
printf("Expected: %d, Got: %d\n", expected, actual);
```

### Tests pass locally but fail in CI

**Check**:
- Environment differences (paths, permissions)
- Timing issues
- File system differences
- Dependencies not installed in CI

## Prevention Tips

1. **Always build with warnings**: `make clean && make all`
2. **Run tests frequently**: `make run-tests`
3. **Use version control**: Commit working code often
4. **Validate world files**: Test after editing .world files
5. **Check memory**: Run valgrind periodically
6. **Keep backups**: Copy save files before testing save/load

---

**Still stuck?** Open an issue: https://github.com/jcaldwell-labs/adventure-engine-v2/issues

*Last updated: Nov 23, 2025 (Phase 2)*
