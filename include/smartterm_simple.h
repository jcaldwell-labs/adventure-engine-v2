/*
 * SmartTerm Simple - Minimal terminal UI library
 * Extracted from smartterm POC for adventure-engine-v2
 *
 * Provides:
 * - Scrolling output buffer (no prompt duplication)
 * - Context-aware coloring
 * - Status bar
 * - Readline integration for input
 */

#ifndef SMARTTERM_SIMPLE_H
#define SMARTTERM_SIMPLE_H

// Context types for colored output
typedef enum {
    ST_CTX_NORMAL,
    ST_CTX_COMMAND,    // ! prefix - yellow
    ST_CTX_COMMENT,    // # prefix - green
    ST_CTX_SPECIAL,    // @ prefix - cyan
    ST_CTX_SEARCH      // / prefix - magenta
} STContext;

// Initialize smartterm (sets up ncurses, colors, windows)
void st_init(void);

// Cleanup smartterm (restore terminal)
void st_cleanup(void);

// Add line to output buffer with context
void st_add_output(const char *text, STContext ctx);

// Clear all output
void st_clear_output(void);

// Update status bar (left and right text)
void st_update_status(const char *left, const char *right);

// Read input with readline (handles history, editing)
// Returns dynamically allocated string (caller must free)
// Returns NULL on EOF/quit
char* st_read_input(const char *prompt);

// Render current state to screen
void st_render(void);

// Detect context from input string (checks first character)
STContext st_detect_context(const char *input);

// Get text without context prefix (returns pointer into input, not allocated)
const char* st_strip_context(const char *input, STContext ctx);

#endif // SMARTTERM_SIMPLE_H
