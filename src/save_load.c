/*
 * Adventure Engine - Save/Load System Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include "save_load.h"

#define SAVE_DIR_NAME ".adventure-saves"
#define SAVE_VERSION 2  // v2 adds exit_unlocked state

// Get the save directory path
static void get_save_dir(char *buffer, size_t buffer_size) {
    const char *home = getenv("HOME");
    if (home) {
        snprintf(buffer, buffer_size, "%s/%s", home, SAVE_DIR_NAME);
    } else {
        snprintf(buffer, buffer_size, "%s", SAVE_DIR_NAME);
    }
}

// Ensure save directory exists
static bool ensure_save_dir(void) {
    char save_dir[512];
    get_save_dir(save_dir, sizeof(save_dir));

    struct stat st = {0};
    if (stat(save_dir, &st) == -1) {
        if (mkdir(save_dir, 0700) != 0) {
            return false;
        }
    }

    return true;
}

bool is_safe_filename(const char *filename) {
    if (!filename || filename[0] == '\0') {
        return false;
    }

    // Reject if too long (reasonable filename limit)
    if (strlen(filename) > 64) {
        return false;
    }

    // Check each character
    for (const char *p = filename; *p != '\0'; p++) {
        // Allow only: alphanumeric, underscore, hyphen
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-') {
            return false;
        }
    }

    // Reject directory traversal patterns
    if (strstr(filename, "..") != NULL) {
        return false;
    }

    // Reject absolute paths
    if (filename[0] == '/' || filename[0] == '\\') {
        return false;
    }

    return true;
}

void get_save_path(const char *slot_name, char *buffer, size_t buffer_size) {
    char save_dir[512];
    get_save_dir(save_dir, sizeof(save_dir));
    snprintf(buffer, buffer_size, "%s/%s.sav", save_dir, slot_name);
}

bool save_exists(const char *slot_name) {
    char path[512];
    get_save_path(slot_name, path, sizeof(path));

    struct stat st = {0};
    return stat(path, &st) == 0;
}

bool game_save(const World *world, const char *slot_name, const char *world_name) {
    // Validate slot_name to prevent path traversal
    if (!is_safe_filename(slot_name)) {
        fprintf(stderr, "Error: Invalid save slot name '%s'. Only alphanumeric, underscore, and hyphen allowed.\n", slot_name);
        return false;
    }

    if (!ensure_save_dir()) {
        return false;
    }

    char path[512];
    get_save_path(slot_name, path, sizeof(path));

    FILE *file = fopen(path, "w");
    if (!file) {
        return false;
    }

    // Write header
    fprintf(file, "# Adventure Engine Save File\n");
    fprintf(file, "VERSION: %d\n", SAVE_VERSION);
    fprintf(file, "WORLD: %s\n", world_name);
    fprintf(file, "\n");

    // Write current state
    fprintf(file, "[STATE]\n");
    fprintf(file, "current_room: %d\n", world->current_room);
    fprintf(file, "room_count: %d\n", world->room_count);
    fprintf(file, "item_count: %d\n", world->item_count);
    fprintf(file, "\n");

    // Write inventory
    fprintf(file, "[INVENTORY]\n");
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (world->inventory[i] != -1) {
            fprintf(file, "%d\n", world->inventory[i]);
        }
    }
    fprintf(file, "\n");

    // Write room visited states
    fprintf(file, "[VISITED]\n");
    for (int i = 0; i < world->room_count; i++) {
        fprintf(file, "%d\n", world->rooms[i].visited ? 1 : 0);
    }
    fprintf(file, "\n");

    // Write room item placements
    fprintf(file, "[ROOM_ITEMS]\n");
    for (int i = 0; i < world->room_count; i++) {
        fprintf(file, "ROOM:%d:", i);
        int first = 1;
        for (int j = 0; j < MAX_ITEMS; j++) {
            if (world->rooms[i].items[j] != -1) {
                if (!first) fprintf(file, ",");
                fprintf(file, "%d", world->rooms[i].items[j]);
                first = 0;
            }
        }
        fprintf(file, "\n");
    }
    fprintf(file, "\n");

    // Write unlocked exits state (v2+)
    fprintf(file, "[UNLOCKED_EXITS]\n");
    for (int i = 0; i < world->room_count; i++) {
        fprintf(file, "ROOM:%d:", i);
        for (int j = 0; j < DIR_COUNT; j++) {
            fprintf(file, "%d", world->rooms[i].exit_unlocked[j] ? 1 : 0);
            if (j < DIR_COUNT - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return true;
}

bool game_load(World *world, const char *slot_name, char *world_name, size_t world_name_size) {
    // Validate slot_name to prevent path traversal
    if (!is_safe_filename(slot_name)) {
        fprintf(stderr, "Error: Invalid save slot name '%s'. Only alphanumeric, underscore, and hyphen allowed.\n", slot_name);
        return false;
    }

    char path[512];
    get_save_path(slot_name, path, sizeof(path));

    FILE *file = fopen(path, "r");
    if (!file) {
        return false;
    }

    char line[512];
    char section[64] = "";
    int version = 0;
    int room_count = 0;
    int item_count = 0;
    int current_room = 0;

    // Temporary storage for loaded data
    int inventory[MAX_INVENTORY];
    int visited[MAX_ROOMS];
    int room_items[MAX_ROOMS][MAX_ITEMS];

    // Temporary storage for unlocked exits (v2+)
    bool unlocked_exits[MAX_ROOMS][DIR_COUNT];

    // Initialize
    for (int i = 0; i < MAX_INVENTORY; i++) inventory[i] = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        visited[i] = 0;
        for (int j = 0; j < MAX_ITEMS; j++) {
            room_items[i][j] = -1;
        }
        for (int j = 0; j < DIR_COUNT; j++) {
            unlocked_exits[i][j] = false;
        }
    }

    int inv_idx = 0;
    int visited_idx = 0;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        // Check for section
        if (line[0] == '[') {
            sscanf(line, "[%63[^]]]", section);
            continue;
        }

        // Parse based on section
        if (strlen(section) == 0) {
            // Header properties
            if (strncmp(line, "VERSION:", 8) == 0) {
                sscanf(line + 8, "%d", &version);
            } else if (strncmp(line, "WORLD:", 6) == 0) {
                strncpy(world_name, line + 7, world_name_size - 1);
                world_name[world_name_size - 1] = '\0';
            }
        } else if (strcmp(section, "STATE") == 0) {
            if (strncmp(line, "current_room:", 13) == 0) {
                sscanf(line + 13, "%d", &current_room);
            } else if (strncmp(line, "room_count:", 11) == 0) {
                sscanf(line + 11, "%d", &room_count);
            } else if (strncmp(line, "item_count:", 11) == 0) {
                sscanf(line + 11, "%d", &item_count);
            }
        } else if (strcmp(section, "INVENTORY") == 0) {
            int item_id;
            if (sscanf(line, "%d", &item_id) == 1) {
                if (inv_idx < MAX_INVENTORY) {
                    inventory[inv_idx++] = item_id;
                }
            }
        } else if (strcmp(section, "VISITED") == 0) {
            int vis;
            if (sscanf(line, "%d", &vis) == 1) {
                if (visited_idx < MAX_ROOMS) {
                    visited[visited_idx++] = vis;
                }
            }
        } else if (strcmp(section, "ROOM_ITEMS") == 0) {
            if (strncmp(line, "ROOM:", 5) == 0) {
                int room_idx;
                char items_str[512];
                if (sscanf(line, "ROOM:%d:%s", &room_idx, items_str) >= 1) {
                    if (room_idx >= 0 && room_idx < MAX_ROOMS) {
                        // Parse comma-separated item list
                        char *colon = strchr(line, ':');
                        if (colon) {
                            colon = strchr(colon + 1, ':');
                            if (colon && colon[1] != '\0') {
                                char *items = colon + 1;
                                int item_slot = 0;
                                char *saveptr;
                                char *token = strtok_r(items, ",", &saveptr);
                                while (token && item_slot < MAX_ITEMS) {
                                    int item_id;
                                    if (sscanf(token, "%d", &item_id) == 1) {
                                        room_items[room_idx][item_slot++] = item_id;
                                    }
                                    token = strtok_r(NULL, ",", &saveptr);
                                }
                            }
                        }
                    }
                }
            }
        } else if (strcmp(section, "UNLOCKED_EXITS") == 0) {
            // Parse: ROOM:0:0,0,1,0,0,0 (one value per direction)
            if (strncmp(line, "ROOM:", 5) == 0) {
                int room_idx;
                if (sscanf(line, "ROOM:%d:", &room_idx) == 1) {
                    if (room_idx >= 0 && room_idx < MAX_ROOMS) {
                        char *colon = strchr(line, ':');
                        if (colon) {
                            colon = strchr(colon + 1, ':');
                            if (colon && colon[1] != '\0') {
                                char *exits = colon + 1;
                                int dir_slot = 0;
                                char *saveptr;
                                char *token = strtok_r(exits, ",", &saveptr);
                                while (token && dir_slot < DIR_COUNT) {
                                    int unlocked;
                                    if (sscanf(token, "%d", &unlocked) == 1) {
                                        unlocked_exits[room_idx][dir_slot] = (unlocked != 0);
                                    }
                                    dir_slot++;
                                    token = strtok_r(NULL, ",", &saveptr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    fclose(file);

    // Validate version (accept v1 and v2 saves, v1 saves won't have unlocked exits)
    if (version < 1 || version > SAVE_VERSION) {
        return false;
    }

    // Apply loaded state to world
    world->current_room = current_room;

    // Apply inventory
    for (int i = 0; i < MAX_INVENTORY; i++) {
        world->inventory[i] = inventory[i];
    }

    // Apply visited states and room items
    // Use the minimum of world's room count and save file's room count to avoid
    // applying states to rooms that don't exist in the current world structure.
    // If world->room_count is 0 (loading into empty world), use save file's count.
    int rooms_to_apply = world->room_count > 0 ? world->room_count : room_count;
    if (room_count < rooms_to_apply) rooms_to_apply = room_count;
    if (rooms_to_apply > MAX_ROOMS) rooms_to_apply = MAX_ROOMS;

    // Apply visited states
    for (int i = 0; i < rooms_to_apply; i++) {
        world->rooms[i].visited = visited[i];
    }

    // Apply room items using the same room count
    for (int i = 0; i < rooms_to_apply; i++) {
        for (int j = 0; j < MAX_ITEMS; j++) {
            world->rooms[i].items[j] = room_items[i][j];
        }
    }

    // Apply unlocked exits state (v2+)
    for (int i = 0; i < rooms_to_apply; i++) {
        for (int j = 0; j < DIR_COUNT; j++) {
            world->rooms[i].exit_unlocked[j] = unlocked_exits[i][j];
        }
    }

    return true;
}

int game_list_saves(char saves[][64], int max_saves) {
    char save_dir[512];
    get_save_dir(save_dir, sizeof(save_dir));

    DIR *dir = opendir(save_dir);
    if (!dir) {
        return 0;
    }

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < max_saves) {
        // Check if file ends with .sav
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".sav") == 0) {
            // Copy filename without extension
            size_t name_len = ext - entry->d_name;
            if (name_len < 64) {
                strncpy(saves[count], entry->d_name, name_len);
                saves[count][name_len] = '\0';
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

bool game_delete_save(const char *slot_name) {
    // Validate slot_name to prevent path traversal
    if (!is_safe_filename(slot_name)) {
        fprintf(stderr, "Error: Invalid save slot name '%s'. Only alphanumeric, underscore, and hyphen allowed.\n", slot_name);
        return false;
    }

    char path[512];
    get_save_path(slot_name, path, sizeof(path));

    return unlink(path) == 0;
}
