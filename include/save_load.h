/*
 * Adventure Engine - Save/Load System
 * Handles game state persistence
 */

#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H

#include "world.h"
#include <stdbool.h>

// Save game state to file
// slot_name: save slot identifier (e.g., "slot1", "autosave")
// world_name: name of the world being played
// Returns true on success
bool game_save(const World *world, const char *slot_name, const char *world_name);

// Load game state from file
// slot_name: save slot identifier
// Returns true on success
bool game_load(World *world, const char *slot_name, char *world_name, size_t world_name_size);

// List available save slots
// Returns number of saves found
int game_list_saves(char saves[][64], int max_saves);

// Delete a save slot
// Returns true on success
bool game_delete_save(const char *slot_name);

// Get save file path for a slot
// buffer: output buffer for path
// buffer_size: size of buffer
void get_save_path(const char *slot_name, char *buffer, size_t buffer_size);

// Check if a save slot exists
bool save_exists(const char *slot_name);

#endif // SAVE_LOAD_H
