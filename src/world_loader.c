/*
 * Adventure Engine - World Loader Implementation
 * Parses .world files and creates World structures
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "world_loader.h"

#define MAX_LINE 1024

// Helper: Trim whitespace (modifies string in place)
static char* trim(char *str) {
    while (isspace(*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = 0;

    return str;
}

// Helper: Check if line is comment or empty
static bool is_empty_or_comment(char *line) {
    char *trimmed = trim(line);
    return trimmed[0] == '\0' || trimmed[0] == '#';
}

// Helper: Parse section header [TYPE:id] or [TYPE]
static bool parse_section_header(const char *line, char *type, char *id) {
    type[0] = '\0';
    id[0] = '\0';

    const char *start = strchr(line, '[');
    const char *end = strchr(line, ']');

    if (!start || !end || end <= start) return false;

    start++; // Skip '['

    const char *colon = strchr(start, ':');

    if (colon && colon < end) {
        // Has ID: [TYPE:id]
        size_t type_len = colon - start;
        strncpy(type, start, type_len);
        type[type_len] = '\0';

        size_t id_len = end - colon - 1;
        strncpy(id, colon + 1, id_len);
        id[id_len] = '\0';
    } else {
        // No ID: [TYPE]
        size_t type_len = end - start;
        strncpy(type, start, type_len);
        type[type_len] = '\0';
    }

    return true;
}

// Helper: Parse property line "key: value"
static bool parse_property(const char *line, char *key, char *value) {
    key[0] = '\0';
    value[0] = '\0';

    const char *colon = strchr(line, ':');
    if (!colon) return false;

    // Extract key
    size_t key_len = colon - line;
    strncpy(key, line, key_len);
    key[key_len] = '\0';

    // Trim key
    char *key_trimmed = trim(key);
    if (key_trimmed != key) {
        memmove(key, key_trimmed, strlen(key_trimmed) + 1);
    }

    // Extract value (limit to 511 chars to ensure null-termination)
    strncpy(value, colon + 1, 511);
    value[511] = '\0';
    char *value_trimmed = trim(value);
    if (value_trimmed != value) {
        memmove(value, value_trimmed, strlen(value_trimmed) + 1);
    }

    return true;
}

// Helper: Parse boolean value
static bool parse_bool(const char *str) {
    if (strcmp(str, "yes") == 0 || strcmp(str, "true") == 0 || strcmp(str, "1") == 0) {
        return true;
    }
    return false;
}

// Helper: Parse exits string "north=hall, east=chamber"
static void parse_exits(World *world, int room_idx, const char *exits_str) {
    char buffer[512];
    strncpy(buffer, exits_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *saveptr;
    char *token = strtok_r(buffer, ",", &saveptr);
    while (token) {
        token = trim(token);

        char *equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            char *dir_str = trim(token);
            char *room_id = trim(equals + 1);

            int dir = str_to_direction(dir_str);
            if (dir != -1) {
                int target_room = world_find_room(world, room_id);
                if (target_room != -1) {
                    world_connect_rooms(world, room_idx, (Direction)dir, target_room);
                } else {
                    fprintf(stderr, "Warning: Room '%s' has invalid exit '%s' to non-existent room '%s'\n",
                            world->rooms[room_idx].id, dir_str, room_id);
                }
            } else {
                fprintf(stderr, "Warning: Room '%s' has invalid direction '%s'\n",
                        world->rooms[room_idx].id, dir_str);
            }
        }

        token = strtok_r(NULL, ",", &saveptr);
    }
}

// Main loader function
bool world_load_from_file(World *world, const char *filename, LoadError *error) {
    error->has_error = false;
    error->line_number = 0;
    error->message[0] = '\0';

    FILE *file = fopen(filename, "r");
    if (!file) {
        error->has_error = true;
        error->line_number = 0;
        snprintf(error->message, sizeof(error->message), "Cannot open file: %s", filename);
        return false;
    }

    world_init(world);

    char line[MAX_LINE];
    int line_num = 0;

    char current_section[32] = "";
    char current_id[32] = "";

    // Properties for current item/room being parsed
    char prop_name[64] = "";
    char prop_description[512] = "";
    char prop_exits[512] = "";
    char prop_location[32] = "";
    bool prop_takeable = false;

    char world_name[64] = "Untitled";
    char world_start[32] = "";

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        // Skip comments and empty lines
        if (is_empty_or_comment(line)) {
            continue;
        }

        // Check for section header
        if (line[0] == '[') {
            // Save previous section if needed
            if (strcmp(current_section, "ROOM") == 0 && current_id[0] != '\0') {
                if (prop_name[0] == '\0' || prop_description[0] == '\0') {
                    error->has_error = true;
                    error->line_number = line_num;
                    snprintf(error->message, sizeof(error->message),
                             "Room '%s' missing required fields", current_id);
                    fclose(file);
                    return false;
                }

                int room_idx = world_add_room(world, current_id, prop_name, prop_description);
                if (room_idx == -1) {
                    error->has_error = true;
                    error->line_number = line_num;
                    snprintf(error->message, sizeof(error->message),
                             "Failed to add room '%s' (too many rooms?)", current_id);
                    fclose(file);
                    return false;
                }

                // Parse exits if present
                if (prop_exits[0] != '\0') {
                    parse_exits(world, room_idx, prop_exits);
                }
            } else if (strcmp(current_section, "ITEM") == 0 && current_id[0] != '\0') {
                if (prop_name[0] == '\0' || prop_description[0] == '\0' || prop_location[0] == '\0') {
                    error->has_error = true;
                    error->line_number = line_num;
                    snprintf(error->message, sizeof(error->message),
                             "Item '%s' missing required fields", current_id);
                    fclose(file);
                    return false;
                }

                int item_idx = world_add_item(world, current_id, prop_name, prop_description, prop_takeable);
                if (item_idx == -1) {
                    error->has_error = true;
                    error->line_number = line_num;
                    snprintf(error->message, sizeof(error->message),
                             "Failed to add item '%s' (too many items?)", current_id);
                    fclose(file);
                    return false;
                }

                // Place item in room
                int room_idx = world_find_room(world, prop_location);
                if (room_idx != -1) {
                    world_place_item(world, item_idx, room_idx);
                }
            }

            // Parse new section header
            char section_type[32];
            char section_id[32];

            if (!parse_section_header(line, section_type, section_id)) {
                error->has_error = true;
                error->line_number = line_num;
                snprintf(error->message, sizeof(error->message), "Invalid section header");
                fclose(file);
                return false;
            }

            strncpy(current_section, section_type, sizeof(current_section) - 1);
            current_section[sizeof(current_section) - 1] = '\0';
            strncpy(current_id, section_id, sizeof(current_id) - 1);
            current_id[sizeof(current_id) - 1] = '\0';

            // Reset properties
            prop_name[0] = '\0';
            prop_description[0] = '\0';
            prop_exits[0] = '\0';
            prop_location[0] = '\0';
            prop_takeable = false;

            continue;
        }

        // Parse property
        char key[64];
        char value[512];

        if (!parse_property(line, key, value)) {
            error->has_error = true;
            error->line_number = line_num;
            snprintf(error->message, sizeof(error->message), "Invalid property line");
            fclose(file);
            return false;
        }

        // Handle property based on current section
        if (strcmp(current_section, "WORLD") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(world_name, value, sizeof(world_name) - 1);
            } else if (strcmp(key, "start") == 0) {
                strncpy(world_start, value, sizeof(world_start) - 1);
            }
        } else if (strcmp(current_section, "ROOM") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(prop_name, value, sizeof(prop_name) - 1);
            } else if (strcmp(key, "description") == 0) {
                strncpy(prop_description, value, sizeof(prop_description) - 1);
            } else if (strcmp(key, "exits") == 0) {
                strncpy(prop_exits, value, sizeof(prop_exits) - 1);
            }
        } else if (strcmp(current_section, "ITEM") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(prop_name, value, sizeof(prop_name) - 1);
            } else if (strcmp(key, "description") == 0) {
                strncpy(prop_description, value, sizeof(prop_description) - 1);
            } else if (strcmp(key, "takeable") == 0) {
                prop_takeable = parse_bool(value);
            } else if (strcmp(key, "location") == 0) {
                strncpy(prop_location, value, sizeof(prop_location) - 1);
            }
        }
    }

    // Handle last section
    if (strcmp(current_section, "ROOM") == 0 && current_id[0] != '\0') {
        if (prop_name[0] == '\0' || prop_description[0] == '\0') {
            error->has_error = true;
            error->line_number = line_num;
            snprintf(error->message, sizeof(error->message),
                     "Room '%s' missing required fields", current_id);
            fclose(file);
            return false;
        }

        int room_idx = world_add_room(world, current_id, prop_name, prop_description);
        if (room_idx != -1 && prop_exits[0] != '\0') {
            parse_exits(world, room_idx, prop_exits);
        }
    } else if (strcmp(current_section, "ITEM") == 0 && current_id[0] != '\0') {
        if (prop_name[0] == '\0' || prop_description[0] == '\0' || prop_location[0] == '\0') {
            error->has_error = true;
            error->line_number = line_num;
            snprintf(error->message, sizeof(error->message),
                     "Item '%s' missing required fields", current_id);
            fclose(file);
            return false;
        }

        int item_idx = world_add_item(world, current_id, prop_name, prop_description, prop_takeable);
        if (item_idx != -1) {
            int room_idx = world_find_room(world, prop_location);
            if (room_idx != -1) {
                world_place_item(world, item_idx, room_idx);
            }
        }
    }

    fclose(file);

    // Set starting room
    if (world_start[0] != '\0') {
        int start_room = world_find_room(world, world_start);
        if (start_room != -1) {
            world->current_room = start_room;
            world->rooms[start_room].visited = true;
        }
    } else if (world->room_count > 0) {
        // Default to first room
        world->current_room = 0;
        world->rooms[0].visited = true;
    }

    // Validate world
    if (world->room_count == 0) {
        error->has_error = true;
        error->line_number = 0;
        snprintf(error->message, sizeof(error->message), "No rooms defined in world");
        return false;
    }

    return true;
}

const char* world_loader_get_error(const LoadError *error) {
    static char buffer[512];
    if (error->line_number > 0) {
        snprintf(buffer, sizeof(buffer), "Line %d: %s", error->line_number, error->message);
    } else {
        snprintf(buffer, sizeof(buffer), "%s", error->message);
    }
    return buffer;
}
