/*
 * Adventure Engine - Command Parser Implementation
 *
 * This module implements a simple verb+noun command parser for text adventure games.
 * It handles:
 *   - Single-word commands (look, quit, inventory)
 *   - Two-word commands (go north, take key)
 *   - Multi-word nouns (take rusty key, examine burning torch)
 *   - Case-insensitive matching
 *   - Whitespace normalization
 *
 * Algorithm:
 *   1. Copy input to working buffer
 *   2. Trim and lowercase the input
 *   3. Split on first space: verb + noun
 *   4. Noun contains everything after first space (allows multi-word items)
 *
 * Examples:
 *   "look"              -> verb="look", noun=""
 *   "go north"          -> verb="go", noun="north"
 *   "take rusty key"    -> verb="take", noun="rusty key"
 *   "  EXAMINE  torch " -> verb="examine", noun="torch"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

/*
 * Helper: Convert string to lowercase in-place
 * Used for case-insensitive command matching
 */
static void to_lower(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

/*
 * Helper: Trim leading and trailing whitespace from string
 * Returns pointer to trimmed string (modifies in-place)
 *
 * Examples:
 *   "  hello  " -> "hello"
 *   "\thello\n" -> "hello"
 */
static char* trim(char *str) {
    // Trim leading whitespace
    while (isspace(*str)) str++;

    // Handle empty string
    if (*str == 0) return str;

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = 0;  // Null terminate

    return str;
}

/*
 * Parse user input into a Command structure
 *
 * This is the main parsing function. It extracts a verb (action) and optional
 * noun (target) from user input. Multi-word nouns are supported by treating
 * everything after the first space as the noun.
 *
 * Parameters:
 *   input - Raw user input string
 *
 * Returns:
 *   Command structure with verb and noun populated
 *   valid=true if parsing succeeded, false otherwise
 *
 * Examples:
 *   "look"           -> {verb="look", noun="", valid=true}
 *   "go north"       -> {verb="go", noun="north", valid=true}
 *   "take iron key"  -> {verb="take", noun="iron key", valid=true}
 *   ""               -> {verb="", noun="", valid=false}
 */
Command parse_input(const char *input) {
    Command cmd = {0};
    cmd.valid = false;

    // Handle NULL or empty input
    if (!input || strlen(input) == 0) {
        return cmd;
    }

    // Make a working copy to avoid modifying original
    char buffer[256];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Normalize: trim whitespace and convert to lowercase
    char *trimmed = trim(buffer);
    to_lower(trimmed);

    // Handle single-word commands (e.g., "quit", "help", "inventory", "look")
    // These have no noun component
    if (strchr(trimmed, ' ') == NULL) {
        strncpy(cmd.verb, trimmed, sizeof(cmd.verb) - 1);
        cmd.noun[0] = '\0';
        cmd.valid = true;
        return cmd;
    }

    // Parse verb + noun pattern
    // Split on first space: everything before = verb, everything after = noun
    // This allows multi-word nouns like "rusty key" or "burning torch"
    char *space = strchr(trimmed, ' ');
    if (space) {
        // Extract verb (everything before first space)
        size_t verb_len = space - trimmed;
        if (verb_len >= sizeof(cmd.verb)) {
            verb_len = sizeof(cmd.verb) - 1;  // Clamp to buffer size
        }
        strncpy(cmd.verb, trimmed, verb_len);
        cmd.verb[verb_len] = '\0';

        // Extract noun (everything after first space, trimmed)
        // Trim again to handle multiple spaces between verb and noun
        char *noun_start = trim(space + 1);
        strncpy(cmd.noun, noun_start, sizeof(cmd.noun) - 1);
        cmd.noun[sizeof(cmd.noun) - 1] = '\0';

        cmd.valid = true;
    } else {
        // Fallback: single word (redundant with above check, but safe)
        strncpy(cmd.verb, trimmed, sizeof(cmd.verb) - 1);
        cmd.noun[0] = '\0';
        cmd.valid = true;
    }

    return cmd;
}

/*
 * Check if command matches a specific verb
 *
 * Helper function for command dispatch. Use this to check if a parsed
 * command matches a specific action.
 *
 * Parameters:
 *   cmd  - Parsed command
 *   verb - Verb to check against (e.g., "look", "take", "go")
 *
 * Returns:
 *   true if command's verb matches, false otherwise
 *
 * Example:
 *   Command cmd = parse_input("look around");
 *   if (cmd_is(&cmd, "look")) {
 *       // Handle look command
 *   }
 */
bool cmd_is(const Command *cmd, const char *verb) {
    if (!cmd || !cmd->valid || !verb) return false;
    return strcmp(cmd->verb, verb) == 0;
}

/*
 * Check if command matches both verb and noun
 *
 * Helper function for exact command matching. Use when you need to check
 * both the action and target.
 *
 * Parameters:
 *   cmd  - Parsed command
 *   verb - Verb to check against
 *   noun - Noun to check against
 *
 * Returns:
 *   true if both verb and noun match, false otherwise
 *
 * Example:
 *   Command cmd = parse_input("go north");
 *   if (cmd_is_full(&cmd, "go", "north")) {
 *       // Handle go north command
 *   }
 */
bool cmd_is_full(const Command *cmd, const char *verb, const char *noun) {
    if (!cmd || !cmd->valid || !verb || !noun) return false;
    return strcmp(cmd->verb, verb) == 0 && strcmp(cmd->noun, noun) == 0;
}

/*
 * Free any resources allocated by the command
 *
 * Currently a no-op since Command uses fixed-size buffers on the stack.
 * Included for API completeness and future extensibility (e.g., if we
 * switch to dynamic allocation for long commands).
 *
 * Parameters:
 *   cmd - Command to free
 */
void cmd_free(Command *cmd) {
    // Nothing to free currently (stack-allocated strings)
    // Placeholder for future heap allocations
    (void)cmd;
}
