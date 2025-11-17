/*
 * Adventure Engine - Command Parser Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

// Helper: Convert string to lowercase
static void to_lower(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Helper: Trim whitespace from string
static char* trim(char *str) {
    // Trim leading
    while (isspace(*str)) str++;

    if (*str == 0) return str;

    // Trim trailing
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = 0;

    return str;
}

Command parse_input(const char *input) {
    Command cmd = {0};
    cmd.valid = false;

    if (!input || strlen(input) == 0) {
        return cmd;
    }

    // Make a working copy
    char buffer[256];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Trim and lowercase
    char *trimmed = trim(buffer);
    to_lower(trimmed);

    // Handle single-word commands (quit, help, inventory, look)
    if (strchr(trimmed, ' ') == NULL) {
        strncpy(cmd.verb, trimmed, sizeof(cmd.verb) - 1);
        cmd.noun[0] = '\0';
        cmd.valid = true;
        return cmd;
    }

    // Parse verb + noun (everything after first space is noun)
    char *space = strchr(trimmed, ' ');
    if (space) {
        // Extract verb
        size_t verb_len = space - trimmed;
        if (verb_len >= sizeof(cmd.verb)) {
            verb_len = sizeof(cmd.verb) - 1;
        }
        strncpy(cmd.verb, trimmed, verb_len);
        cmd.verb[verb_len] = '\0';

        // Extract noun (rest of string)
        char *noun_start = trim(space + 1);
        strncpy(cmd.noun, noun_start, sizeof(cmd.noun) - 1);
        cmd.noun[sizeof(cmd.noun) - 1] = '\0';

        cmd.valid = true;
    } else {
        // Single word
        strncpy(cmd.verb, trimmed, sizeof(cmd.verb) - 1);
        cmd.noun[0] = '\0';
        cmd.valid = true;
    }

    return cmd;
}

bool cmd_is(const Command *cmd, const char *verb) {
    if (!cmd || !cmd->valid || !verb) return false;
    return strcmp(cmd->verb, verb) == 0;
}

bool cmd_is_full(const Command *cmd, const char *verb, const char *noun) {
    if (!cmd || !cmd->valid || !verb || !noun) return false;
    return strcmp(cmd->verb, verb) == 0 && strcmp(cmd->noun, noun) == 0;
}

void cmd_free(Command *cmd) {
    // Nothing to free currently (stack-allocated strings)
    // Placeholder for future heap allocations
    (void)cmd;
}
