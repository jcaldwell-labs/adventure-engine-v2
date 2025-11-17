/*
 * SmartTerm Simple - Implementation
 * Extracted from smartterm POC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "smartterm_simple.h"

#define MAX_OUTPUT_LINES 1000

// Output buffer to prevent prompt duplication
typedef struct {
    char **lines;
    int count;
    int capacity;
} OutputBuffer;

// Global state
static OutputBuffer output;
static WINDOW *output_win;
static WINDOW *status_win;
static int screen_height, screen_width;
static char status_left[256] = "";
static char status_right[256] = "";

// Initialize output buffer
static void init_output_buffer(void) {
    output.capacity = MAX_OUTPUT_LINES;
    output.lines = malloc(sizeof(char*) * output.capacity);
    output.count = 0;
}

// Free output buffer
static void free_output_buffer(void) {
    for (int i = 0; i < output.count; i++) {
        free(output.lines[i]);
    }
    free(output.lines);
}

void st_init(void) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Initialize colors
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // Normal
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Command
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Comment
    init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Special
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Search

    getmaxyx(stdscr, screen_height, screen_width);

    // Create windows
    // Output takes most of screen (top)
    output_win = newwin(screen_height - 3, screen_width, 0, 0);
    scrollok(output_win, TRUE);

    // Status bar (single line, above input)
    status_win = newwin(1, screen_width, screen_height - 3, 0);

    refresh();

    // Initialize output buffer
    init_output_buffer();
}

void st_cleanup(void) {
    delwin(output_win);
    delwin(status_win);
    endwin();
    free_output_buffer();
}

void st_add_output(const char *text, STContext ctx) {
    if (output.count >= output.capacity) {
        // Drop oldest line
        free(output.lines[0]);
        memmove(output.lines, output.lines + 1, sizeof(char*) * (output.capacity - 1));
        output.count--;
    }

    // Store with context prefix for coloring
    char prefix = (ctx == ST_CTX_COMMAND) ? '!' :
                  (ctx == ST_CTX_COMMENT) ? '#' :
                  (ctx == ST_CTX_SPECIAL) ? '@' :
                  (ctx == ST_CTX_SEARCH)  ? '/' : ' ';

    size_t len = strlen(text) + 3;
    output.lines[output.count] = malloc(len);
    snprintf(output.lines[output.count], len, "%c %s", prefix, text);
    output.count++;
}

void st_clear_output(void) {
    for (int i = 0; i < output.count; i++) {
        free(output.lines[i]);
    }
    output.count = 0;
}

void st_update_status(const char *left, const char *right) {
    if (left) {
        strncpy(status_left, left, sizeof(status_left) - 1);
        status_left[sizeof(status_left) - 1] = '\0';
    }
    if (right) {
        strncpy(status_right, right, sizeof(status_right) - 1);
        status_right[sizeof(status_right) - 1] = '\0';
    }
}

void st_render(void) {
    // Render output buffer
    werase(output_win);
    box(output_win, 0, 0);

    int max_display_lines = screen_height - 5;
    int start_line = (output.count > max_display_lines) ?
                     (output.count - max_display_lines) : 0;

    for (int i = 0; i < output.count - start_line; i++) {
        int line_idx = start_line + i;
        char *line = output.lines[line_idx];

        // Color based on context prefix
        int color = COLOR_PAIR(1); // default
        if (line[0] == '!') color = COLOR_PAIR(2); // command (yellow)
        else if (line[0] == '#') color = COLOR_PAIR(3); // comment (green)
        else if (line[0] == '@') color = COLOR_PAIR(4); // special (cyan)
        else if (line[0] == '/') color = COLOR_PAIR(5); // search (magenta)

        wattron(output_win, color);
        mvwprintw(output_win, i + 1, 2, "%s", line + 2); // Skip prefix
        wattroff(output_win, color);
    }

    wrefresh(output_win);

    // Render status bar
    werase(status_win);
    wattron(status_win, A_REVERSE);
    mvwprintw(status_win, 0, 0, "%-*s", screen_width, "");
    mvwprintw(status_win, 0, 2, "%s", status_left);
    mvwprintw(status_win, 0, screen_width - strlen(status_right) - 2, "%s", status_right);
    wattroff(status_win, A_REVERSE);
    wrefresh(status_win);
}

char* st_read_input(const char *prompt) {
    // Temporarily suspend ncurses for readline
    def_prog_mode();
    endwin();

    // Use readline (handles history, editing)
    char *input = readline(prompt);

    // Resume ncurses
    reset_prog_mode();
    refresh();

    // Add to history if non-empty
    if (input && strlen(input) > 0) {
        add_history(input);
    }

    return input;
}

STContext st_detect_context(const char *input) {
    if (!input || strlen(input) == 0) return ST_CTX_NORMAL;

    if (input[0] == '!') return ST_CTX_COMMAND;
    if (input[0] == '#') return ST_CTX_COMMENT;
    if (input[0] == '@') return ST_CTX_SPECIAL;
    if (input[0] == '/') return ST_CTX_SEARCH;
    return ST_CTX_NORMAL;
}

const char* st_strip_context(const char *input, STContext ctx) {
    if (ctx != ST_CTX_NORMAL && input && strlen(input) > 0) {
        return input + 1; // Skip first character
    }
    return input;
}
