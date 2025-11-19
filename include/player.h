#ifndef PLAYER_H
#define PLAYER_H

#include <time.h>
#include <stdbool.h>

#define MAX_USERNAME 64
#define MAX_PLAYERS 8

// Player roles for team building
typedef enum {
    ROLE_NONE,
    ROLE_LEADER,    // Can inspire team, see objectives, make decisions
    ROLE_SCOUT,     // Can reveal map, detect hidden items, move faster
    ROLE_ENGINEER,  // Can fix/build items, unlock mechanisms, solve technical puzzles
    ROLE_MEDIC,     // Can heal team, remove debuffs, provide support
    ROLE_DIPLOMAT,  // Can talk to NPCs, negotiate, gather information
    ROLE_SPECIALIST // Wildcard role with unique campaign-specific abilities
} PlayerRole;

// Player connection state
typedef enum {
    PLAYER_DISCONNECTED,
    PLAYER_CONNECTED,
    PLAYER_ACTIVE,      // Currently playing
    PLAYER_IDLE,        // Connected but inactive
    PLAYER_SPECTATING   // Watching but not playing
} PlayerState;

// Player structure
typedef struct {
    char username[MAX_USERNAME];
    char session_id[64];
    int player_number;      // 0-based index in session

    PlayerRole role;
    PlayerState state;

    // Connection info
    char socket_path[256];  // IPC socket for this player
    time_t connected_at;
    time_t last_activity;
    time_t last_heartbeat;

    // Game stats
    int commands_issued;
    int items_found;
    int puzzles_contributed;
    int team_actions;       // Actions that helped team

    // Abilities (role-based)
    bool can_inspect;       // See hidden details
    bool can_unlock;        // Open locked things
    bool can_heal;          // Heal team members
    bool can_negotiate;     // Talk to NPCs
    bool can_lead;          // Make team decisions

    // Status
    int health;             // 0-100
    int energy;             // 0-100 (consumed by special abilities)
    bool is_ready;          // Ready for next challenge

} Player;

// Player registry for session
typedef struct {
    Player players[MAX_PLAYERS];
    int player_count;
    int connected_count;
    int active_count;
} PlayerRegistry;

// Function declarations
Player* player_create(const char* username, const char* session_id,
                     PlayerRole role);
bool player_destroy(Player* player);
bool player_connect(Player* player);
bool player_disconnect(Player* player);
bool player_activate(Player* player);
bool player_heartbeat(Player* player);
bool player_save(const Player* player);
bool player_load(Player* player, const char* session_id, const char* username);

// Role management
const char* role_to_string(PlayerRole role);
PlayerRole role_from_string(const char* role_str);
void role_assign_abilities(Player* player);
bool role_can_perform_action(const Player* player, const char* action);

// Player registry operations
PlayerRegistry* player_registry_init(void);
bool player_registry_add(PlayerRegistry* registry, Player* player);
bool player_registry_remove(PlayerRegistry* registry, const char* username);
Player* player_registry_find(PlayerRegistry* registry, const char* username);
int player_registry_get_by_role(PlayerRegistry* registry, PlayerRole role,
                                Player** out_players, int max);
int player_registry_get_active(PlayerRegistry* registry, Player** out_players, int max);
void player_registry_update_states(PlayerRegistry* registry);
bool player_registry_save(const PlayerRegistry* registry, const char* session_id);
bool player_registry_load(PlayerRegistry* registry, const char* session_id);

// Team operations
bool team_is_ready(const PlayerRegistry* registry);
int team_count_roles(const PlayerRegistry* registry, PlayerRole role);
bool team_has_role(const PlayerRegistry* registry, PlayerRole role);
int team_total_health(const PlayerRegistry* registry);
bool team_can_proceed(const PlayerRegistry* registry);

// Utility
bool player_validate(const Player* player);
void player_reset_stats(Player* player);

#endif // PLAYER_H
