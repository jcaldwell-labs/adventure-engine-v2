/*
 * Adventure Engine - World Loader
 * Loads world definitions from .world files
 */

#ifndef WORLD_LOADER_H
#define WORLD_LOADER_H

#include "world.h"
#include <stdbool.h>

// Error information
typedef struct {
    int line_number;
    char message[256];
    bool has_error;
} LoadError;

// Load world from file
// Returns true on success, false on error
// If error, details are in the error parameter
bool world_load_from_file(World *world, const char *filename, LoadError *error);

// Get user-friendly error message
const char* world_loader_get_error(const LoadError *error);

#endif // WORLD_LOADER_H
