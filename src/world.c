/*
 * Adventure Engine - World System Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"

void world_init(World *world) {
    memset(world, 0, sizeof(World));

    world->room_count = 0;
    world->item_count = 0;
    world->current_room = 0;

    // Initialize exits to -1 (no exit) and locked_exits to empty
    for (int i = 0; i < MAX_ROOMS; i++) {
        for (int j = 0; j < DIR_COUNT; j++) {
            world->rooms[i].exits[j] = -1;
            world->rooms[i].locked_exits[j][0] = '\0';
            world->rooms[i].exit_unlocked[j] = false;
        }
        for (int j = 0; j < MAX_ITEMS; j++) {
            world->rooms[i].items[j] = -1;
        }
    }

    // Initialize inventory
    for (int i = 0; i < MAX_INVENTORY; i++) {
        world->inventory[i] = -1;
    }
}

int world_add_room(World *world, const char *id, const char *name, const char *desc) {
    if (world->room_count >= MAX_ROOMS) return -1;

    int idx = world->room_count++;
    Room *room = &world->rooms[idx];

    strncpy(room->id, id, sizeof(room->id) - 1);
    strncpy(room->name, name, sizeof(room->name) - 1);
    strncpy(room->description, desc, sizeof(room->description) - 1);
    room->visited = false;
    room->description_shown = false;
    room->conditional_desc_count = 0;

    // Initialize exits and locked exits
    for (int i = 0; i < DIR_COUNT; i++) {
        room->exits[i] = -1;
        room->locked_exits[i][0] = '\0';
        room->exit_unlocked[i] = false;
    }

    // Initialize items
    for (int i = 0; i < MAX_ITEMS; i++) {
        room->items[i] = -1;
    }

    return idx;
}

int world_add_item(World *world, const char *id, const char *name, const char *desc, bool takeable) {
    if (world->item_count >= MAX_ITEMS) return -1;

    int idx = world->item_count++;
    Item *item = &world->items[idx];

    strncpy(item->id, id, sizeof(item->id) - 1);
    strncpy(item->name, name, sizeof(item->name) - 1);
    strncpy(item->description, desc, sizeof(item->description) - 1);
    item->takeable = takeable;
    item->visible = true;
    // Issue #8: Initialize use command fields
    item->use_message[0] = '\0';
    item->use_consumable = false;
    item->used = false;

    return idx;
}

void world_place_item(World *world, int item_id, int room_id) {
    if (item_id < 0 || item_id >= world->item_count) return;
    if (room_id < 0 || room_id >= world->room_count) return;

    Room *room = &world->rooms[room_id];

    // Find empty slot
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] == -1) {
            room->items[i] = item_id;
            return;
        }
    }
}

void world_connect_rooms(World *world, int from_room, Direction dir, int to_room) {
    if (from_room < 0 || from_room >= world->room_count) return;
    if (to_room < 0 || to_room >= world->room_count) return;
    if (dir < 0 || dir >= DIR_COUNT) return;

    world->rooms[from_room].exits[dir] = to_room;
}

int world_find_room(World *world, const char *id) {
    for (int i = 0; i < world->room_count; i++) {
        if (strcmp(world->rooms[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

int world_find_item(World *world, const char *id) {
    for (int i = 0; i < world->item_count; i++) {
        if (strcmp(world->items[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

Room* world_current_room(World *world) {
    if (world->current_room < 0 || world->current_room >= world->room_count) {
        return NULL;
    }
    return &world->rooms[world->current_room];
}

// Helper: Check if item is in the given room
static bool room_has_item(World *world, Room *room, const char *item_id) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] != -1) {
            Item *item = &world->items[room->items[i]];
            if (strcmp(item->id, item_id) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Helper: Check if item has been used
static bool item_was_used(World *world, const char *item_id) {
    int idx = world_find_item(world, item_id);
    if (idx >= 0) {
        return world->items[idx].used;
    }
    return false;
}

// Helper: Evaluate a single condition
static bool evaluate_condition(World *world, Room *room, ConditionalDesc *cond) {
    bool result = false;

    switch (cond->type) {
        case COND_FIRST_VISIT:
            // First time showing room description - uses description_shown flag
            // which is set AFTER the description is displayed (not on room entry)
            result = !room->description_shown;
            break;

        case COND_VISITED:
            // Has been visited before (return visit)
            result = room->visited;
            break;

        case COND_HAS_ITEM:
            result = world_has_item(world, cond->subject);
            break;

        case COND_ROOM_HAS_ITEM:
            result = room_has_item(world, room, cond->subject);
            break;

        case COND_ITEM_USED:
            result = item_was_used(world, cond->subject);
            break;
    }

    // Apply negation if specified
    return cond->negate ? !result : result;
}

const char* world_get_room_description(World *world, Room *room) {
    if (!world || !room) return "";

    // Priority: item_used > has_item > room_has_item > first_visit/visited > default
    // Highest priority matching condition wins; ties broken by definition order
    const char *best_desc = NULL;
    int best_priority = -1;

    for (int i = 0; i < room->conditional_desc_count; i++) {
        ConditionalDesc *cond = &room->conditional_descs[i];

        if (!evaluate_condition(world, room, cond)) {
            continue;
        }

        // Assign priority based on condition type
        int priority;
        switch (cond->type) {
            case COND_ITEM_USED:
                priority = 4;
                break;
            case COND_HAS_ITEM:
                priority = 3;
                break;
            case COND_ROOM_HAS_ITEM:
                priority = 2;
                break;
            case COND_FIRST_VISIT:
            case COND_VISITED:
                priority = 1;
                break;
            default:
                priority = 0;
        }

        if (priority > best_priority) {
            best_priority = priority;
            best_desc = cond->description;
        }
    }

    // Mark that this room's description has been shown (for first_visit tracking)
    room->description_shown = true;

    // Return best matching conditional description, or default
    return best_desc ? best_desc : room->description;
}

bool world_move(World *world, Direction dir) {
    char key_needed[32];
    MoveResult result = world_move_ex(world, dir, key_needed, sizeof(key_needed));
    return result == MOVE_SUCCESS;
}

MoveResult world_move_ex(World *world, Direction dir, char *key_needed, size_t key_size) {
    if (key_needed && key_size > 0) {
        key_needed[0] = '\0';
    }

    Room *room = world_current_room(world);
    if (!room) return MOVE_NO_EXIT;

    int next_room = room->exits[dir];
    if (next_room == -1) return MOVE_NO_EXIT;

    // Check if exit is locked
    if (room->locked_exits[dir][0] != '\0' && !room->exit_unlocked[dir]) {
        // Exit is locked - check if player has the key
        const char *required_key = room->locked_exits[dir];
        if (world_has_item(world, required_key)) {
            // Player has key - auto-unlock and proceed
            room->exit_unlocked[dir] = true;
        } else {
            // Player doesn't have key
            if (key_needed && key_size > 0) {
                strncpy(key_needed, required_key, key_size - 1);
                key_needed[key_size - 1] = '\0';
            }
            return MOVE_LOCKED;
        }
    }

    world->current_room = next_room;
    world->rooms[next_room].visited = true;
    return MOVE_SUCCESS;
}

bool world_exit_is_locked(World *world, Direction dir) {
    Room *room = world_current_room(world);
    if (!room) return false;

    // Exit is locked if it has a required key AND hasn't been unlocked yet
    return room->locked_exits[dir][0] != '\0' && !room->exit_unlocked[dir];
}

void world_unlock_exit(World *world, int room_id, Direction dir) {
    if (room_id < 0 || room_id >= world->room_count) return;
    if (dir < 0 || dir >= DIR_COUNT) return;

    world->rooms[room_id].exit_unlocked[dir] = true;
}

void world_lock_exit(World *world, int room_id, Direction dir, const char *key_item_id) {
    if (room_id < 0 || room_id >= world->room_count) return;
    if (dir < 0 || dir >= DIR_COUNT) return;
    if (!key_item_id) return;

    strncpy(world->rooms[room_id].locked_exits[dir],
            key_item_id,
            sizeof(world->rooms[room_id].locked_exits[dir]) - 1);
    world->rooms[room_id].locked_exits[dir][sizeof(world->rooms[room_id].locked_exits[dir]) - 1] = '\0';
    world->rooms[room_id].exit_unlocked[dir] = false;
}

const char* world_get_required_key(World *world, Direction dir) {
    Room *room = world_current_room(world);
    if (!room) return NULL;

    if (room->locked_exits[dir][0] != '\0') {
        return room->locked_exits[dir];
    }
    return NULL;
}

bool world_take_item(World *world, const char *item_id) {
    Room *room = world_current_room(world);
    if (!room) return false;

    // Find item in room
    int item_idx = -1;
    int room_slot = -1;

    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] != -1) {
            Item *item = &world->items[room->items[i]];
            if (strcmp(item->id, item_id) == 0) {
                item_idx = room->items[i];
                room_slot = i;
                break;
            }
        }
    }

    if (item_idx == -1) return false;

    Item *item = &world->items[item_idx];
    if (!item->takeable) return false;

    // Find empty inventory slot
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] == -1) {
            world->inventory[i] = item_idx;
            room->items[room_slot] = -1;
            return true;
        }
    }

    return false; // Inventory full
}

bool world_drop_item(World *world, const char *item_id) {
    Room *room = world_current_room(world);
    if (!room) return false;

    // Find item in inventory
    int item_idx = -1;
    int inv_slot = -1;

    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            Item *item = &world->items[world->inventory[i]];
            if (strcmp(item->id, item_id) == 0) {
                item_idx = world->inventory[i];
                inv_slot = i;
                break;
            }
        }
    }

    if (item_idx == -1) return false;

    // Find empty room slot
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] == -1) {
            room->items[i] = item_idx;
            world->inventory[inv_slot] = -1;
            return true;
        }
    }

    return false; // Room full (shouldn't happen normally)
}

bool world_has_item(World *world, const char *item_id) {
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            Item *item = &world->items[world->inventory[i]];
            if (strcmp(item->id, item_id) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool world_remove_from_inventory(World *world, const char *item_id) {
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            Item *item = &world->items[world->inventory[i]];
            if (strcmp(item->id, item_id) == 0) {
                world->inventory[i] = -1;
                return true;
            }
        }
    }
    return false;
}

Item* world_get_inventory_item(World *world, const char *item_id) {
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            Item *item = &world->items[world->inventory[i]];
            if (strcmp(item->id, item_id) == 0) {
                return item;
            }
        }
    }
    return NULL;
}

Item* world_get_room_item(World *world, const char *item_id) {
    Room *room = world_current_room(world);
    if (!room) return NULL;

    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] != -1) {
            Item *item = &world->items[room->items[i]];
            if (strcmp(item->id, item_id) == 0) {
                return item;
            }
        }
    }
    return NULL;
}

int str_to_direction(const char *str) {
    if (strcmp(str, "north") == 0 || strcmp(str, "n") == 0) return DIR_NORTH;
    if (strcmp(str, "south") == 0 || strcmp(str, "s") == 0) return DIR_SOUTH;
    if (strcmp(str, "east") == 0 || strcmp(str, "e") == 0) return DIR_EAST;
    if (strcmp(str, "west") == 0 || strcmp(str, "w") == 0) return DIR_WEST;
    if (strcmp(str, "up") == 0 || strcmp(str, "u") == 0) return DIR_UP;
    if (strcmp(str, "down") == 0 || strcmp(str, "d") == 0) return DIR_DOWN;
    return -1;
}

const char* direction_to_str(Direction dir) {
    switch (dir) {
        case DIR_NORTH: return "north";
        case DIR_SOUTH: return "south";
        case DIR_EAST: return "east";
        case DIR_WEST: return "west";
        case DIR_UP: return "up";
        case DIR_DOWN: return "down";
        default: return "unknown";
    }
}
