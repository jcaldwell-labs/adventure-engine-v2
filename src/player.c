#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define PLAYER_DIR "/tmp/adventure-players"

// Convert role to string
const char* role_to_string(PlayerRole role) {
    switch (role) {
        case ROLE_NONE:      return "NONE";
        case ROLE_LEADER:    return "LEADER";
        case ROLE_SCOUT:     return "SCOUT";
        case ROLE_ENGINEER:  return "ENGINEER";
        case ROLE_MEDIC:     return "MEDIC";
        case ROLE_DIPLOMAT:  return "DIPLOMAT";
        case ROLE_SPECIALIST: return "SPECIALIST";
        default:             return "UNKNOWN";
    }
}

// Convert string to role
PlayerRole role_from_string(const char* role_str) {
    if (!role_str) return ROLE_NONE;

    if (strcasecmp(role_str, "LEADER") == 0) return ROLE_LEADER;
    if (strcasecmp(role_str, "SCOUT") == 0) return ROLE_SCOUT;
    if (strcasecmp(role_str, "ENGINEER") == 0) return ROLE_ENGINEER;
    if (strcasecmp(role_str, "MEDIC") == 0) return ROLE_MEDIC;
    if (strcasecmp(role_str, "DIPLOMAT") == 0) return ROLE_DIPLOMAT;
    if (strcasecmp(role_str, "SPECIALIST") == 0) return ROLE_SPECIALIST;

    return ROLE_NONE;
}

// Assign abilities based on role
void role_assign_abilities(Player* player) {
    if (!player) return;

    // Reset all abilities
    player->can_inspect = false;
    player->can_unlock = false;
    player->can_heal = false;
    player->can_negotiate = false;
    player->can_lead = false;

    // Assign based on role
    switch (player->role) {
        case ROLE_LEADER:
            player->can_lead = true;
            player->can_inspect = true;
            break;

        case ROLE_SCOUT:
            player->can_inspect = true;
            break;

        case ROLE_ENGINEER:
            player->can_unlock = true;
            break;

        case ROLE_MEDIC:
            player->can_heal = true;
            break;

        case ROLE_DIPLOMAT:
            player->can_negotiate = true;
            player->can_inspect = true;
            break;

        case ROLE_SPECIALIST:
            // Wildcard - gets all abilities but reduced effectiveness
            player->can_inspect = true;
            player->can_unlock = true;
            player->can_heal = true;
            player->can_negotiate = true;
            break;

        default:
            break;
    }
}

// Check if player can perform action
bool role_can_perform_action(const Player* player, const char* action) {
    if (!player || !action) return false;

    if (strcasecmp(action, "inspect") == 0) return player->can_inspect;
    if (strcasecmp(action, "unlock") == 0) return player->can_unlock;
    if (strcasecmp(action, "heal") == 0) return player->can_heal;
    if (strcasecmp(action, "negotiate") == 0) return player->can_negotiate;
    if (strcasecmp(action, "lead") == 0) return player->can_lead;

    return true;  // Unknown actions allowed by default
}

// Create a new player
Player* player_create(const char* username, const char* session_id, PlayerRole role) {
    if (!username || !session_id) {
        return NULL;
    }

    Player* player = (Player*)calloc(1, sizeof(Player));
    if (!player) {
        perror("Failed to allocate player");
        return NULL;
    }

    strncpy(player->username, username, MAX_USERNAME - 1);
    strncpy(player->session_id, session_id, 63);
    player->player_number = -1;  // Assigned when added to registry

    player->role = role;
    player->state = PLAYER_DISCONNECTED;

    // Assign role abilities
    role_assign_abilities(player);

    // Connection info
    snprintf(player->socket_path, sizeof(player->socket_path),
             "/tmp/adventure-engine/%s-%s.sock", session_id, username);
    player->connected_at = 0;
    player->last_activity = 0;
    player->last_heartbeat = 0;

    // Initialize stats
    player->commands_issued = 0;
    player->items_found = 0;
    player->puzzles_contributed = 0;
    player->team_actions = 0;

    // Initialize status
    player->health = 100;
    player->energy = 100;
    player->is_ready = false;

    return player;
}

// Destroy player
bool player_destroy(Player* player) {
    if (!player) {
        return false;
    }

    free(player);
    return true;
}

// Connect player
bool player_connect(Player* player) {
    if (!player) {
        return false;
    }

    player->state = PLAYER_CONNECTED;
    player->connected_at = time(NULL);
    player->last_activity = player->connected_at;
    player->last_heartbeat = player->connected_at;

    return true;
}

// Disconnect player
bool player_disconnect(Player* player) {
    if (!player) {
        return false;
    }

    player->state = PLAYER_DISCONNECTED;
    return true;
}

// Activate player (start playing)
bool player_activate(Player* player) {
    if (!player || player->state != PLAYER_CONNECTED) {
        return false;
    }

    player->state = PLAYER_ACTIVE;
    player->last_activity = time(NULL);
    return true;
}

// Update heartbeat
bool player_heartbeat(Player* player) {
    if (!player) {
        return false;
    }

    player->last_heartbeat = time(NULL);
    player->last_activity = player->last_heartbeat;
    return true;
}

// Save player data
bool player_save(const Player* player) {
    if (!player) {
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "/tmp/adventure-players/%s-%s.player",
             player->session_id, player->username);

    FILE* fp = fopen(path, "w");
    if (!fp) {
        return false;
    }

    fprintf(fp, "[PLAYER]\n");
    fprintf(fp, "username: %s\n", player->username);
    fprintf(fp, "session: %s\n", player->session_id);
    fprintf(fp, "number: %d\n", player->player_number);
    fprintf(fp, "role: %s\n", role_to_string(player->role));
    fprintf(fp, "health: %d\n", player->health);
    fprintf(fp, "energy: %d\n", player->energy);
    fprintf(fp, "commands: %d\n", player->commands_issued);
    fprintf(fp, "items_found: %d\n", player->items_found);
    fprintf(fp, "puzzles: %d\n", player->puzzles_contributed);
    fprintf(fp, "team_actions: %d\n", player->team_actions);

    fclose(fp);
    return true;
}

// Load player data
bool player_load(Player* player, const char* session_id, const char* username) {
    if (!player || !session_id || !username) {
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "/tmp/adventure-players/%s-%s.player",
             session_id, username);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    char line[256];
    // Security: Initialize buffers to prevent use of uninitialized data
    char key[64] = {0};
    char value[192] = {0};

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '[' || line[0] == '\n') continue;

        // Security: Reset buffers before each sscanf to prevent stale data
        key[0] = '\0';
        value[0] = '\0';

        if (sscanf(line, "%63[^:]: %191[^\n]", key, value) == 2) {
            // Security: Ensure null termination after strncpy
            if (strcmp(key, "username") == 0) {
                strncpy(player->username, value, MAX_USERNAME - 1);
                player->username[MAX_USERNAME - 1] = '\0';
            } else if (strcmp(key, "session") == 0) {
                strncpy(player->session_id, value, 63);
                player->session_id[63] = '\0';
            } else if (strcmp(key, "number") == 0) {
                player->player_number = atoi(value);
            } else if (strcmp(key, "role") == 0) {
                player->role = role_from_string(value);
                role_assign_abilities(player);
            } else if (strcmp(key, "health") == 0) {
                player->health = atoi(value);
            } else if (strcmp(key, "energy") == 0) {
                player->energy = atoi(value);
            } else if (strcmp(key, "commands") == 0) {
                player->commands_issued = atoi(value);
            } else if (strcmp(key, "items_found") == 0) {
                player->items_found = atoi(value);
            } else if (strcmp(key, "puzzles") == 0) {
                player->puzzles_contributed = atoi(value);
            } else if (strcmp(key, "team_actions") == 0) {
                player->team_actions = atoi(value);
            }
        }
    }

    fclose(fp);
    return true;
}

// Validate player
bool player_validate(const Player* player) {
    if (!player) return false;
    if (player->username[0] == '\0') return false;
    if (player->session_id[0] == '\0') return false;
    return true;
}

// Reset player stats
void player_reset_stats(Player* player) {
    if (!player) return;

    player->commands_issued = 0;
    player->items_found = 0;
    player->puzzles_contributed = 0;
    player->team_actions = 0;
    player->health = 100;
    player->energy = 100;
    player->is_ready = false;
}

// ============================================================================
// PLAYER REGISTRY
// ============================================================================

// Initialize player registry
PlayerRegistry* player_registry_init(void) {
    PlayerRegistry* registry = (PlayerRegistry*)calloc(1, sizeof(PlayerRegistry));
    if (!registry) {
        perror("Failed to allocate player registry");
        return NULL;
    }

    registry->player_count = 0;
    registry->connected_count = 0;
    registry->active_count = 0;

    return registry;
}

// Add player to registry
bool player_registry_add(PlayerRegistry* registry, Player* player) {
    if (!registry || !player) {
        return false;
    }

    if (registry->player_count >= MAX_PLAYERS) {
        fprintf(stderr, "Player registry is full\n");
        return false;
    }

    // Assign player number
    player->player_number = registry->player_count;

    // Copy player data
    memcpy(&registry->players[registry->player_count], player, sizeof(Player));
    registry->player_count++;

    player_registry_update_states(registry);
    return true;
}

// Remove player from registry
bool player_registry_remove(PlayerRegistry* registry, const char* username) {
    if (!registry || !username) {
        return false;
    }

    for (int i = 0; i < registry->player_count; i++) {
        if (strcmp(registry->players[i].username, username) == 0) {
            // Shift remaining players
            for (int j = i; j < registry->player_count - 1; j++) {
                memcpy(&registry->players[j], &registry->players[j + 1], sizeof(Player));
                // Update player numbers
                registry->players[j].player_number = j;
            }
            registry->player_count--;
            player_registry_update_states(registry);
            return true;
        }
    }

    return false;
}

// Find player in registry
Player* player_registry_find(PlayerRegistry* registry, const char* username) {
    if (!registry || !username) {
        return NULL;
    }

    for (int i = 0; i < registry->player_count; i++) {
        if (strcmp(registry->players[i].username, username) == 0) {
            return &registry->players[i];
        }
    }

    return NULL;
}

// Get players by role
int player_registry_get_by_role(PlayerRegistry* registry, PlayerRole role,
                                Player** out_players, int max) {
    if (!registry || !out_players) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < registry->player_count && count < max; i++) {
        if (registry->players[i].role == role) {
            out_players[count++] = &registry->players[i];
        }
    }

    return count;
}

// Get active players
int player_registry_get_active(PlayerRegistry* registry, Player** out_players, int max) {
    if (!registry || !out_players) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < registry->player_count && count < max; i++) {
        if (registry->players[i].state == PLAYER_ACTIVE) {
            out_players[count++] = &registry->players[i];
        }
    }

    return count;
}

// Update registry state counts
void player_registry_update_states(PlayerRegistry* registry) {
    if (!registry) return;

    registry->connected_count = 0;
    registry->active_count = 0;

    for (int i = 0; i < registry->player_count; i++) {
        if (registry->players[i].state == PLAYER_CONNECTED ||
            registry->players[i].state == PLAYER_ACTIVE) {
            registry->connected_count++;
        }

        if (registry->players[i].state == PLAYER_ACTIVE) {
            registry->active_count++;
        }
    }
}

// Save player registry
bool player_registry_save(const PlayerRegistry* registry, const char* session_id) {
    if (!registry || !session_id) {
        return false;
    }

    char path[256];
    snprintf(path, sizeof(path), "/tmp/adventure-players/%s-registry.dat", session_id);

    FILE* fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }

    fwrite(&registry->player_count, sizeof(int), 1, fp);
    fwrite(registry->players, sizeof(Player), registry->player_count, fp);

    fclose(fp);
    return true;
}

// Load player registry
bool player_registry_load(PlayerRegistry* registry, const char* session_id) {
    if (!registry || !session_id) {
        return false;
    }

    char path[256];
    snprintf(path, sizeof(path), "/tmp/adventure-players/%s-registry.dat", session_id);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }

    // Security: Read player_count and validate before using
    int temp_count = 0;
    size_t read_count = fread(&temp_count, sizeof(int), 1, fp);
    if (read_count != 1) {
        fprintf(stderr, "Security: Failed to read player count from registry\n");
        fclose(fp);
        return false;
    }

    // Security: Validate player_count range to prevent integer overflow
    // and buffer overflow when reading player array
    if (temp_count < 0 || temp_count > MAX_PLAYERS) {
        fprintf(stderr, "Security: Invalid player count %d (max: %d)\n",
                temp_count, MAX_PLAYERS);
        fclose(fp);
        return false;
    }

    registry->player_count = temp_count;

    // Security: Check fread return value to prevent memory corruption
    if (registry->player_count > 0) {
        size_t players_read = fread(registry->players, sizeof(Player),
                                    registry->player_count, fp);
        if (players_read != (size_t)registry->player_count) {
            fprintf(stderr, "Security: Failed to read player data (got %zu, expected %d)\n",
                    players_read, registry->player_count);
            registry->player_count = 0;  // Reset to safe state
            fclose(fp);
            return false;
        }
    }

    fclose(fp);

    player_registry_update_states(registry);
    return true;
}

// ============================================================================
// TEAM OPERATIONS
// ============================================================================

// Check if all players are ready
bool team_is_ready(const PlayerRegistry* registry) {
    if (!registry || registry->player_count == 0) {
        return false;
    }

    for (int i = 0; i < registry->player_count; i++) {
        if (!registry->players[i].is_ready) {
            return false;
        }
    }

    return true;
}

// Count players with specific role
int team_count_roles(const PlayerRegistry* registry, PlayerRole role) {
    if (!registry) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < registry->player_count; i++) {
        if (registry->players[i].role == role) {
            count++;
        }
    }

    return count;
}

// Check if team has at least one player with role
bool team_has_role(const PlayerRegistry* registry, PlayerRole role) {
    return team_count_roles(registry, role) > 0;
}

// Calculate total team health
int team_total_health(const PlayerRegistry* registry) {
    if (!registry) {
        return 0;
    }

    int total = 0;
    for (int i = 0; i < registry->player_count; i++) {
        total += registry->players[i].health;
    }

    return total;
}

// Check if team can proceed (all alive, enough energy, etc.)
bool team_can_proceed(const PlayerRegistry* registry) {
    if (!registry || registry->player_count == 0) {
        return false;
    }

    // At least one player must be active
    if (registry->active_count == 0) {
        return false;
    }

    // All active players must have health > 0
    for (int i = 0; i < registry->player_count; i++) {
        if (registry->players[i].state == PLAYER_ACTIVE &&
            registry->players[i].health <= 0) {
            return false;
        }
    }

    return true;
}
