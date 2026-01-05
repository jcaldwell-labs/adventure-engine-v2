/*
 * Adventure Engine v2 - Main Program
 * Uses smartterm_simple library for UI
 * Now with world file loading and save/load support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smartterm_simple.h"
#include "parser.h"
#include "world.h"
#include "world_loader.h"
#include "save_load.h"

// Global world name for save/load
static char g_world_name[64] = "unknown";

// Forward declarations
void handle_command(World *world, const Command *cmd);
void cmd_look(World *world);
void cmd_go(World *world, const char *direction);
void cmd_take(World *world, const char *item_id);
void cmd_drop(World *world, const char *item_id);
void cmd_inventory(World *world);
void cmd_examine(World *world, const char *item_id);
void cmd_use(World *world, const char *item_id);
void cmd_save(World *world, const char *slot_name);
void cmd_load(World *world, const char *slot_name);
void cmd_saves(void);
void cmd_help(void);

// Helper: Find item by partial name match
static Item* find_item_fuzzy(World *world, const char *name, bool check_inventory, bool check_room) {
    // First try exact match in inventory
    if (check_inventory) {
        Item *item = world_get_inventory_item(world, name);
        if (item) return item;
    }

    // Try exact match in room
    if (check_room) {
        Item *item = world_get_room_item(world, name);
        if (item) return item;
    }

    // Try partial match by checking if item name contains the search string
    if (check_inventory) {
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (world->inventory[i] != -1) {
                Item *item = &world->items[world->inventory[i]];
                if (strstr(item->name, name) != NULL || strstr(item->id, name) != NULL) {
                    return item;
                }
            }
        }
    }

    if (check_room) {
        Room *room = world_current_room(world);
        if (room) {
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (room->items[i] != -1) {
                    Item *item = &world->items[room->items[i]];
                    if (strstr(item->name, name) != NULL || strstr(item->id, name) != NULL) {
                        return item;
                    }
                }
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // Initialize systems
    st_init();

    World world;
    world_init(&world);

    // Welcome message
    st_add_output("╔═══════════════════════════════════════════════╗", ST_CTX_NORMAL);
    st_add_output("║    ADVENTURE ENGINE v2.0 - World Explorer    ║", ST_CTX_NORMAL);
    st_add_output("╚═══════════════════════════════════════════════╝", ST_CTX_NORMAL);
    st_add_output("", ST_CTX_NORMAL);

    // Load world from command line or prompt
    char world_file[256] = "";
    bool loaded_from_save = false;

    if (argc > 1) {
        strncpy(world_file, argv[1], sizeof(world_file) - 1);
    } else {
        st_add_output("Available worlds:", ST_CTX_NORMAL);
        st_add_output("  1. dark_tower", ST_CTX_NORMAL);
        st_add_output("  2. haunted_mansion", ST_CTX_NORMAL);
        st_add_output("  3. crystal_caverns", ST_CTX_NORMAL);
        st_add_output("  4. sky_pirates", ST_CTX_NORMAL);
        st_add_output("", ST_CTX_NORMAL);
        st_add_output("Type 'help' for commands, 'quit' to exit", ST_CTX_NORMAL);
        st_add_output("", ST_CTX_NORMAL);
        st_render();

        char *input = st_read_input("Select world (or 'load <slot>'): ");
        if (!input) {
            st_cleanup();
            return 0;
        }

        // Check if user wants to load a save
        if (strncmp(input, "load ", 5) == 0) {
            char slot_name[64];
            strncpy(slot_name, input + 5, sizeof(slot_name) - 1);
            slot_name[sizeof(slot_name) - 1] = '\0';

            // Trim whitespace
            char *trimmed = slot_name;
            while (*trimmed == ' ') trimmed++;

            if (strlen(trimmed) > 0) {
                char loaded_world[64];
                if (game_load(&world, trimmed, loaded_world, sizeof(loaded_world))) {
                    st_add_output("", ST_CTX_NORMAL);
                    st_add_output("Game loaded successfully!", ST_CTX_SPECIAL);
                    strncpy(g_world_name, loaded_world, sizeof(g_world_name) - 1);
                    loaded_from_save = true;
                    free(input);
                } else {
                    st_add_output("", ST_CTX_NORMAL);
                    st_add_output("Failed to load save. Starting new game.", ST_CTX_NORMAL);
                    st_add_output("", ST_CTX_NORMAL);
                    st_render();
                    free(input);
                    input = st_read_input("Select world: ");
                    if (!input) {
                        st_cleanup();
                        return 0;
                    }
                    strncpy(world_file, input, sizeof(world_file) - 1);
                    free(input);
                }
            }
        } else {
            strncpy(world_file, input, sizeof(world_file) - 1);
            free(input);
        }
    }

    // Load world from file if not loaded from save
    if (!loaded_from_save) {
        // Map number to world name (using strncpy for safety)
        if (strcmp(world_file, "1") == 0) {
            strncpy(world_file, "dark_tower", sizeof(world_file) - 1);
            world_file[sizeof(world_file) - 1] = '\0';
        }
        else if (strcmp(world_file, "2") == 0) {
            strncpy(world_file, "haunted_mansion", sizeof(world_file) - 1);
            world_file[sizeof(world_file) - 1] = '\0';
        }
        else if (strcmp(world_file, "3") == 0) {
            strncpy(world_file, "crystal_caverns", sizeof(world_file) - 1);
            world_file[sizeof(world_file) - 1] = '\0';
        }
        else if (strcmp(world_file, "4") == 0) {
            strncpy(world_file, "sky_pirates", sizeof(world_file) - 1);
            world_file[sizeof(world_file) - 1] = '\0';
        }

        // Validate world file name to prevent path traversal
        if (!is_safe_filename(world_file)) {
            st_add_output("", ST_CTX_NORMAL);
            st_add_output("ERROR: Invalid world file name. Only alphanumeric, underscore, and hyphen allowed.", ST_CTX_NORMAL);
            st_add_output("", ST_CTX_NORMAL);
            st_render();
            st_cleanup();
            return 1;
        }

        // Build full path
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "worlds/%s.world", world_file);

        // Load world
        LoadError error;
        if (!world_load_from_file(&world, full_path, &error)) {
            st_add_output("", ST_CTX_NORMAL);
            st_add_output("ERROR: Failed to load world file!", ST_CTX_NORMAL);
            st_add_output(world_loader_get_error(&error), ST_CTX_NORMAL);
            st_add_output("", ST_CTX_NORMAL);
            st_render();
            st_cleanup();
            return 1;
        }

        strncpy(g_world_name, world_file, sizeof(g_world_name) - 1);

        st_add_output("", ST_CTX_NORMAL);
        st_add_output("World loaded successfully!", ST_CTX_SPECIAL);
    }

    st_add_output("", ST_CTX_NORMAL);

    // Show initial room
    cmd_look(&world);

    st_update_status("Adventure Engine", g_world_name);
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
        char status_right[128];
        snprintf(status_right, sizeof(status_right), "%s | Turns: %d", g_world_name, turn_count);
        st_update_status("Adventure Engine", status_right);
        st_render();

        cmd_free(&cmd);
    }

    st_cleanup();
    printf("Adventure complete. Total turns: %d\n", turn_count);
    return 0;
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
    } else if (cmd_is(cmd, "drop") || cmd_is(cmd, "put")) {
        cmd_drop(world, cmd->noun);
    } else if (cmd_is(cmd, "inventory") || cmd_is(cmd, "i")) {
        cmd_inventory(world);
    } else if (cmd_is(cmd, "examine") || cmd_is(cmd, "x") || cmd_is(cmd, "inspect")) {
        cmd_examine(world, cmd->noun);
    } else if (cmd_is(cmd, "use")) {
        cmd_use(world, cmd->noun);
    } else if (cmd_is(cmd, "save")) {
        cmd_save(world, cmd->noun);
    } else if (cmd_is(cmd, "load")) {
        cmd_load(world, cmd->noun);
    } else if (cmd_is(cmd, "saves")) {
        cmd_saves();
    } else {
        st_add_output("I don't know how to do that. Type 'help' for commands.", ST_CTX_NORMAL);
    }
}

void cmd_help(void) {
    st_add_output("", ST_CTX_NORMAL);
    st_add_output("=== COMMANDS ===", ST_CTX_SPECIAL);
    st_add_output("  look, l              - Look around current room", ST_CTX_NORMAL);
    st_add_output("  go <dir>, <dir>      - Move (north/south/east/west/up/down)", ST_CTX_NORMAL);
    st_add_output("  take <item>          - Pick up an item", ST_CTX_NORMAL);
    st_add_output("  drop <item>          - Drop an item", ST_CTX_NORMAL);
    st_add_output("  examine <item>       - Examine an item closely", ST_CTX_NORMAL);
    st_add_output("  use <item>           - Use an item from inventory", ST_CTX_NORMAL);
    st_add_output("  inventory, i         - Show your inventory", ST_CTX_NORMAL);
    st_add_output("  save <slot>          - Save game to slot", ST_CTX_NORMAL);
    st_add_output("  load <slot>          - Load game from slot", ST_CTX_NORMAL);
    st_add_output("  saves                - List all save slots", ST_CTX_NORMAL);
    st_add_output("  help, ?              - Show this help", ST_CTX_NORMAL);
    st_add_output("  quit, exit           - Quit the game", ST_CTX_NORMAL);
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
    st_add_output(world_get_room_description(world, room), ST_CTX_NORMAL);

    // Show exits
    char exits_buf[256];
    size_t buf_len = sizeof(exits_buf);
    size_t offset = 0;

    // Start with "Exits: "
    offset = snprintf(exits_buf, buf_len, "Exits: ");

    int exit_count = 0;
    for (int i = 0; i < DIR_COUNT; i++) {
        if (room->exits[i] != -1) {
            // Add comma separator if not first exit
            if (exit_count > 0 && offset < buf_len) {
                offset += snprintf(exits_buf + offset, buf_len - offset, ", ");
            }
            // Add direction string with bounds checking
            if (offset < buf_len) {
                offset += snprintf(exits_buf + offset, buf_len - offset, "%s", direction_to_str((Direction)i));
            }
            exit_count++;
        }
    }
    if (exit_count == 0 && offset < buf_len) {
        snprintf(exits_buf + offset, buf_len - offset, "none");
    }
    st_add_output(exits_buf, ST_CTX_COMMENT);

    // Show items
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (room->items[i] != -1) {
            Item *item = &world->items[room->items[i]];
            if (item->visible) {
                char item_buf[128];
                snprintf(item_buf, sizeof(item_buf), "You see: %s", item->name);
                st_add_output(item_buf, ST_CTX_NORMAL);
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

    int dir = str_to_direction(direction);
    if (dir == -1) {
        st_add_output("I don't know that direction.", ST_CTX_NORMAL);
        return;
    }

    char key_needed[32];
    MoveResult result = world_move_ex(world, (Direction)dir, key_needed, sizeof(key_needed));

    switch (result) {
        case MOVE_SUCCESS:
            st_add_output("", ST_CTX_NORMAL);
            cmd_look(world);
            break;
        case MOVE_NO_EXIT:
            st_add_output("You can't go that way.", ST_CTX_NORMAL);
            break;
        case MOVE_LOCKED: {
            char msg[128];
            // Look up the key's display name from the item
            int key_idx = world_find_item(world, key_needed);
            if (key_idx != -1) {
                snprintf(msg, sizeof(msg), "The way %s is locked. You need the %s.",
                        direction, world->items[key_idx].name);
            } else {
                snprintf(msg, sizeof(msg), "The way %s is locked.", direction);
            }
            st_add_output(msg, ST_CTX_NORMAL);
            break;
        }
    }
}

void cmd_take(World *world, const char *item_id) {
    if (!item_id || strlen(item_id) == 0) {
        st_add_output("Take what?", ST_CTX_NORMAL);
        return;
    }

    Item *item = find_item_fuzzy(world, item_id, false, true);
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

    if (world_take_item(world, item->id)) {
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

    Item *item = find_item_fuzzy(world, item_id, true, false);
    if (!item) {
        st_add_output("You don't have that.", ST_CTX_NORMAL);
        return;
    }

    if (world_drop_item(world, item->id)) {
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

void cmd_examine(World *world, const char *item_id) {
    if (!item_id || strlen(item_id) == 0) {
        st_add_output("Examine what?", ST_CTX_NORMAL);
        return;
    }

    // Check both inventory and room
    Item *item = find_item_fuzzy(world, item_id, true, true);

    if (!item) {
        st_add_output("You don't see that here.", ST_CTX_NORMAL);
        return;
    }

    st_add_output("", ST_CTX_NORMAL);
    st_add_output(item->name, ST_CTX_SPECIAL);
    st_add_output(item->description, ST_CTX_NORMAL);
    st_add_output("", ST_CTX_NORMAL);
}

void cmd_save(World *world, const char *slot_name) {
    if (!slot_name || strlen(slot_name) == 0) {
        st_add_output("Save to which slot? Example: save slot1", ST_CTX_NORMAL);
        return;
    }

    if (game_save(world, slot_name, g_world_name)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Game saved to slot '%s'", slot_name);
        st_add_output(buf, ST_CTX_SPECIAL);
    } else {
        st_add_output("Failed to save game.", ST_CTX_NORMAL);
    }
}

void cmd_load(World *world, const char *slot_name) {
    if (!slot_name || strlen(slot_name) == 0) {
        st_add_output("Load from which slot? Example: load slot1", ST_CTX_NORMAL);
        return;
    }

    char loaded_world[64];
    if (game_load(world, slot_name, loaded_world, sizeof(loaded_world))) {
        strncpy(g_world_name, loaded_world, sizeof(g_world_name) - 1);
        st_add_output("", ST_CTX_NORMAL);
        st_add_output("Game loaded successfully!", ST_CTX_SPECIAL);
        st_add_output("", ST_CTX_NORMAL);
        cmd_look(world);
    } else {
        st_add_output("Failed to load game. Slot may not exist.", ST_CTX_NORMAL);
    }
}

void cmd_saves(void) {
    char saves[50][64];
    int count = game_list_saves(saves, 50);

    st_add_output("", ST_CTX_NORMAL);
    st_add_output("=== SAVE SLOTS ===", ST_CTX_SPECIAL);

    if (count == 0) {
        st_add_output("  (no saves found)", ST_CTX_COMMENT);
    } else {
        for (int i = 0; i < count; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf), "  - %s", saves[i]);
            st_add_output(buf, ST_CTX_NORMAL);
        }
    }

    st_add_output("", ST_CTX_NORMAL);
}

void cmd_use(World *world, const char *item_id) {
    if (!item_id || strlen(item_id) == 0) {
        st_add_output("Use what?", ST_CTX_NORMAL);
        return;
    }

    // Find item in inventory only (must have item to use it)
    Item *item = find_item_fuzzy(world, item_id, true, false);

    if (!item) {
        st_add_output("You don't have that.", ST_CTX_NORMAL);
        return;
    }

    // Check if item is usable
    if (item->use_message[0] == '\0') {
        char buf[128];
        snprintf(buf, sizeof(buf), "You can't use the %s.", item->name);
        st_add_output(buf, ST_CTX_NORMAL);
        return;
    }

    // Mark item as used (for conditional descriptions)
    item->used = true;

    // Display use message
    st_add_output("", ST_CTX_NORMAL);
    st_add_output(item->use_message, ST_CTX_SPECIAL);
    st_add_output("", ST_CTX_NORMAL);

    // Remove item if consumable
    if (item->use_consumable) {
        world_remove_from_inventory(world, item->id);
        char buf[128];
        snprintf(buf, sizeof(buf), "The %s is consumed.", item->name);
        st_add_output(buf, ST_CTX_COMMENT);
    }
}
