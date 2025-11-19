/*
 * Session Coordinator Daemon
 *
 * Central server for managing multiplayer adventure sessions.
 * Handles player connections, state synchronization, and event distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "session.h"
#include "player.h"
#include "ipc.h"

#define COORDINATOR_SOCKET "/tmp/adventure-engine/coordinator.sock"
#define TICK_INTERVAL_MS 100  // 100ms tick rate

// Global state
static volatile int g_running = 1;
static SessionRegistry* g_session_registry = NULL;

// Signal handler for graceful shutdown
void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("\nShutting down coordinator...\n");
        g_running = 0;
    }
}

// Initialize coordinator
bool coordinator_init(void) {
    printf("Initializing session coordinator...\n");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize IPC
    if (!ipc_init()) {
        fprintf(stderr, "Failed to initialize IPC\n");
        return false;
    }

    // Initialize session registry
    g_session_registry = registry_init();
    if (!g_session_registry) {
        fprintf(stderr, "Failed to initialize session registry\n");
        return false;
    }

    printf("Coordinator initialized successfully\n");
    return true;
}

// Cleanup coordinator
void coordinator_cleanup(void) {
    printf("Cleaning up coordinator...\n");

    // Save session registry
    if (g_session_registry) {
        registry_save(g_session_registry);
        free(g_session_registry);
        g_session_registry = NULL;
    }

    // Cleanup IPC
    ipc_cleanup();

    printf("Coordinator shut down cleanly\n");
}

// Process coordinator tick
void coordinator_tick(void) {
    // Cleanup old sessions periodically
    static time_t last_cleanup = 0;
    time_t now = time(NULL);

    if (now - last_cleanup > 300) {  // Every 5 minutes
        registry_cleanup_old_sessions(g_session_registry, 24);  // 24 hour cutoff
        last_cleanup = now;
    }

    // TODO: Process pending messages
    // TODO: Update session states
    // TODO: Broadcast state changes
}

// Handle create session command
bool handle_create_session(const char* campaign, const char* gm,
                          int max_players, int min_players) {
    Session* session = session_create(campaign, gm, max_players, min_players);
    if (!session) {
        fprintf(stderr, "Failed to create session\n");
        return false;
    }

    if (!registry_add_session(g_session_registry, session)) {
        fprintf(stderr, "Failed to add session to registry\n");
        session_destroy(session);
        return false;
    }

    printf("Created session: %s\n", session->id);
    printf("  Campaign: %s\n", campaign);
    printf("  GM: %s\n", gm);
    printf("  Players: %d-%d\n", min_players, max_players);

    // Don't free session - it's in the registry now
    return true;
}

// Handle list sessions command
void handle_list_sessions(void) {
    if (!g_session_registry || g_session_registry->session_count == 0) {
        printf("No active sessions\n");
        return;
    }

    printf("\n=== ACTIVE SESSIONS ===\n");
    for (int i = 0; i < g_session_registry->session_count; i++) {
        Session* s = &g_session_registry->sessions[i];
        printf("\n[%d] %s\n", i + 1, s->id);
        printf("    Campaign: %s\n", s->campaign_name);
        printf("    GM: %s\n", s->gm_name);
        printf("    State: %s\n", session_state_to_string(s->state));
        printf("    Players: %d/%d\n", s->current_players, s->max_players);
        printf("    Created: %s", ctime(&s->created_at));
    }
    printf("\n");
}

// Handle join session command
bool handle_join_session(const char* session_id, const char* username,
                        const char* role_str) {
    Session* session = registry_find_session(g_session_registry, session_id);
    if (!session) {
        fprintf(stderr, "Session not found: %s\n", session_id);
        return false;
    }

    if (session->current_players >= session->max_players) {
        fprintf(stderr, "Session is full\n");
        return false;
    }

    PlayerRole role = role_from_string(role_str);
    if (role == ROLE_NONE) {
        fprintf(stderr, "Invalid role: %s\n", role_str);
        return false;
    }

    printf("Player '%s' joining session %s as %s\n",
           username, session_id, role_to_string(role));

    // Add player to session
    if (!session_add_player(session)) {
        fprintf(stderr, "Failed to add player to session\n");
        return false;
    }

    // TODO: Create player in session's player registry
    // TODO: Send welcome message to player

    printf("Player joined successfully\n");
    return true;
}

// Handle start session command
bool handle_start_session(const char* session_id) {
    Session* session = registry_find_session(g_session_registry, session_id);
    if (!session) {
        fprintf(stderr, "Session not found: %s\n", session_id);
        return false;
    }

    if (!session_start(session)) {
        fprintf(stderr, "Failed to start session\n");
        return false;
    }

    printf("Session %s started\n", session_id);
    return true;
}

// Interactive command loop (for testing)
void coordinator_interactive(void) {
    char cmd[256];
    char arg1[128], arg2[128], arg3[128], arg4[128];

    printf("\nCoordinator Interactive Mode\n");
    printf("Commands: create, list, join, start, quit\n\n");

    while (g_running) {
        printf("coordinator> ");
        fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin)) {
            break;
        }

        // Remove newline
        cmd[strcspn(cmd, "\n")] = 0;

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "list") == 0) {
            handle_list_sessions();
        } else if (sscanf(cmd, "create %127s %127s %127s %127s",
                         arg1, arg2, arg3, arg4) == 4) {
            // create <campaign> <gm> <max_players> <min_players>
            handle_create_session(arg1, arg2, atoi(arg3), atoi(arg4));
        } else if (sscanf(cmd, "join %127s %127s %127s", arg1, arg2, arg3) == 3) {
            // join <session_id> <username> <role>
            handle_join_session(arg1, arg2, arg3);
        } else if (sscanf(cmd, "start %127s", arg1) == 1) {
            // start <session_id>
            handle_start_session(arg1);
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
            printf("Commands: create <campaign> <gm> <max> <min>\n");
            printf("          list\n");
            printf("          join <session_id> <user> <role>\n");
            printf("          start <session_id>\n");
            printf("          quit\n");
        }
    }
}

// Daemon mode (background service)
void coordinator_daemon_mode(void) {
    printf("Running in daemon mode (pid: %d)\n", getpid());
    printf("Tick interval: %dms\n", TICK_INTERVAL_MS);

    while (g_running) {
        coordinator_tick();
        usleep(TICK_INTERVAL_MS * 1000);  // Convert ms to microseconds
    }
}

// Print usage
void print_usage(const char* prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\nOptions:\n");
    printf("  -d, --daemon     Run as background daemon\n");
    printf("  -i, --interactive Run in interactive mode (default)\n");
    printf("  -h, --help       Show this help\n");
    printf("\nInteractive Commands:\n");
    printf("  create <campaign> <gm> <max_players> <min_players>\n");
    printf("  list\n");
    printf("  join <session_id> <username> <role>\n");
    printf("  start <session_id>\n");
    printf("  quit\n");
}

// Main
int main(int argc, char* argv[]) {
    bool daemon_mode = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            daemon_mode = false;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Initialize
    if (!coordinator_init()) {
        fprintf(stderr, "Failed to initialize coordinator\n");
        return 1;
    }

    // Run
    if (daemon_mode) {
        coordinator_daemon_mode();
    } else {
        coordinator_interactive();
    }

    // Cleanup
    coordinator_cleanup();

    return 0;
}
