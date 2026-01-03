// Security: Feature test macros must come BEFORE any system includes
// Required for flock() file locking on POSIX systems
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef SESSION_H
#define SESSION_H

#include <time.h>
#include <stdbool.h>

#define MAX_SESSION_ID 64
#define MAX_SESSION_NAME 128
#define MAX_SESSIONS 16
#define MAX_PLAYERS_PER_SESSION 8
#define MAX_GM_NAME 64
#define MAX_REALM_NAME 64

// Session states
typedef enum {
    SESSION_LOBBY,      // Waiting for players to join
    SESSION_ACTIVE,     // Game in progress
    SESSION_PAUSED,     // Temporarily paused
    SESSION_COMPLETED,  // Finished successfully
    SESSION_ABORTED     // Ended prematurely
} SessionState;

// Session structure
typedef struct {
    char id[MAX_SESSION_ID];                // Unique session ID (generated)
    char campaign_name[MAX_SESSION_NAME];   // Campaign being played
    char gm_name[MAX_GM_NAME];              // Game Master name

    time_t created_at;
    time_t updated_at;
    time_t started_at;
    time_t completed_at;

    int max_players;
    int current_players;
    int min_players;        // Minimum to start

    SessionState state;

    char current_realm[MAX_REALM_NAME];     // Current realm in campaign
    int realm_index;                        // Index in campaign sequence

    // Statistics
    int commands_processed;
    int events_triggered;
    int puzzles_solved;

    // File paths
    char save_path[256];    // Where session state is saved
    char log_path[256];     // Session log file

} Session;

// Session registry for coordinator
typedef struct {
    Session sessions[MAX_SESSIONS];
    int session_count;
    time_t last_cleanup;
} SessionRegistry;

// Function declarations
Session* session_create(const char* campaign_name, const char* gm_name,
                       int max_players, int min_players);
bool session_destroy(Session* session);
bool session_add_player(Session* session);
bool session_remove_player(Session* session);
bool session_start(Session* session);
bool session_pause(Session* session);
bool session_resume(Session* session);
bool session_complete(Session* session);
bool session_save(const Session* session);
bool session_load(Session* session, const char* session_id);

// Session registry operations
SessionRegistry* registry_init(void);
bool registry_add_session(SessionRegistry* registry, Session* session);
bool registry_remove_session(SessionRegistry* registry, const char* session_id);
Session* registry_find_session(SessionRegistry* registry, const char* session_id);
int registry_list_sessions(SessionRegistry* registry, Session** out_sessions, int max);
void registry_cleanup_old_sessions(SessionRegistry* registry, int max_age_hours);
bool registry_save(const SessionRegistry* registry);
bool registry_load(SessionRegistry* registry);

// Utility functions
void session_generate_id(char* out_id, int max_len);
const char* session_state_to_string(SessionState state);
bool session_validate(const Session* session);

#endif // SESSION_H
