// Note: _POSIX_C_SOURCE is defined in session.h for portability
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>  // Security: Added for flock() file locking

#define SESSION_DIR "/tmp/adventure-sessions"
#define REGISTRY_FILE "/tmp/adventure-sessions/registry.dat"

// Helper to ensure session directory exists
static bool ensure_session_dir(void) {
    struct stat st = {0};
    if (stat(SESSION_DIR, &st) == -1) {
        if (mkdir(SESSION_DIR, 0700) != 0) {
            perror("Failed to create session directory");
            return false;
        }
    }
    return true;
}

// Generate unique session ID
void session_generate_id(char* out_id, int max_len) {
    // Format: SESS-YYYYMMDD-HHMMSS-PID
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    snprintf(out_id, max_len, "SESS-%04d%02d%02d-%02d%02d%02d-%d",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             getpid());
}

// Convert session state to string
const char* session_state_to_string(SessionState state) {
    switch (state) {
        case SESSION_LOBBY:     return "LOBBY";
        case SESSION_ACTIVE:    return "ACTIVE";
        case SESSION_PAUSED:    return "PAUSED";
        case SESSION_COMPLETED: return "COMPLETED";
        case SESSION_ABORTED:   return "ABORTED";
        default:                return "UNKNOWN";
    }
}

// Create a new session
Session* session_create(const char* campaign_name, const char* gm_name,
                       int max_players, int min_players) {
    if (!ensure_session_dir()) {
        return NULL;
    }

    Session* session = (Session*)calloc(1, sizeof(Session));
    if (!session) {
        perror("Failed to allocate session");
        return NULL;
    }

    // Generate unique ID
    session_generate_id(session->id, MAX_SESSION_ID);

    // Set basic info
    // Security: Ensure null termination after all strncpy calls
    strncpy(session->campaign_name, campaign_name, MAX_SESSION_NAME - 1);
    session->campaign_name[MAX_SESSION_NAME - 1] = '\0';
    strncpy(session->gm_name, gm_name, MAX_GM_NAME - 1);
    session->gm_name[MAX_GM_NAME - 1] = '\0';

    // Initialize timestamps
    session->created_at = time(NULL);
    session->updated_at = session->created_at;
    session->started_at = 0;
    session->completed_at = 0;

    // Player limits
    session->max_players = (max_players > 0 && max_players <= MAX_PLAYERS_PER_SESSION) ?
                          max_players : 4;
    session->min_players = (min_players > 0) ? min_players : 2;
    session->current_players = 0;

    // State
    session->state = SESSION_LOBBY;

    // Realm tracking
    session->current_realm[0] = '\0';
    session->realm_index = 0;

    // Statistics
    session->commands_processed = 0;
    session->events_triggered = 0;
    session->puzzles_solved = 0;

    // File paths
    snprintf(session->save_path, sizeof(session->save_path),
             "%s/%s.session", SESSION_DIR, session->id);
    snprintf(session->log_path, sizeof(session->log_path),
             "%s/%s.log", SESSION_DIR, session->id);

    // Save initial state
    if (!session_save(session)) {
        fprintf(stderr, "Warning: Failed to save initial session state\n");
    }

    return session;
}

// Destroy session
bool session_destroy(Session* session) {
    if (!session) {
        return false;
    }

    // Remove session files
    if (session->save_path[0]) {
        unlink(session->save_path);
    }
    // Keep log file for archival

    free(session);
    return true;
}

// Add player to session
bool session_add_player(Session* session) {
    if (!session) {
        return false;
    }

    if (session->current_players >= session->max_players) {
        fprintf(stderr, "Session is full\n");
        return false;
    }

    session->current_players++;
    session->updated_at = time(NULL);
    return session_save(session);
}

// Remove player from session
bool session_remove_player(Session* session) {
    if (!session || session->current_players <= 0) {
        return false;
    }

    session->current_players--;
    session->updated_at = time(NULL);
    return session_save(session);
}

// Start session (transition from LOBBY to ACTIVE)
bool session_start(Session* session) {
    if (!session) {
        return false;
    }

    if (session->state != SESSION_LOBBY) {
        fprintf(stderr, "Session cannot be started from state %s\n",
                session_state_to_string(session->state));
        return false;
    }

    if (session->current_players < session->min_players) {
        fprintf(stderr, "Not enough players to start (need %d, have %d)\n",
                session->min_players, session->current_players);
        return false;
    }

    session->state = SESSION_ACTIVE;
    session->started_at = time(NULL);
    session->updated_at = session->started_at;

    return session_save(session);
}

// Pause session
bool session_pause(Session* session) {
    if (!session || session->state != SESSION_ACTIVE) {
        return false;
    }

    session->state = SESSION_PAUSED;
    session->updated_at = time(NULL);
    return session_save(session);
}

// Resume session
bool session_resume(Session* session) {
    if (!session || session->state != SESSION_PAUSED) {
        return false;
    }

    session->state = SESSION_ACTIVE;
    session->updated_at = time(NULL);
    return session_save(session);
}

// Complete session
bool session_complete(Session* session) {
    if (!session) {
        return false;
    }

    session->state = SESSION_COMPLETED;
    session->completed_at = time(NULL);
    session->updated_at = session->completed_at;
    return session_save(session);
}

// Save session to file
bool session_save(const Session* session) {
    if (!session) {
        return false;
    }

    FILE* fp = fopen(session->save_path, "w");
    if (!fp) {
        perror("Failed to open session file for writing");
        return false;
    }

    fprintf(fp, "[SESSION]\n");
    fprintf(fp, "id: %s\n", session->id);
    fprintf(fp, "campaign: %s\n", session->campaign_name);
    fprintf(fp, "gm: %s\n", session->gm_name);
    fprintf(fp, "created: %ld\n", session->created_at);
    fprintf(fp, "updated: %ld\n", session->updated_at);
    fprintf(fp, "started: %ld\n", session->started_at);
    fprintf(fp, "completed: %ld\n", session->completed_at);
    fprintf(fp, "max_players: %d\n", session->max_players);
    fprintf(fp, "min_players: %d\n", session->min_players);
    fprintf(fp, "current_players: %d\n", session->current_players);
    fprintf(fp, "state: %s\n", session_state_to_string(session->state));
    fprintf(fp, "current_realm: %s\n", session->current_realm);
    fprintf(fp, "realm_index: %d\n", session->realm_index);
    fprintf(fp, "commands_processed: %d\n", session->commands_processed);
    fprintf(fp, "events_triggered: %d\n", session->events_triggered);
    fprintf(fp, "puzzles_solved: %d\n", session->puzzles_solved);

    fclose(fp);
    return true;
}

// Load session from file
bool session_load(Session* session, const char* session_id) {
    if (!session || !session_id) {
        return false;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s.session", SESSION_DIR, session_id);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        perror("Failed to open session file for reading");
        return false;
    }

    char line[512];
    // Security: Initialize buffers to prevent use of uninitialized data
    char key[128] = {0};
    char value[384] = {0};

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '[' || line[0] == '\n') {
            continue;
        }

        // Security: Reset buffers before each sscanf to prevent stale data
        key[0] = '\0';
        value[0] = '\0';

        if (sscanf(line, "%127[^:]: %383[^\n]", key, value) == 2) {
            // Security: Ensure null termination after all strncpy calls
            if (strcmp(key, "id") == 0) {
                strncpy(session->id, value, MAX_SESSION_ID - 1);
                session->id[MAX_SESSION_ID - 1] = '\0';
            } else if (strcmp(key, "campaign") == 0) {
                strncpy(session->campaign_name, value, MAX_SESSION_NAME - 1);
                session->campaign_name[MAX_SESSION_NAME - 1] = '\0';
            } else if (strcmp(key, "gm") == 0) {
                strncpy(session->gm_name, value, MAX_GM_NAME - 1);
                session->gm_name[MAX_GM_NAME - 1] = '\0';
            } else if (strcmp(key, "created") == 0) {
                session->created_at = atol(value);
            } else if (strcmp(key, "updated") == 0) {
                session->updated_at = atol(value);
            } else if (strcmp(key, "started") == 0) {
                session->started_at = atol(value);
            } else if (strcmp(key, "completed") == 0) {
                session->completed_at = atol(value);
            } else if (strcmp(key, "max_players") == 0) {
                session->max_players = atoi(value);
            } else if (strcmp(key, "min_players") == 0) {
                session->min_players = atoi(value);
            } else if (strcmp(key, "current_players") == 0) {
                session->current_players = atoi(value);
            } else if (strcmp(key, "current_realm") == 0) {
                strncpy(session->current_realm, value, MAX_REALM_NAME - 1);
                session->current_realm[MAX_REALM_NAME - 1] = '\0';
            } else if (strcmp(key, "realm_index") == 0) {
                session->realm_index = atoi(value);
            } else if (strcmp(key, "commands_processed") == 0) {
                session->commands_processed = atoi(value);
            } else if (strcmp(key, "events_triggered") == 0) {
                session->events_triggered = atoi(value);
            } else if (strcmp(key, "puzzles_solved") == 0) {
                session->puzzles_solved = atoi(value);
            }
            // state parsing omitted for brevity - add if needed
        }
    }

    fclose(fp);

    // Reconstruct file paths
    snprintf(session->save_path, sizeof(session->save_path),
             "%s/%s.session", SESSION_DIR, session->id);
    snprintf(session->log_path, sizeof(session->log_path),
             "%s/%s.log", SESSION_DIR, session->id);

    return true;
}

// Validate session
bool session_validate(const Session* session) {
    if (!session) {
        return false;
    }

    if (session->id[0] == '\0') {
        return false;
    }

    if (session->max_players <= 0 || session->max_players > MAX_PLAYERS_PER_SESSION) {
        return false;
    }

    if (session->current_players < 0 || session->current_players > session->max_players) {
        return false;
    }

    return true;
}

// ============================================================================
// SESSION REGISTRY
// ============================================================================

// Initialize session registry
SessionRegistry* registry_init(void) {
    SessionRegistry* registry = (SessionRegistry*)calloc(1, sizeof(SessionRegistry));
    if (!registry) {
        perror("Failed to allocate session registry");
        return NULL;
    }

    registry->session_count = 0;
    registry->last_cleanup = time(NULL);

    // Try to load existing registry
    if (!registry_load(registry)) {
        fprintf(stderr, "Starting with empty registry\n");
    }

    return registry;
}

// Add session to registry
bool registry_add_session(SessionRegistry* registry, Session* session) {
    if (!registry || !session) {
        return false;
    }

    if (registry->session_count >= MAX_SESSIONS) {
        fprintf(stderr, "Registry is full\n");
        return false;
    }

    // Copy session data
    memcpy(&registry->sessions[registry->session_count], session, sizeof(Session));
    registry->session_count++;

    return registry_save(registry);
}

// Remove session from registry
bool registry_remove_session(SessionRegistry* registry, const char* session_id) {
    if (!registry || !session_id) {
        return false;
    }

    for (int i = 0; i < registry->session_count; i++) {
        if (strcmp(registry->sessions[i].id, session_id) == 0) {
            // Shift remaining sessions
            for (int j = i; j < registry->session_count - 1; j++) {
                memcpy(&registry->sessions[j], &registry->sessions[j + 1], sizeof(Session));
            }
            registry->session_count--;
            return registry_save(registry);
        }
    }

    return false;
}

// Find session in registry
Session* registry_find_session(SessionRegistry* registry, const char* session_id) {
    if (!registry || !session_id) {
        return NULL;
    }

    for (int i = 0; i < registry->session_count; i++) {
        if (strcmp(registry->sessions[i].id, session_id) == 0) {
            return &registry->sessions[i];
        }
    }

    return NULL;
}

// List all sessions
int registry_list_sessions(SessionRegistry* registry, Session** out_sessions, int max) {
    if (!registry || !out_sessions) {
        return 0;
    }

    int count = (registry->session_count < max) ? registry->session_count : max;
    for (int i = 0; i < count; i++) {
        out_sessions[i] = &registry->sessions[i];
    }

    return count;
}

// Clean up old sessions
void registry_cleanup_old_sessions(SessionRegistry* registry, int max_age_hours) {
    if (!registry) {
        return;
    }

    time_t now = time(NULL);
    time_t cutoff = now - (max_age_hours * 3600);

    for (int i = registry->session_count - 1; i >= 0; i--) {
        Session* session = &registry->sessions[i];

        // Remove completed or aborted sessions older than cutoff
        if ((session->state == SESSION_COMPLETED || session->state == SESSION_ABORTED) &&
            session->updated_at < cutoff) {
            registry_remove_session(registry, session->id);
        }
    }

    registry->last_cleanup = now;
}

// Save registry to file
bool registry_save(const SessionRegistry* registry) {
    if (!registry) {
        return false;
    }

    if (!ensure_session_dir()) {
        return false;
    }

    FILE* fp = fopen(REGISTRY_FILE, "wb");
    if (!fp) {
        perror("Failed to save registry");
        return false;
    }

    // Security: Acquire exclusive lock to prevent race conditions
    // during multiplayer registry access
    int fd = fileno(fp);
    if (flock(fd, LOCK_EX) != 0) {
        perror("Security: Failed to acquire registry lock");
        fclose(fp);
        return false;
    }

    size_t written = 0;
    written += fwrite(&registry->session_count, sizeof(int), 1, fp);
    written += fwrite(&registry->last_cleanup, sizeof(time_t), 1, fp);
    written += fwrite(registry->sessions, sizeof(Session), registry->session_count, fp);

    if (written != (size_t)(2 + registry->session_count)) {
        fprintf(stderr, "Failed to write complete registry data\n");
        flock(fd, LOCK_UN);  // Release lock
        fclose(fp);
        return false;
    }

    // Security: Ensure data is flushed before releasing lock
    fflush(fp);
    flock(fd, LOCK_UN);  // Release lock
    fclose(fp);
    return true;
}

// Load registry from file
bool registry_load(SessionRegistry* registry) {
    if (!registry) {
        return false;
    }

    FILE* fp = fopen(REGISTRY_FILE, "rb");
    if (!fp) {
        return false;  // No registry file exists yet
    }

    // Security: Acquire shared lock to prevent reading during writes
    int fd = fileno(fp);
    if (flock(fd, LOCK_SH) != 0) {
        perror("Security: Failed to acquire registry read lock");
        fclose(fp);
        return false;
    }

    // Security: Read session_count into temporary variable and validate
    int temp_count = 0;
    size_t read = fread(&temp_count, sizeof(int), 1, fp);
    if (read != 1) {
        fprintf(stderr, "Security: Failed to read session count from registry\n");
        flock(fd, LOCK_UN);
        fclose(fp);
        return false;
    }

    // Security: Validate session count range to prevent buffer overflow
    if (temp_count < 0 || temp_count > MAX_SESSIONS) {
        fprintf(stderr, "Security: Invalid session count in registry: %d (max: %d)\n",
                temp_count, MAX_SESSIONS);
        flock(fd, LOCK_UN);
        fclose(fp);
        return false;
    }

    registry->session_count = temp_count;

    // Read last_cleanup timestamp
    if (fread(&registry->last_cleanup, sizeof(time_t), 1, fp) != 1) {
        fprintf(stderr, "Security: Failed to read last_cleanup from registry\n");
        registry->session_count = 0;  // Reset to safe state
        flock(fd, LOCK_UN);
        fclose(fp);
        return false;
    }

    // Security: Read sessions with validated count
    if (registry->session_count > 0) {
        size_t sessions_read = fread(registry->sessions, sizeof(Session),
                                      registry->session_count, fp);
        if (sessions_read != (size_t)registry->session_count) {
            fprintf(stderr, "Security: Failed to read session data (got %zu, expected %d)\n",
                    sessions_read, registry->session_count);
            registry->session_count = 0;  // Reset to safe state
            flock(fd, LOCK_UN);
            fclose(fp);
            return false;
        }
    }

    flock(fd, LOCK_UN);  // Release lock
    fclose(fp);
    return true;
}
