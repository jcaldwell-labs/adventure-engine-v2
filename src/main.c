/*
 * Adventure Engine v2 - Main Program
 * Uses smartterm_simple library for UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smartterm_simple.h"
#include "parser.h"
#include "world.h"

// Forward declarations
void create_demo_world(World *world);
void handle_command(World *world, const Command *cmd);
void cmd_look(World *world);
void cmd_go(World *world, const char *direction);
void cmd_take(World *world, const char *item_id);
void cmd_drop(World *world, const char *item_id);
void cmd_inventory(World *world);
void cmd_help(void);

int main(void) {
    // Initialize systems
    st_init();

    World world;
    world_init(&world);
    create_demo_world(&world);

    // Welcome message
    st_add_output("╔═══════════════════════════════════════════════╗", ST_CTX_NORMAL);
    st_add_output("║    ADVENTURE ENGINE v2.0 - The Dark Tower    ║", ST_CTX_NORMAL);
    st_add_output("╚═══════════════════════════════════════════════╝", ST_CTX_NORMAL);
    st_add_output("", ST_CTX_NORMAL);
    st_add_output("Type 'help' for commands, 'quit' to exit", ST_CTX_NORMAL);
    st_add_output("", ST_CTX_NORMAL);

    // Show initial room
    cmd_look(&world);

    st_update_status("Adventure Engine", "v2.0");
    st_render();

    // Game loop
    int running = 1;
    int turn_count = 0;

    while (running) {
        char *input = st_read_input("> ");

        if (!input) {
            running = 0;
            break;
        }

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        // Parse command
        Command cmd = parse_input(input);
        free(input);

        if (!cmd.valid) {
            st_add_output("I don't understand that.", ST_CTX_NORMAL);
            st_render();
            continue;
        }

        // Handle quit
        if (cmd_is(&cmd, "quit") || cmd_is(&cmd, "exit")) {
            st_add_output("", ST_CTX_NORMAL);
            st_add_output("Thanks for playing! Goodbye.", ST_CTX_NORMAL);
            running = 0;
        } else {
            handle_command(&world, &cmd);
            turn_count++;
        }

        // Update status
        char status_right[64];
        snprintf(status_right, sizeof(status_right), "Turns: %d", turn_count);
        st_update_status("Adventure Engine", status_right);
        st_render();

        cmd_free(&cmd);
    }

    st_cleanup();
    printf("Adventure complete. Total turns: %d\n", turn_count);
    return 0;
}

void create_demo_world(World *world) {
    // Create rooms
    int entrance = world_add_room(world, "entrance", "Tower Entrance",
        "You stand before a massive dark tower. Its stone walls are cold and ancient. "
        "A heavy wooden door stands ajar to the north.");

    int hall = world_add_room(world, "hall", "Grand Hall",
        "A vast hall with high vaulted ceilings. Torches flicker on the walls, "
        "casting dancing shadows. Stone stairs lead up to the east. "
        "The exit is to the south.");

    int chamber = world_add_room(world, "chamber", "Treasure Chamber",
        "A small chamber filled with ancient artifacts. Dust covers everything. "
        "Stairs lead down to the west.");

    // Connect rooms
    world_connect_rooms(world, entrance, DIR_NORTH, hall);
    world_connect_rooms(world, hall, DIR_SOUTH, entrance);
    world_connect_rooms(world, hall, DIR_EAST, chamber);
    world_connect_rooms(world, chamber, DIR_WEST, hall);

    // Create items
    int key = world_add_item(world, "key", "rusty key",
        "An old iron key, covered in rust but still functional.", true);

    int torch = world_add_item(world, "torch", "burning torch",
        "A wooden torch with flames that never seem to die.", true);

    int statue = world_add_item(world, "statue", "stone statue",
        "A heavy stone statue of a forgotten king. Too heavy to move.", false);

    int gem = world_add_item(world, "gem", "glowing gem",
        "A brilliant blue gem that pulses with inner light.", true);

    // Place items
    world_place_item(world, key, entrance);
    world_place_item(world, torch, hall);
    world_place_item(world, statue, hall);
    world_place_item(world, gem, chamber);

    // Start in entrance
    world->current_room = entrance;
    world->rooms[entrance].visited = true;
}

void handle_command(World *world, const Command *cmd) {
    if (cmd_is(cmd, "help") || cmd_is(cmd, "?")) {
        cmd_help();
    } else if (cmd_is(cmd, "look") || cmd_is(cmd, "l")) {
        cmd_look(world);
    } else if (cmd_is(cmd, "go") || cmd_is(cmd, "move")) {
        cmd_go(world, cmd->noun);
    } else if (cmd_is(cmd, "north") || cmd_is(cmd, "n")) {
        cmd_go(world, "north");
    } else if (cmd_is(cmd, "south") || cmd_is(cmd, "s")) {
        cmd_go(world, "south");
    } else if (cmd_is(cmd, "east") || cmd_is(cmd, "e")) {
        cmd_go(world, "east");
    } else if (cmd_is(cmd, "west") || cmd_is(cmd, "w")) {
        cmd_go(world, "west");
    } else if (cmd_is(cmd, "up") || cmd_is(cmd, "u")) {
        cmd_go(world, "up");
    } else if (cmd_is(cmd, "down") || cmd_is(cmd, "d")) {
        cmd_go(world, "down");
    } else if (cmd_is(cmd, "take") || cmd_is(cmd, "get")) {
        cmd_take(world, cmd->noun);
    } else if (cmd_is(cmd, "drop")) {
        cmd_drop(world, cmd->noun);
    } else if (cmd_is(cmd, "inventory") || cmd_is(cmd, "i")) {
        cmd_inventory(world);
    } else {
        st_add_output("I don't know how to do that. Type 'help' for commands.", ST_CTX_NORMAL);
    }
}

void cmd_help(void) {
    st_add_output("", ST_CTX_NORMAL);
    st_add_output("=== COMMANDS ===", ST_CTX_SPECIAL);
    st_add_output("  look, l          - Look around current room", ST_CTX_NORMAL);
    st_add_output("  go <dir>, <dir>  - Move in direction (north/south/east/west/up/down)", ST_CTX_NORMAL);
    st_add_output("  take <item>      - Pick up an item", ST_CTX_NORMAL);
    st_add_output("  drop <item>      - Drop an item", ST_CTX_NORMAL);
    st_add_output("  inventory, i     - Show your inventory", ST_CTX_NORMAL);
    st_add_output("  help, ?          - Show this help", ST_CTX_NORMAL);
    st_add_output("  quit, exit       - Quit the game", ST_CTX_NORMAL);
    st_add_output("", ST_CTX_NORMAL);
}

void cmd_look(World *world) {
    Room *room = world_current_room(world);
    if (!room) {
        st_add_output("You are nowhere. This is a bug.", ST_CTX_NORMAL);
        return;
    }

    st_add_output("", ST_CTX_NORMAL);
    st_add_output(room->name, ST_CTX_SPECIAL);
    st_add_output(room->description, ST_CTX_NORMAL);

    // Show exits
    char exits_buf[256] = "Exits: ";
    int exit_count = 0;
    for (int i = 0; i < DIR_COUNT; i++) {
        if (room->exits[i] != -1) {
            if (exit_count > 0) strcat(exits_buf, ", ");
            strcat(exits_buf, direction_to_str((Direction)i));
            exit_count++;
        }
    }
    if (exit_count == 0) {
        strcat(exits_buf, "none");
    }
    st_add_output(exits_buf, ST_CTX_COMMENT);

    // Show items
    int item_count = 0;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] != -1) {
            Item *item = &world->items[room->items[i]];
            if (item->visible) {
                char item_buf[128];
                snprintf(item_buf, sizeof(item_buf), "You see: %s", item->name);
                st_add_output(item_buf, ST_CTX_NORMAL);
                item_count++;
            }
        }
    }

    st_add_output("", ST_CTX_NORMAL);
}

void cmd_go(World *world, const char *direction) {
    if (!direction || strlen(direction) == 0) {
        st_add_output("Go where? Try 'go north' or just 'north'.", ST_CTX_NORMAL);
        return;
    }

    Direction dir = str_to_direction(direction);
    if (dir == -1) {
        st_add_output("I don't know that direction.", ST_CTX_NORMAL);
        return;
    }

    if (world_move(world, dir)) {
        st_add_output("", ST_CTX_NORMAL);
        cmd_look(world);
    } else {
        st_add_output("You can't go that way.", ST_CTX_NORMAL);
    }
}

void cmd_take(World *world, const char *item_id) {
    if (!item_id || strlen(item_id) == 0) {
        st_add_output("Take what?", ST_CTX_NORMAL);
        return;
    }

    Item *item = world_get_room_item(world, item_id);
    if (!item) {
        st_add_output("You don't see that here.", ST_CTX_NORMAL);
        return;
    }

    if (!item->takeable) {
        char buf[128];
        snprintf(buf, sizeof(buf), "You can't take the %s.", item->name);
        st_add_output(buf, ST_CTX_NORMAL);
        return;
    }

    if (world_take_item(world, item_id)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "You take the %s.", item->name);
        st_add_output(buf, ST_CTX_NORMAL);
    } else {
        st_add_output("Your inventory is full!", ST_CTX_NORMAL);
    }
}

void cmd_drop(World *world, const char *item_id) {
    if (!item_id || strlen(item_id) == 0) {
        st_add_output("Drop what?", ST_CTX_NORMAL);
        return;
    }

    Item *item = world_get_inventory_item(world, item_id);
    if (!item) {
        st_add_output("You don't have that.", ST_CTX_NORMAL);
        return;
    }

    if (world_drop_item(world, item_id)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "You drop the %s.", item->name);
        st_add_output(buf, ST_CTX_NORMAL);
    } else {
        st_add_output("You can't drop that here.", ST_CTX_NORMAL);
    }
}

void cmd_inventory(World *world) {
    st_add_output("", ST_CTX_NORMAL);
    st_add_output("=== INVENTORY ===", ST_CTX_SPECIAL);

    int count = 0;
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            Item *item = &world->items[world->inventory[i]];
            char buf[128];
            snprintf(buf, sizeof(buf), "  - %s", item->name);
            st_add_output(buf, ST_CTX_NORMAL);
            count++;
        }
    }

    if (count == 0) {
        st_add_output("  (empty)", ST_CTX_COMMENT);
    }

    st_add_output("", ST_CTX_NORMAL);
}
