# Contributing to Adventure Engine v2

Thank you for your interest in contributing to the Adventure Engine v2 project! This document provides guidelines and information to help you contribute effectively.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Code Guidelines](#code-guidelines)
- [Testing](#testing)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)
- [Community Guidelines](#community-guidelines)

## Getting Started

### Prerequisites

**Required:**
- GCC or compatible C compiler (C11 support)
- Make
- ncurses development libraries
- readline development libraries
- Git

**Optional:**
- tmux (for multiplayer features)
- valgrind (for memory leak detection)
- gdb (for debugging)

### Installation

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential gcc make tmux \
    libncurses-dev libreadline-dev git valgrind gdb
```

#### macOS
```bash
brew install gcc make tmux ncurses readline git valgrind
```

### Clone and Build

```bash
git clone https://github.com/jcaldwell-labs/adventure-engine-v2.git
cd adventure-engine-v2
make all
make run-tests
```

If all tests pass and the engine runs, you're ready to contribute!

## Development Setup

### Project Structure

Familiarize yourself with the codebase structure:

```
adventure-engine-v2/
├── src/              # C source files
├── include/          # Header files
├── lib/              # External libraries
├── tests/            # Test suite
├── worlds/           # Example world files
├── docs/             # Documentation
└── Makefile          # Build system
```

### Build Targets

```bash
make all              # Build everything
make engine           # Build single-player engine
make multiplayer      # Build multiplayer coordinator
make tests            # Build test suite
make run              # Run single-player engine
make run-tests        # Run all tests
make clean            # Clean build artifacts
make help             # Show all targets
```

### Development Workflow

1. Create a feature branch
2. Make your changes
3. Add/update tests
4. Run tests and ensure they pass
5. Update documentation
6. Submit a pull request

## How to Contribute

### Types of Contributions

We welcome various types of contributions:

#### 1. **Code Contributions**
- Bug fixes
- New features
- Performance improvements
- Code refactoring

#### 2. **Content Contributions**
- New world files (`.world`)
- Campaign definitions
- Example adventures
- Puzzles and challenges

#### 3. **Documentation**
- Code documentation
- User guides
- Tutorials
- API documentation

#### 4. **Testing**
- New test cases
- Bug reports
- Performance testing
- Platform compatibility testing

### Finding Issues to Work On

- Check the GitHub Issues page
- Look for issues tagged `good-first-issue` or `help-wanted`
- Review `TODO` and `FIXME` comments in the code:
  ```bash
  grep -rn "TODO\|FIXME" src/ include/
  ```

## Code Guidelines

### C Code Style

**General Principles:**
- Follow C11 standard
- Write clear, readable code
- Use descriptive variable and function names
- Keep functions focused and < 100 lines when possible
- Add comments for complex logic

**Naming Conventions:**
```c
// Functions: lowercase with underscores
void world_add_room(World *world, ...);

// Types: PascalCase
typedef struct {
    ...
} Command;

// Constants/Macros: UPPERCASE with underscores
#define MAX_ROOMS 50

// Variables: lowercase with underscores
int current_room = 0;
```

**Code Formatting:**
```c
// Use 4-space indentation (no tabs)
void example_function(int param) {
    if (condition) {
        do_something();
    } else {
        do_something_else();
    }
}

// Braces on same line for control structures
for (int i = 0; i < count; i++) {
    process(i);
}

// Pointer asterisk with type
char *string;
World *world;
```

**Comments:**
```c
// Single-line comments for brief explanations

/*
 * Multi-line comments for longer explanations,
 * function headers, and module documentation.
 */
```

### Error Handling

Always handle errors for:
- File I/O operations
- Memory allocation
- User input
- System calls

Example:
```c
FILE *fp = fopen(filename, "r");
if (!fp) {
    fprintf(stderr, "Error: Could not open file '%s'\n", filename);
    return false;
}
```

### Memory Management

- Always free allocated memory
- Avoid memory leaks
- Initialize pointers to NULL
- Check allocation success
- Use valgrind to detect leaks:
  ```bash
  valgrind --leak-check=full ./build/adventure-engine
  ```

### Compiler Warnings

Code must compile with zero warnings:
```bash
gcc -Wall -Wextra -std=c11 ...
```

Fix all warnings before submitting a PR.

## Testing

### Running Tests

```bash
# Run all tests
make run-tests

# Run specific test
./build/test_parser
./build/test_world
./build/test_save_load
```

### Writing Tests

When adding new features, include tests:

1. Create or update test file in `tests/`
2. Use existing test macros:
   ```c
   TEST("Feature name");
   ASSERT_EQ(expected, actual, "description");
   ASSERT_STR_EQ("expected", actual, "description");
   ASSERT_TRUE(condition, "description");
   PASS();
   ```
3. Update Makefile if adding new test file
4. Run tests to verify

Example test:
```c
void test_new_feature(void) {
    TEST("New feature description");

    // Setup
    World world;
    world_init(&world);

    // Test
    int result = new_feature(&world);

    // Assert
    ASSERT_EQ(EXPECTED_VALUE, result, "feature should return expected value");

    PASS();
}
```

### Test Requirements

- All tests must pass before merging
- Aim for good code coverage
- Test both success and failure cases
- Test edge cases and boundary conditions

## Documentation

### Code Documentation

**Header Files:**
```c
/*
 * Brief description of what this function does.
 *
 * Parameters:
 *   world - The world to operate on
 *   value - Description of parameter
 *
 * Returns:
 *   Description of return value
 */
int function_name(World *world, int value);
```

**Source Files:**
```c
// Add file-level comment at top
/*
 * Module Name - Brief Description
 * More detailed description of what this module does.
 */
```

### User Documentation

When adding user-facing features:

1. Update `README.md` if it affects users
2. Update relevant docs in `docs/`
3. Add examples if applicable
4. Update help text in the application

### World File Documentation

When creating example worlds:

1. Add world file to `worlds/`
2. Follow `.world` format specification
3. Include descriptive room and item names
4. Test that the world loads and plays correctly
5. Document any special features or puzzles

## Pull Request Process

### Before Submitting

**Checklist:**
- [ ] Code compiles with no warnings (`make all`)
- [ ] All tests pass (`make run-tests`)
- [ ] Added tests for new features
- [ ] Updated relevant documentation
- [ ] Followed code style guidelines
- [ ] No memory leaks (verified with valgrind if applicable)
- [ ] Commit messages are clear and descriptive

### Commit Messages

Write clear, descriptive commit messages:

```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain the problem this commit solves and why the change
was made.

- Bullet points are okay
- Use present tense: "Add feature" not "Added feature"
- Reference issues: "Fixes #123" or "Relates to #456"
```

Examples:
```
Fix memory leak in world_loader

The world_loader was not freeing temporary string buffers
after parsing room descriptions. Added proper cleanup in
the error path and success path.

Fixes #42
```

```
Add support for locked doors in world files

Implements new [LOCK] section in .world files allowing
creators to define locked doors that require specific items.

- Added lock checking to world_move()
- Updated world file parser
- Added test cases
- Updated WORLD-FORMAT.md documentation
```

### Pull Request Template

When creating a PR, include:

1. **Description**: What does this PR do?
2. **Motivation**: Why is this change needed?
3. **Testing**: How was this tested?
4. **Related Issues**: Links to related issues
5. **Screenshots**: If UI changes (for terminal output, use text)
6. **Checklist**: Confirm all requirements met

### Review Process

1. Maintainers will review your PR
2. Address any requested changes
3. Once approved, your PR will be merged
4. Your contribution will be acknowledged in release notes

## Community Guidelines

### Code of Conduct

- Be respectful and professional
- Welcome newcomers
- Provide constructive feedback
- Focus on what is best for the project
- Show empathy towards other community members

### Communication

- **Issues**: Bug reports, feature requests, questions
- **Pull Requests**: Code contributions
- **Discussions**: General project discussion

### Getting Help

If you need help:

1. Check existing documentation (`README.md`, `docs/`, `CLAUDE.md`)
2. Search existing issues
3. Create a new issue with:
   - Clear description of problem
   - Steps to reproduce (if applicable)
   - Environment details (OS, compiler version)
   - Error messages or logs

## Specific Contribution Areas

### Adding New Commands

To add a new game command:

1. Add verb to parser if needed (`src/parser.c`)
2. Add command function in `src/main.c`:
   ```c
   void cmd_new_command(World *world, const char *arg) {
       // Implementation
   }
   ```
3. Add to command dispatch in `handle_command()`
4. Update help text
5. Add tests in `tests/test_parser.c`

### Creating New Worlds

World files use a simple text format:

1. Create file in `worlds/` with `.world` extension
2. Follow format in `docs/WORLD-FORMAT.md`
3. Test by running: `./build/adventure-engine world_name`
4. Ensure all rooms are reachable
5. Verify items are properly placed
6. Submit PR with your world file

### Improving Multiplayer

The multiplayer infrastructure exists but needs integration:

1. Review `docs/MULTIPLAYER-SETUP.md`
2. Check TODOs in `src/session_coordinator.c`
3. Implement message handlers in IPC system
4. Add integration tests
5. Document your changes

### Performance Improvements

When optimizing:

1. Profile first to find bottlenecks
2. Measure performance before and after
3. Include benchmarks in PR
4. Don't sacrifice readability for minor gains
5. Document any algorithmic changes

## Release Process

(For maintainers)

1. Update version numbers
2. Update CHANGELOG
3. Run full test suite
4. Tag release
5. Update documentation
6. Announce release

## Questions?

If you have questions not covered here:

- Open an issue with the `question` label
- Check `CLAUDE.md` for technical details
- Review existing documentation in `docs/`

## Thank You!

Your contributions make this project better. We appreciate your time and effort!

---

**Happy Contributing!** 🎮✨
