/*
 * Adventure Engine - Command Parser
 * Handles verb+noun command parsing
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

// Command structure
typedef struct {
    char verb[64];    // Primary action (go, take, look, etc.)
    char noun[64];    // Object/target (north, key, door, etc.)
    bool valid;       // Was parse successful?
} Command;

// Parse input string into command structure
Command parse_input(const char *input);

// Check if command matches verb (case-insensitive)
bool cmd_is(const Command *cmd, const char *verb);

// Check if command matches verb and noun
bool cmd_is_full(const Command *cmd, const char *verb, const char *noun);

// Free any allocated command data (if needed in future)
void cmd_free(Command *cmd);

#endif // PARSER_H
