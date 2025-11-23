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

    // Initialize exits to -1 (no exit)
    for (int i = 0; i < MAX_ROOMS; i++) {
        for (int j = 0; j < DIR_COUNT; j++) {
            world->rooms[i].exits[j] = -1;
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

    // Initialize exits
    for (int i = 0; i < DIR_COUNT; i++) {
        room->exits[i] = -1;
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

bool world_move(World *world, Direction dir) {
    Room *room = world_current_room(world);
    if (!room) return false;

    int next_room = room->exits[dir];
    if (next_room == -1) return false;

    world->current_room = next_room;
    world->rooms[next_room].visited = true;
    return true;
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
