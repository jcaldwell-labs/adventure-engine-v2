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

// Helper: Parse conditional description key
// Format: description_if(condition) where condition can be:
//   first_visit, visited, has_item=item_id, !has_item=item_id,
//   room_has_item=item_id, item_used=item_id
// Note: Uses '=' separator to avoid conflict with property ':' separator
// Returns true if successfully parsed, fills out the ConditionalDesc
static bool parse_cond_desc_key(const char *key, ConditionalDesc *cond) {
    // Check if key starts with "description_if("
    if (strncmp(key, "description_if(", 15) != 0) {
        return false;
    }

    const char *cond_start = key + 15;
    const char *cond_end = strchr(cond_start, ')');
    if (!cond_end) {
        return false;
    }

    // Extract condition string
    char condition[64];
    size_t cond_len = cond_end - cond_start;
    if (cond_len >= sizeof(condition)) {
        cond_len = sizeof(condition) - 1;
    }
    strncpy(condition, cond_start, cond_len);
    condition[cond_len] = '\0';

    // Check for negation
    cond->negate = false;
    char *cond_str = condition;
    if (cond_str[0] == '!') {
        cond->negate = true;
        cond_str++;
    }

    // Parse condition type (use '=' as separator for item conditions)
    cond->subject[0] = '\0';

    if (strcmp(cond_str, "first_visit") == 0) {
        cond->type = COND_FIRST_VISIT;
    } else if (strcmp(cond_str, "visited") == 0) {
        cond->type = COND_VISITED;
    } else if (strncmp(cond_str, "has_item=", 9) == 0) {
        cond->type = COND_HAS_ITEM;
        strncpy(cond->subject, cond_str + 9, sizeof(cond->subject) - 1);
        cond->subject[sizeof(cond->subject) - 1] = '\0';
    } else if (strncmp(cond_str, "room_has_item=", 14) == 0) {
        cond->type = COND_ROOM_HAS_ITEM;
        strncpy(cond->subject, cond_str + 14, sizeof(cond->subject) - 1);
        cond->subject[sizeof(cond->subject) - 1] = '\0';
    } else if (strncmp(cond_str, "item_used=", 10) == 0) {
        cond->type = COND_ITEM_USED;
        strncpy(cond->subject, cond_str + 10, sizeof(cond->subject) - 1);
        cond->subject[sizeof(cond->subject) - 1] = '\0';
    } else {
        return false;  // Unknown condition type
    }

    return true;
}

// Helper: Apply use command properties to an item
// Note: use_consumable is only valid when use_message is provided
static void apply_use_properties(Item *item, const char *use_message, bool use_consumable) {
    if (use_message[0] != '\0') {
        strncpy(item->use_message, use_message, sizeof(item->use_message) - 1);
        item->use_message[sizeof(item->use_message) - 1] = '\0';
        item->use_consumable = use_consumable;
    } else {
        // No use message means item is not usable, so cannot be consumable
        item->use_message[0] = '\0';
        item->use_consumable = false;
    }
}

// Helper: Parse locked_exits string "north=iron_key, east=master_key"
// Note: Key validation is deferred to end of load since items may be defined after rooms
static void parse_locked_exits(World *world, int room_idx, const char *exits_str) {
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
            char *key_id = trim(equals + 1);

            int dir = str_to_direction(dir_str);
            if (dir != -1) {
                // Lock the exit - key validation happens at end of load
                world_lock_exit(world, room_idx, (Direction)dir, key_id);
            } else {
                fprintf(stderr, "Warning: Room '%s' has invalid locked direction '%s'\n",
                        world->rooms[room_idx].id, dir_str);
            }
        }

        token = strtok_r(NULL, ",", &saveptr);
    }
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
    char prop_locked_exits[512] = "";
    char prop_location[32] = "";
    bool prop_takeable = false;
    // Issue #8: Use command properties
    char prop_use_message[256] = "";
    bool prop_use_consumable = false;
    // Issue #6: Conditional descriptions
    ConditionalDesc prop_cond_descs[MAX_CONDITIONAL_DESCS];
    int prop_cond_desc_count = 0;

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

                // Copy conditional descriptions
                Room *room = &world->rooms[room_idx];
                room->conditional_desc_count = prop_cond_desc_count;
                for (int i = 0; i < prop_cond_desc_count; i++) {
                    room->conditional_descs[i] = prop_cond_descs[i];
                }

                // Parse exits if present
                if (prop_exits[0] != '\0') {
                    parse_exits(world, room_idx, prop_exits);
                }

                // Parse locked_exits if present
                if (prop_locked_exits[0] != '\0') {
                    parse_locked_exits(world, room_idx, prop_locked_exits);
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

                // Set use command properties
                apply_use_properties(&world->items[item_idx], prop_use_message, prop_use_consumable);

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
            prop_locked_exits[0] = '\0';
            prop_location[0] = '\0';
            prop_takeable = false;
            prop_use_message[0] = '\0';
            prop_use_consumable = false;
            prop_cond_desc_count = 0;

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
        // Security: Ensure null termination after all strncpy calls
        if (strcmp(current_section, "WORLD") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(world_name, value, sizeof(world_name) - 1);
                world_name[sizeof(world_name) - 1] = '\0';
            } else if (strcmp(key, "start") == 0) {
                strncpy(world_start, value, sizeof(world_start) - 1);
                world_start[sizeof(world_start) - 1] = '\0';
            }
        } else if (strcmp(current_section, "ROOM") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(prop_name, value, sizeof(prop_name) - 1);
                prop_name[sizeof(prop_name) - 1] = '\0';
            } else if (strcmp(key, "description") == 0) {
                strncpy(prop_description, value, sizeof(prop_description) - 1);
                prop_description[sizeof(prop_description) - 1] = '\0';
            } else if (strcmp(key, "exits") == 0) {
                strncpy(prop_exits, value, sizeof(prop_exits) - 1);
                prop_exits[sizeof(prop_exits) - 1] = '\0';
            } else if (strcmp(key, "locked_exits") == 0) {
                strncpy(prop_locked_exits, value, sizeof(prop_locked_exits) - 1);
                prop_locked_exits[sizeof(prop_locked_exits) - 1] = '\0';
            } else if (strncmp(key, "description_if(", 15) == 0) {
                // Issue #6: Parse conditional description
                if (prop_cond_desc_count < MAX_CONDITIONAL_DESCS) {
                    ConditionalDesc *cond = &prop_cond_descs[prop_cond_desc_count];
                    if (parse_cond_desc_key(key, cond)) {
                        strncpy(cond->description, value, sizeof(cond->description) - 1);
                        cond->description[sizeof(cond->description) - 1] = '\0';
                        prop_cond_desc_count++;
                    } else {
                        fprintf(stderr, "Warning: Invalid conditional description '%s' in room '%s'\n",
                                key, current_id);
                    }
                } else {
                    fprintf(stderr, "Warning: Too many conditional descriptions in room '%s'\n",
                            current_id);
                }
            }
        } else if (strcmp(current_section, "ITEM") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(prop_name, value, sizeof(prop_name) - 1);
                prop_name[sizeof(prop_name) - 1] = '\0';
            } else if (strcmp(key, "description") == 0) {
                strncpy(prop_description, value, sizeof(prop_description) - 1);
                prop_description[sizeof(prop_description) - 1] = '\0';
            } else if (strcmp(key, "takeable") == 0) {
                prop_takeable = parse_bool(value);
            } else if (strcmp(key, "location") == 0) {
                strncpy(prop_location, value, sizeof(prop_location) - 1);
                prop_location[sizeof(prop_location) - 1] = '\0';
            } else if (strcmp(key, "use_message") == 0) {
                strncpy(prop_use_message, value, sizeof(prop_use_message) - 1);
                prop_use_message[sizeof(prop_use_message) - 1] = '\0';
            } else if (strcmp(key, "use_consumable") == 0) {
                prop_use_consumable = parse_bool(value);
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
        if (room_idx != -1) {
            // Copy conditional descriptions
            Room *room = &world->rooms[room_idx];
            room->conditional_desc_count = prop_cond_desc_count;
            for (int i = 0; i < prop_cond_desc_count; i++) {
                room->conditional_descs[i] = prop_cond_descs[i];
            }

            if (prop_exits[0] != '\0') {
                parse_exits(world, room_idx, prop_exits);
            }
            if (prop_locked_exits[0] != '\0') {
                parse_locked_exits(world, room_idx, prop_locked_exits);
            }
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
            // Set use command properties
            apply_use_properties(&world->items[item_idx], prop_use_message, prop_use_consumable);

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

    // Validate locked exits reference existing items
    for (int i = 0; i < world->room_count; i++) {
        for (int dir = 0; dir < DIR_COUNT; dir++) {
            if (world->rooms[i].locked_exits[dir][0] != '\0') {
                int key_item = world_find_item(world, world->rooms[i].locked_exits[dir]);
                if (key_item == -1) {
                    fprintf(stderr, "Warning: Room '%s' has locked exit '%s' requiring non-existent key '%s'\n",
                            world->rooms[i].id, direction_to_str((Direction)dir),
                            world->rooms[i].locked_exits[dir]);
                }
            }
        }
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
