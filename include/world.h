/*
 * Adventure Engine - World System
 * Defines rooms, items, and world state
 */

#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>

#define MAX_ITEMS 50
#define MAX_ROOMS 50
#define MAX_INVENTORY 20

// Directions
typedef enum {
    DIR_NORTH = 0,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_UP,
    DIR_DOWN,
    DIR_COUNT
} Direction;

// Item definition
typedef struct {
    char id[32];           // Unique identifier (e.g., "key", "sword")
    char name[64];         // Display name (e.g., "rusty key")
    char description[256]; // Full description
    bool takeable;         // Can be picked up?
    bool visible;          // Is it visible/discovered?
    // Issue #8: Use command support
    char use_message[256]; // Message shown when item is used (empty = not usable)
    bool use_consumable;   // Is item consumed after use?
} Item;

// Room definition
typedef struct {
    char id[32];              // Unique identifier
    char name[64];            // Short name
    char description[512];    // Full description
    int exits[DIR_COUNT];     // Room IDs for each direction (-1 = no exit)
    int items[MAX_ITEMS];     // Item IDs in this room (-1 = empty slot)
    bool visited;             // Has player been here?
    char locked_exits[DIR_COUNT][32];  // Item ID required to unlock each direction (empty = unlocked)
    bool exit_unlocked[DIR_COUNT];     // Runtime state: has this exit been unlocked?
} Room;

// Movement result codes (all non-negative for consistency)
typedef enum {
    MOVE_SUCCESS = 0,
    MOVE_NO_EXIT = 1,
    MOVE_LOCKED = 2
} MoveResult;

// World state
typedef struct {
    Room rooms[MAX_ROOMS];
    Item items[MAX_ITEMS];
    int inventory[MAX_INVENTORY]; // Item IDs in inventory (-1 = empty slot)
    int room_count;
    int item_count;
    int current_room;         // Current room ID
} World;

// Initialize world (empty)
void world_init(World *world);

// Add room to world (returns room ID)
int world_add_room(World *world, const char *id, const char *name, const char *desc);

// Add item to world (returns item ID)
int world_add_item(World *world, const char *id, const char *name, const char *desc, bool takeable);

// Place item in room
void world_place_item(World *world, int item_id, int room_id);

// Connect rooms with exit
void world_connect_rooms(World *world, int from_room, Direction dir, int to_room);

// Find room by ID (returns index, -1 if not found)
int world_find_room(World *world, const char *id);

// Find item by ID (returns index, -1 if not found)
int world_find_item(World *world, const char *id);

// Get current room
Room* world_current_room(World *world);

// Move to room in direction (returns true if successful)
bool world_move(World *world, Direction dir);

// Move with extended result (returns MoveResult, fills key_needed if locked)
MoveResult world_move_ex(World *world, Direction dir, char *key_needed, size_t key_size);

// Check if exit in direction is locked
bool world_exit_is_locked(World *world, Direction dir);

// Unlock exit in direction (for use by key items)
void world_unlock_exit(World *world, int room_id, Direction dir);

// Lock an exit (used by world loader)
void world_lock_exit(World *world, int room_id, Direction dir, const char *key_item_id);

// Get the key required for a locked exit
const char* world_get_required_key(World *world, Direction dir);

// Take item from current room (returns true if successful)
bool world_take_item(World *world, const char *item_id);

// Drop item in current room (returns true if successful)
bool world_drop_item(World *world, const char *item_id);

// Check if item is in inventory
bool world_has_item(World *world, const char *item_id);

// Remove item from inventory (for consumable items)
bool world_remove_from_inventory(World *world, const char *item_id);

// Get item from inventory by ID (returns NULL if not found)
Item* world_get_inventory_item(World *world, const char *item_id);

// Get item from current room by ID (returns NULL if not found)
Item* world_get_room_item(World *world, const char *item_id);

// Convert direction string to enum (returns -1 if invalid)
int str_to_direction(const char *str);

// Convert direction enum to string
const char* direction_to_str(Direction dir);

#endif // WORLD_H
