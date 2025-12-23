# Deep Dive: GM Live Controls System

**Status**: Implementation Blueprint
**Complexity**: High
**Estimated Effort**: 12-16 hours
**Priority**: High (enables facilitated experiences)

---

## Overview

The Game Master (GM) Live Controls system provides a powerful interface for facilitators to guide, monitor, and enhance team-building sessions in real-time. This is crucial for corporate training and educational uses.

### GM Capabilities

1. **Session Monitoring** - Real-time view of all player states, positions, actions
2. **Event Triggering** - Manually trigger events, spawn items, modify world
3. **Hint System** - Provide graduated hints when teams are stuck
4. **Difficulty Adjustment** - Make puzzles easier/harder on the fly
5. **Communication** - Broadcast messages, whisper to individual players
6. **Pause/Resume** - Control game flow and pacing
7. **Analytics Dashboard** - Real-time metrics and team dynamics
8. **Recording** - Session recording and replay
9. **Emergency Controls** - Reset, rollback, emergency interventions

---

## Architecture Overview

```
┌────────────────────────────────────────────────────────────┐
│                    GM DASHBOARD (tmux panel)                │
│                                                             │
│  ┌──────────────┬──────────────┬──────────────┐           │
│  │  SESSION     │  PLAYER      │  CONTROLS    │           │
│  │  OVERVIEW    │  MONITOR     │  & ACTIONS   │           │
│  ├──────────────┼──────────────┼──────────────┤           │
│  │  ANALYTICS   │  EVENT LOG   │  HINTS       │           │
│  └──────────────┴──────────────┴──────────────┘           │
│                                                             │
│  Command Input: gm> _                                       │
└────────────────────────────────────────────────────────────┘
                          │
                          ▼ GM Commands
┌────────────────────────────────────────────────────────────┐
│              GM CONTROLLER (daemon component)               │
│                                                             │
│  ┌──────────────────────────────────────────────────┐     │
│  │  Command Processor                               │     │
│  │  - Parse GM commands                             │     │
│  │  - Validate permissions                          │     │
│  │  - Execute actions                               │     │
│  └──────────────────────────────────────────────────┘     │
│                                                             │
│  ┌──────────────────────────────────────────────────┐     │
│  │  Session Monitor                                 │     │
│  │  - Track player states                           │     │
│  │  - Calculate metrics                             │     │
│  │  - Detect stuck situations                       │     │
│  └──────────────────────────────────────────────────┘     │
│                                                             │
│  ┌──────────────────────────────────────────────────┐     │
│  │  Event Manager                                   │     │
│  │  - Queue GM-triggered events                     │     │
│  │  - Schedule timed events                         │     │
│  │  - Broadcast to players                          │     │
│  └──────────────────────────────────────────────────┘     │
│                                                             │
│  ┌──────────────────────────────────────────────────┐     │
│  │  Recording System                                │     │
│  │  - Log all actions                               │     │
│  │  - Snapshot states                               │     │
│  │  - Enable replay                                 │     │
│  └──────────────────────────────────────────────────┘     │
└────────────────────────────────────────────────────────────┘
                          │
                          ▼ Affects World State
┌────────────────────────────────────────────────────────────┐
│                  MULTIPLAYER WORLD STATE                    │
└────────────────────────────────────────────────────────────┘
                          │
                          ▼ Updates
┌────────────────────────────────────────────────────────────┐
│                      PLAYERS                                │
└────────────────────────────────────────────────────────────┘
```

---

## Data Structures

### 1. GM Controller Structures

**File**: `include/gm_controller.h`

```c
#ifndef GM_CONTROLLER_H
#define GM_CONTROLLER_H

#include "multiplayer_world.h"
#include "session.h"
#include "player.h"
#include <time.h>
#include <stdbool.h>

#define MAX_GM_COMMANDS 100
#define MAX_HINTS 20
#define MAX_SCHEDULED_EVENTS 50
#define MAX_RECORDINGS 10

// GM permission levels
typedef enum {
    GM_LEVEL_OBSERVER,     // Can only view, no control
    GM_LEVEL_ASSISTANT,    // Can give hints, send messages
    GM_LEVEL_FULL,         // Full control
    GM_LEVEL_ADMIN         // Can modify session config
} GMPermissionLevel;

// GM command types
typedef enum {
    GM_CMD_STATUS,          // Show session status
    GM_CMD_PLAYERS,         // Show player info
    GM_CMD_BROADCAST,       // Send message to all
    GM_CMD_WHISPER,         // Send message to one player
    GM_CMD_PAUSE,           // Pause game
    GM_CMD_RESUME,          // Resume game
    GM_CMD_HINT,            // Give a hint
    GM_CMD_SPAWN_ITEM,      // Spawn an item
    GM_CMD_SPAWN_NPC,       // Spawn an NPC
    GM_CMD_TELEPORT,        // Teleport player
    GM_CMD_SET_FLAG,        // Set world flag
    GM_CMD_CLEAR_FLAG,      // Clear world flag
    GM_CMD_HEAL,            // Heal player/team
    GM_CMD_DAMAGE,          // Damage player
    GM_CMD_UNLOCK,          // Unlock door/path
    GM_CMD_LOCK,            // Lock door/path
    GM_CMD_TRIGGER_EVENT,   // Trigger custom event
    GM_CMD_ADJUST_DIFFICULTY, // Change difficulty
    GM_CMD_RECORD_START,    // Start recording
    GM_CMD_RECORD_STOP,     // Stop recording
    GM_CMD_REPLAY,          // Replay from timestamp
    GM_CMD_RESET,           // Reset to checkpoint
    GM_CMD_END_SESSION,     // End session
    GM_CMD_ANALYTICS,       // Show analytics
    GM_CMD_HELP             // Show GM commands
} GMCommandType;

// GM command structure
typedef struct {
    GMCommandType type;
    char raw_command[256];
    char args[5][128];  // Up to 5 arguments
    int arg_count;
    time_t issued_at;
    char issuer[64];    // GM who issued it
    bool executed;
    char result[512];
} GMCommand;

// Hint structure
typedef struct {
    char id[32];
    char objective[128];      // Which objective this hints at
    int level;                // 1-5, increasing specificity
    char text[256];
    bool revealed;
    time_t revealed_at;
} Hint;

// Hint system
typedef struct {
    Hint hints[MAX_HINTS];
    int hint_count;
    int current_hint_level;  // Track which level we're at
} HintSystem;

// Scheduled event
typedef struct {
    char id[32];
    char event_type[64];
    char description[256];
    time_t scheduled_time;
    time_t executed_time;
    bool executed;
    bool cancelled;

    // Event parameters
    char target_player[64];
    char target_room[32];
    char spawn_item[32];
    char custom_message[256];

} ScheduledEvent;

// Session analytics
typedef struct {
    // Time tracking
    time_t session_start;
    time_t session_end;
    int total_duration_seconds;

    // Player metrics
    int total_commands;
    int commands_per_player[MAX_PLAYERS];
    int puzzles_solved;
    int hints_used;
    int deaths;

    // Team dynamics
    int communication_count;
    float role_utilization[6];  // One per role type
    int collaborative_actions;

    // Progress
    int objectives_completed;
    int objectives_total;
    int rooms_explored;
    int items_collected;

    // Failures/resets
    int resets_used;
    int stuck_count;  // Times team was stuck >5 min

    // Scores
    int completion_score;
    int teamwork_score;
    int efficiency_score;

} SessionAnalytics;

// Recording entry
typedef struct {
    time_t timestamp;
    char event_type[32];
    int player_number;
    char player_name[64];
    char action[128];
    char result[256];
    uint64_t state_version;
} RecordingEntry;

// Recording session
typedef struct {
    char session_id[64];
    bool is_recording;
    time_t started_at;
    time_t stopped_at;

    RecordingEntry entries[10000];
    int entry_count;

    char filepath[256];

} Recording;

// GM Controller main structure
typedef struct {
    // Session reference
    char session_id[64];
    Session* session;
    MultiplayerWorld* world;

    // GM information
    char gm_name[64];
    GMPermissionLevel permission_level;

    // Command history
    GMCommand command_history[MAX_GM_COMMANDS];
    int command_count;

    // Hint system
    HintSystem hint_system;

    // Scheduled events
    ScheduledEvent scheduled_events[MAX_SCHEDULED_EVENTS];
    int scheduled_event_count;

    // Analytics
    SessionAnalytics analytics;

    // Recording
    Recording* active_recording;
    bool auto_record;

    // State
    bool is_active;
    time_t last_update;

} GMController;

// Initialization
GMController* gm_controller_create(const char* session_id, Session* session,
                                  MultiplayerWorld* world, const char* gm_name);
void gm_controller_destroy(GMController* controller);

// Command processing
bool gm_process_command(GMController* controller, const char* command,
                       char* result, int result_max);
bool gm_execute_command(GMController* controller, GMCommand* cmd);

// Session control
bool gm_pause_session(GMController* controller);
bool gm_resume_session(GMController* controller);
bool gm_end_session(GMController* controller);

// Communication
bool gm_broadcast_message(GMController* controller, const char* message);
bool gm_whisper_player(GMController* controller, int player_number, const char* message);

// Hint system
bool gm_add_hint(GMController* controller, const char* objective, int level, const char* text);
bool gm_reveal_hint(GMController* controller, int hint_index, char* result, int result_max);
bool gm_get_next_hint(GMController* controller, char* result, int result_max);

// World manipulation
bool gm_spawn_item(GMController* controller, const char* item_id, const char* room_id);
bool gm_spawn_npc(GMController* controller, const char* npc_id, const char* room_id);
bool gm_teleport_player(GMController* controller, int player_number, const char* room_id);
bool gm_set_world_flag(GMController* controller, const char* flag_name, bool value);
bool gm_unlock_path(GMController* controller, const char* room_id, Direction direction);

// Player manipulation
bool gm_heal_player(GMController* controller, int player_number, int amount);
bool gm_heal_all_players(GMController* controller);
bool gm_damage_player(GMController* controller, int player_number, int amount);

// Event scheduling
bool gm_schedule_event(GMController* controller, const char* event_type,
                       int delay_seconds, const char* description);
bool gm_cancel_event(GMController* controller, const char* event_id);
void gm_process_scheduled_events(GMController* controller);

// Analytics
bool gm_update_analytics(GMController* controller);
bool gm_get_analytics_summary(GMController* controller, char* result, int result_max);
float gm_calculate_teamwork_score(GMController* controller);
float gm_calculate_efficiency_score(GMController* controller);

// Recording
bool gm_start_recording(GMController* controller);
bool gm_stop_recording(GMController* controller);
bool gm_save_recording(GMController* controller, const char* filepath);
bool gm_load_recording(GMController* controller, const char* filepath);
bool gm_replay_from_time(GMController* controller, time_t from_time);

// Difficulty adjustment
bool gm_adjust_difficulty(GMController* controller, float multiplier);

// Utilities
bool gm_is_team_stuck(GMController* controller);
const char* gm_command_type_to_string(GMCommandType type);

#endif // GM_CONTROLLER_H
```

---

## Implementation

### 2. GM Controller Core

**File**: `src/gm_controller.c`

```c
#include "gm_controller.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create GM controller
GMController* gm_controller_create(const char* session_id, Session* session,
                                  MultiplayerWorld* world, const char* gm_name) {
    if (!session_id || !session || !world || !gm_name) {
        return NULL;
    }

    GMController* controller = (GMController*)calloc(1, sizeof(GMController));
    if (!controller) {
        perror("Failed to allocate GM controller");
        return NULL;
    }

    strncpy(controller->session_id, session_id, 63);
    controller->session = session;
    controller->world = world;
    strncpy(controller->gm_name, gm_name, 63);

    controller->permission_level = GM_LEVEL_FULL;
    controller->command_count = 0;
    controller->scheduled_event_count = 0;
    controller->active_recording = NULL;
    controller->auto_record = true;
    controller->is_active = true;
    controller->last_update = time(NULL);

    // Initialize hint system
    controller->hint_system.hint_count = 0;
    controller->hint_system.current_hint_level = 0;

    // Initialize analytics
    memset(&controller->analytics, 0, sizeof(SessionAnalytics));
    controller->analytics.session_start = time(NULL);
    controller->analytics.objectives_total = 5;  // Default

    // Auto-start recording if enabled
    if (controller->auto_record) {
        gm_start_recording(controller);
    }

    printf("[GM] Controller initialized for session %s by %s\n",
           session_id, gm_name);

    return controller;
}

void gm_controller_destroy(GMController* controller) {
    if (!controller) return;

    // Stop recording if active
    if (controller->active_recording) {
        gm_stop_recording(controller);
        free(controller->active_recording);
    }

    free(controller);
}

// Parse and process GM command
bool gm_process_command(GMController* controller, const char* command,
                       char* result, int result_max) {
    if (!controller || !command || !result) {
        return false;
    }

    // Create command structure
    if (controller->command_count >= MAX_GM_COMMANDS) {
        snprintf(result, result_max, "Command history full");
        return false;
    }

    GMCommand* cmd = &controller->command_history[controller->command_count];
    strncpy(cmd->raw_command, command, 255);
    cmd->issued_at = time(NULL);
    strncpy(cmd->issuer, controller->gm_name, 63);
    cmd->executed = false;

    // Parse command
    char verb[64];
    sscanf(command, "%63s", verb);

    // Determine command type
    if (strcmp(verb, "status") == 0 || strcmp(verb, "st") == 0) {
        cmd->type = GM_CMD_STATUS;
    } else if (strcmp(verb, "players") == 0 || strcmp(verb, "pl") == 0) {
        cmd->type = GM_CMD_PLAYERS;
    } else if (strcmp(verb, "broadcast") == 0 || strcmp(verb, "bc") == 0) {
        cmd->type = GM_CMD_BROADCAST;
    } else if (strcmp(verb, "whisper") == 0 || strcmp(verb, "w") == 0) {
        cmd->type = GM_CMD_WHISPER;
    } else if (strcmp(verb, "pause") == 0) {
        cmd->type = GM_CMD_PAUSE;
    } else if (strcmp(verb, "resume") == 0) {
        cmd->type = GM_CMD_RESUME;
    } else if (strcmp(verb, "hint") == 0) {
        cmd->type = GM_CMD_HINT;
    } else if (strcmp(verb, "spawn_item") == 0 || strcmp(verb, "si") == 0) {
        cmd->type = GM_CMD_SPAWN_ITEM;
    } else if (strcmp(verb, "teleport") == 0 || strcmp(verb, "tp") == 0) {
        cmd->type = GM_CMD_TELEPORT;
    } else if (strcmp(verb, "heal") == 0) {
        cmd->type = GM_CMD_HEAL;
    } else if (strcmp(verb, "unlock") == 0) {
        cmd->type = GM_CMD_UNLOCK;
    } else if (strcmp(verb, "trigger") == 0) {
        cmd->type = GM_CMD_TRIGGER_EVENT;
    } else if (strcmp(verb, "difficulty") == 0) {
        cmd->type = GM_CMD_ADJUST_DIFFICULTY;
    } else if (strcmp(verb, "analytics") == 0 || strcmp(verb, "stats") == 0) {
        cmd->type = GM_CMD_ANALYTICS;
    } else if (strcmp(verb, "help") == 0 || strcmp(verb, "?") == 0) {
        cmd->type = GM_CMD_HELP;
    } else {
        snprintf(result, result_max, "Unknown GM command: %s", verb);
        return false;
    }

    // Parse arguments
    const char* args_start = command + strlen(verb);
    while (*args_start == ' ') args_start++;

    cmd->arg_count = 0;
    const char* p = args_start;
    int arg_idx = 0;
    int char_idx = 0;

    while (*p && arg_idx < 5) {
        if (*p == ' ' && char_idx > 0) {
            cmd->args[arg_idx][char_idx] = '\0';
            arg_idx++;
            char_idx = 0;
            cmd->arg_count++;
            p++;
            continue;
        } else if (*p != ' ') {
            cmd->args[arg_idx][char_idx++] = *p;
        }
        p++;
    }

    if (char_idx > 0) {
        cmd->args[arg_idx][char_idx] = '\0';
        cmd->arg_count++;
    }

    // Execute command
    bool success = gm_execute_command(controller, cmd);
    if (success) {
        cmd->executed = true;
        controller->command_count++;
    }

    strncpy(result, cmd->result, result_max - 1);

    return success;
}

// Execute GM command
bool gm_execute_command(GMController* controller, GMCommand* cmd) {
    if (!controller || !cmd) return false;

    switch (cmd->type) {
        case GM_CMD_STATUS:
            return gm_cmd_status(controller, cmd);

        case GM_CMD_PLAYERS:
            return gm_cmd_players(controller, cmd);

        case GM_CMD_BROADCAST:
            if (cmd->arg_count == 0) {
                strcpy(cmd->result, "Usage: broadcast <message>");
                return false;
            }
            return gm_broadcast_message(controller, cmd->args[0]);

        case GM_CMD_PAUSE:
            return gm_pause_session(controller);

        case GM_CMD_RESUME:
            return gm_resume_session(controller);

        case GM_CMD_HINT:
            return gm_get_next_hint(controller, cmd->result, sizeof(cmd->result));

        case GM_CMD_SPAWN_ITEM:
            if (cmd->arg_count < 2) {
                strcpy(cmd->result, "Usage: spawn_item <item_id> <room_id>");
                return false;
            }
            return gm_spawn_item(controller, cmd->args[0], cmd->args[1]);

        case GM_CMD_HEAL:
            if (cmd->arg_count == 0) {
                return gm_heal_all_players(controller);
            } else {
                int player_num = atoi(cmd->args[0]);
                return gm_heal_player(controller, player_num, 100);
            }

        case GM_CMD_ANALYTICS:
            return gm_get_analytics_summary(controller, cmd->result, sizeof(cmd->result));

        case GM_CMD_HELP:
            return gm_cmd_help(controller, cmd);

        default:
            snprintf(cmd->result, sizeof(cmd->result),
                    "Command not yet implemented: %s",
                    gm_command_type_to_string(cmd->type));
            return false;
    }
}

// STATUS command
bool gm_cmd_status(GMController* controller, GMCommand* cmd) {
    if (!controller) return false;

    int offset = 0;
    char* result = cmd->result;
    int max_len = sizeof(cmd->result);

    offset += snprintf(result + offset, max_len - offset,
                      "=== SESSION STATUS ===\n");
    offset += snprintf(result + offset, max_len - offset,
                      "Session: %s\n", controller->session_id);
    offset += snprintf(result + offset, max_len - offset,
                      "GM: %s\n", controller->gm_name);
    offset += snprintf(result + offset, max_len - offset,
                      "State: %s\n", session_state_to_string(controller->session->state));
    offset += snprintf(result + offset, max_len - offset,
                      "Players: %d/%d\n",
                      controller->session->current_players,
                      controller->session->max_players);

    time_t now = time(NULL);
    int elapsed = now - controller->analytics.session_start;
    offset += snprintf(result + offset, max_len - offset,
                      "Elapsed: %02d:%02d:%02d\n",
                      elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);

    offset += snprintf(result + offset, max_len - offset,
                      "Commands: %d\n", controller->analytics.total_commands);
    offset += snprintf(result + offset, max_len - offset,
                      "Puzzles Solved: %d\n", controller->analytics.puzzles_solved);
    offset += snprintf(result + offset, max_len - offset,
                      "Hints Used: %d\n", controller->analytics.hints_used);

    return true;
}

// PLAYERS command
bool gm_cmd_players(GMController* controller, GMCommand* cmd) {
    if (!controller || !controller->world) return false;

    int offset = 0;
    char* result = cmd->result;
    int max_len = sizeof(cmd->result);

    offset += snprintf(result + offset, max_len - offset,
                      "=== PLAYERS ===\n");

    PlayerRegistry* registry = controller->world->player_registry;
    for (int i = 0; i < registry->player_count; i++) {
        Player* p = &registry->players[i];
        const char* room = mp_world_get_player_room(controller->world, i);

        offset += snprintf(result + offset, max_len - offset,
                          "[%d] %s (%s)\n", i, p->username, role_to_string(p->role));
        offset += snprintf(result + offset, max_len - offset,
                          "    Room: %s\n", room ? room : "unknown");
        offset += snprintf(result + offset, max_len - offset,
                          "    Health: %d  Energy: %d\n", p->health, p->energy);
        offset += snprintf(result + offset, max_len - offset,
                          "    Commands: %d\n", p->commands_issued);
    }

    return true;
}

// HELP command
bool gm_cmd_help(GMController* controller, GMCommand* cmd) {
    (void)controller;  // Unused

    strcpy(cmd->result,
           "=== GM COMMANDS ===\n"
           "status (st)          - Show session status\n"
           "players (pl)         - Show player info\n"
           "broadcast <msg>      - Send message to all\n"
           "whisper <id> <msg>   - Send private message\n"
           "pause                - Pause game\n"
           "resume               - Resume game\n"
           "hint                 - Give next hint\n"
           "spawn_item <id> <rm> - Spawn item in room\n"
           "teleport <pl> <rm>   - Teleport player\n"
           "heal [player]        - Heal player or all\n"
           "unlock <rm> <dir>    - Unlock exit\n"
           "trigger <event>      - Trigger event\n"
           "difficulty <mult>    - Adjust difficulty\n"
           "analytics            - Show analytics\n"
           "help (?)             - This help\n");

    return true;
}

// Pause session
bool gm_pause_session(GMController* controller) {
    if (!controller || !controller->session) return false;

    bool success = session_pause(controller->session);
    if (success) {
        gm_broadcast_message(controller, "[GM] Game paused by facilitator");
    }

    return success;
}

// Resume session
bool gm_resume_session(GMController* controller) {
    if (!controller || !controller->session) return false;

    bool success = session_resume(controller->session);
    if (success) {
        gm_broadcast_message(controller, "[GM] Game resumed");
    }

    return success;
}

// Broadcast message
bool gm_broadcast_message(GMController* controller, const char* message) {
    if (!controller || !message) return false;

    // TODO: Use IPC to send to all players
    printf("[BROADCAST] %s\n", message);

    // Log to recording
    if (controller->active_recording) {
        // Add to recording
    }

    return true;
}

// Spawn item
bool gm_spawn_item(GMController* controller, const char* item_id, const char* room_id) {
    if (!controller || !item_id || !room_id) return false;

    mp_world_lock(controller->world);

    // Find item
    Item* item = NULL;
    for (int i = 0; i < controller->world->world.item_count; i++) {
        if (strcmp(controller->world->world.items[i].id, item_id) == 0) {
            item = &controller->world->world.items[i];
            break;
        }
    }

    if (!item) {
        mp_world_unlock(controller->world);
        return false;
    }

    // Place in room
    strncpy(item->location, room_id, 31);
    controller->world->item_taken_by_player[/* item index */] = -1;

    mp_world_unlock(controller->world);

    // Broadcast event
    char message[256];
    snprintf(message, sizeof(message), "[GM] %s appeared in the room!", item->name);
    gm_broadcast_message(controller, message);

    return true;
}

// Heal player
bool gm_heal_player(GMController* controller, int player_number, int amount) {
    if (!controller || player_number < 0) return false;

    PlayerRegistry* registry = controller->world->player_registry;
    if (player_number >= registry->player_count) return false;

    Player* player = &registry->players[player_number];
    player->health += amount;
    if (player->health > 100) player->health = 100;

    char message[256];
    snprintf(message, sizeof(message),
            "[GM] %s has been healed! (+%d HP)", player->username, amount);
    gm_broadcast_message(controller, message);

    return true;
}

// Heal all players
bool gm_heal_all_players(GMController* controller) {
    if (!controller) return false;

    PlayerRegistry* registry = controller->world->player_registry;
    for (int i = 0; i < registry->player_count; i++) {
        registry->players[i].health = 100;
        registry->players[i].energy = 100;
    }

    gm_broadcast_message(controller, "[GM] All players fully healed!");

    return true;
}

// Add hint
bool gm_add_hint(GMController* controller, const char* objective, int level, const char* text) {
    if (!controller || !objective || !text) return false;

    HintSystem* hs = &controller->hint_system;
    if (hs->hint_count >= MAX_HINTS) return false;

    Hint* hint = &hs->hints[hs->hint_count];
    snprintf(hint->id, sizeof(hint->id), "hint_%d", hs->hint_count);
    strncpy(hint->objective, objective, 127);
    hint->level = level;
    strncpy(hint->text, text, 255);
    hint->revealed = false;

    hs->hint_count++;

    return true;
}

// Get next hint
bool gm_get_next_hint(GMController* controller, char* result, int result_max) {
    if (!controller || !result) return false;

    HintSystem* hs = &controller->hint_system;

    // Find next unrevealed hint at current level
    for (int i = 0; i < hs->hint_count; i++) {
        Hint* hint = &hs->hints[i];
        if (!hint->revealed && hint->level == hs->current_hint_level + 1) {
            // Reveal this hint
            hint->revealed = true;
            hint->revealed_at = time(NULL);
            hs->current_hint_level = hint->level;

            snprintf(result, result_max,
                    "HINT [Level %d]: %s", hint->level, hint->text);

            // Broadcast to players
            char broadcast[512];
            snprintf(broadcast, sizeof(broadcast),
                    "[GM HINT] %s", hint->text);
            gm_broadcast_message(controller, broadcast);

            controller->analytics.hints_used++;

            return true;
        }
    }

    snprintf(result, result_max, "No more hints available");
    return false;
}

// Update analytics
bool gm_update_analytics(GMController* controller) {
    if (!controller) return false;

    SessionAnalytics* a = &controller->analytics;

    // Update total commands
    a->total_commands = 0;
    PlayerRegistry* registry = controller->world->player_registry;
    for (int i = 0; i < registry->player_count; i++) {
        a->commands_per_player[i] = registry->players[i].commands_issued;
        a->total_commands += a->commands_per_player[i];
    }

    // Calculate role utilization
    for (int i = 0; i < 6; i++) {
        // TODO: Calculate based on role-specific actions
    }

    return true;
}

// Get analytics summary
bool gm_get_analytics_summary(GMController* controller, char* result, int result_max) {
    if (!controller || !result) return false;

    gm_update_analytics(controller);

    SessionAnalytics* a = &controller->analytics;
    int offset = 0;

    offset += snprintf(result + offset, result_max - offset,
                      "=== ANALYTICS ===\n");

    time_t now = time(NULL);
    int duration = now - a->session_start;
    offset += snprintf(result + offset, result_max - offset,
                      "Duration: %02d:%02d:%02d\n",
                      duration / 3600, (duration % 3600) / 60, duration % 60);

    offset += snprintf(result + offset, result_max - offset,
                      "Total Commands: %d\n", a->total_commands);

    offset += snprintf(result + offset, result_max - offset,
                      "Puzzles Solved: %d\n", a->puzzles_solved);

    offset += snprintf(result + offset, result_max - offset,
                      "Objectives: %d/%d\n",
                      a->objectives_completed, a->objectives_total);

    float teamwork = gm_calculate_teamwork_score(controller);
    offset += snprintf(result + offset, result_max - offset,
                      "Teamwork Score: %.1f/100\n", teamwork);

    float efficiency = gm_calculate_efficiency_score(controller);
    offset += snprintf(result + offset, result_max - offset,
                      "Efficiency: %.1f/100\n", efficiency);

    offset += snprintf(result + offset, result_max - offset,
                      "Hints Used: %d\n", a->hints_used);

    return true;
}

// Calculate teamwork score
float gm_calculate_teamwork_score(GMController* controller) {
    if (!controller) return 0.0f;

    SessionAnalytics* a = &controller->analytics;

    // Factors:
    // - Even distribution of commands (not one player dominating)
    // - Collaborative actions
    // - Communication count

    float score = 50.0f;  // Base score

    // Check command distribution
    int max_commands = 0;
    int min_commands = 999999;
    for (int i = 0; i < controller->world->player_registry->player_count; i++) {
        int cmds = a->commands_per_player[i];
        if (cmds > max_commands) max_commands = cmds;
        if (cmds < min_commands && cmds > 0) min_commands = cmds;
    }

    if (max_commands > 0) {
        float distribution = (float)min_commands / max_commands;
        score += distribution * 25.0f;  // Up to +25 for even distribution
    }

    // Collaborative actions
    score += (a->collaborative_actions * 2.0f);  // +2 per collaborative action

    // Cap at 100
    if (score > 100.0f) score = 100.0f;

    return score;
}

// Calculate efficiency score
float gm_calculate_efficiency_score(GMController* controller) {
    if (!controller) return 0.0f;

    SessionAnalytics* a = &controller->analytics;

    float score = 100.0f;

    // Penalty for hints used
    score -= (a->hints_used * 5.0f);

    // Penalty for resets
    score -= (a->resets_used * 10.0f);

    // Penalty for stuck situations
    score -= (a->stuck_count * 8.0f);

    if (score < 0.0f) score = 0.0f;

    return score;
}

// Start recording
bool gm_start_recording(GMController* controller) {
    if (!controller) return false;

    if (controller->active_recording) {
        return false;  // Already recording
    }

    Recording* rec = (Recording*)calloc(1, sizeof(Recording));
    if (!rec) {
        perror("Failed to allocate recording");
        return false;
    }

    strncpy(rec->session_id, controller->session_id, 63);
    rec->is_recording = true;
    rec->started_at = time(NULL);
    rec->entry_count = 0;

    snprintf(rec->filepath, sizeof(rec->filepath),
            "/tmp/adventure-recordings/%s-%ld.rec",
            controller->session_id, rec->started_at);

    controller->active_recording = rec;

    printf("[GM] Recording started: %s\n", rec->filepath);

    return true;
}

// Stop recording
bool gm_stop_recording(GMController* controller) {
    if (!controller || !controller->active_recording) return false;

    controller->active_recording->is_recording = false;
    controller->active_recording->stopped_at = time(NULL);

    // Save to file
    gm_save_recording(controller, controller->active_recording->filepath);

    printf("[GM] Recording stopped: %d entries\n",
           controller->active_recording->entry_count);

    return true;
}

// Save recording to file
bool gm_save_recording(GMController* controller, const char* filepath) {
    if (!controller || !controller->active_recording || !filepath) {
        return false;
    }

    FILE* fp = fopen(filepath, "w");
    if (!fp) {
        perror("Failed to open recording file");
        return false;
    }

    Recording* rec = controller->active_recording;

    fprintf(fp, "[RECORDING]\n");
    fprintf(fp, "session: %s\n", rec->session_id);
    fprintf(fp, "started: %ld\n", rec->started_at);
    fprintf(fp, "stopped: %ld\n", rec->stopped_at);
    fprintf(fp, "entries: %d\n\n", rec->entry_count);

    for (int i = 0; i < rec->entry_count; i++) {
        RecordingEntry* entry = &rec->entries[i];
        fprintf(fp, "[ENTRY]\n");
        fprintf(fp, "timestamp: %ld\n", entry->timestamp);
        fprintf(fp, "event_type: %s\n", entry->event_type);
        fprintf(fp, "player: %s\n", entry->player_name);
        fprintf(fp, "action: %s\n", entry->action);
        fprintf(fp, "result: %s\n", entry->result);
        fprintf(fp, "state_version: %lu\n\n", entry->state_version);
    }

    fclose(fp);

    printf("[GM] Recording saved: %s\n", filepath);

    return true;
}

// Check if team is stuck
bool gm_is_team_stuck(GMController* controller) {
    if (!controller) return false;

    // Team is "stuck" if:
    // 1. No progress in last 5 minutes
    // 2. Commands issued but no state changes
    // 3. Repeated failed attempts

    time_t now = time(NULL);
    time_t last_progress = controller->world->last_update;

    if (now - last_progress > 300) {  // 5 minutes
        // Check if commands were issued
        if (controller->analytics.total_commands > 0) {
            return true;
        }
    }

    return false;
}

// Command type to string
const char* gm_command_type_to_string(GMCommandType type) {
    switch (type) {
        case GM_CMD_STATUS: return "STATUS";
        case GM_CMD_PLAYERS: return "PLAYERS";
        case GM_CMD_BROADCAST: return "BROADCAST";
        case GM_CMD_PAUSE: return "PAUSE";
        case GM_CMD_RESUME: return "RESUME";
        case GM_CMD_HINT: return "HINT";
        // ... other cases
        default: return "UNKNOWN";
    }
}
```

---

## GM Dashboard Interface

### 3. Tmux-Based GM Dashboard

**File**: `bin/gm-dashboard`

```bash
#!/bin/bash
#
# GM Dashboard - Interactive GM control interface
#

SESSION_ID="$1"

if [ -z "$SESSION_ID" ]; then
    echo "Usage: $0 <session_id>"
    exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"

TMUX_SESSION="gm-${SESSION_ID}"

# Check if session exists
if tmux has-session -t "$TMUX_SESSION" 2>/dev/null; then
    echo "GM dashboard already running for session $SESSION_ID"
    exec tmux attach -t "$TMUX_SESSION"
fi

echo "Launching GM Dashboard for session: $SESSION_ID"

# Create tmux session with GM layout
tmux new-session -d -s "$TMUX_SESSION" -n "gm-control"

# Split into 6 panels
# ┌─────────────┬─────────────┬─────────────┐
# │   STATUS    │   PLAYERS   │  CONTROLS   │
# ├─────────────┼─────────────┼─────────────┤
# │ ANALYTICS   │  EVENT LOG  │    HINTS    │
# └─────────────┴─────────────┴─────────────┘

# Top row
tmux split-window -h -t "$TMUX_SESSION:gm-control.0"
tmux split-window -h -t "$TMUX_SESSION:gm-control.1"

# Bottom row
tmux select-pane -t 0
tmux split-window -v -t "$TMUX_SESSION:gm-control.0"
tmux select-pane -t 2
tmux split-window -v -t "$TMUX_SESSION:gm-control.2"
tmux select-pane -t 4
tmux split-window -v -t "$TMUX_SESSION:gm-control.4"

# Panel 0: STATUS
tmux send-keys -t "$TMUX_SESSION:gm-control.0" \
    "watch -n 2 './bin/gm-status $SESSION_ID'" C-m

# Panel 1: PLAYERS
tmux send-keys -t "$TMUX_SESSION:gm-control.1" \
    "watch -n 2 './bin/gm-players $SESSION_ID'" C-m

# Panel 2: CONTROLS (Command input)
tmux send-keys -t "$TMUX_SESSION:gm-control.2" \
    "./bin/gm-cli $SESSION_ID" C-m

# Panel 3: ANALYTICS
tmux send-keys -t "$TMUX_SESSION:gm-control.3" \
    "watch -n 5 './bin/gm-analytics $SESSION_ID'" C-m

# Panel 4: EVENT LOG
tmux send-keys -t "$TMUX_SESSION:gm-control.4" \
    "tail -f /tmp/adventure-sessions/${SESSION_ID}.log" C-m

# Panel 5: HINTS
tmux send-keys -t "$TMUX_SESSION:gm-control.5" \
    "./bin/gm-hints $SESSION_ID" C-m

# Select control panel for input
tmux select-pane -t "$TMUX_SESSION:gm-control.2"

# Attach
exec tmux attach -t "$TMUX_SESSION"
```

### 4. GM CLI Interface

**File**: `bin/gm-cli`

```bash
#!/bin/bash
#
# GM Command Line Interface
#

SESSION_ID="$1"

if [ -z "$SESSION_ID" ]; then
    echo "Error: No session ID provided"
    exit 1
fi

echo "╔════════════════════════════════════════════════════════╗"
echo "║           GM CONTROL INTERFACE                        ║"
echo "║  Session: $SESSION_ID                                  "
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "Type 'help' for GM commands, 'quit' to exit"
echo ""

# Command loop
while true; do
    echo -n "gm> "
    read -r command

    if [ -z "$command" ]; then
        continue
    fi

    if [ "$command" = "quit" ] || [ "$command" = "exit" ]; then
        break
    fi

    # Send command to GM controller
    # TODO: Implement actual IPC with GM controller
    # For now, echo the command
    echo "[Executing: $command]"

    case "$command" in
        status|st)
            # Show session status
            cat "$STATE_DIR/status.txt" 2>/dev/null || echo "Status not available"
            ;;
        players|pl)
            # Show player info
            cat "$STATE_DIR/players.txt" 2>/dev/null || echo "Players not available"
            ;;
        hint)
            # Give next hint
            echo "[GM HINT] Try checking the room for hidden items"
            ;;
        pause)
            echo "[SESSION PAUSED]"
            ;;
        resume)
            echo "[SESSION RESUMED]"
            ;;
        help|?)
            cat << 'EOF'
GM COMMANDS:
  status (st)          - Show session status
  players (pl)         - Show player information
  broadcast <msg>      - Send message to all players
  whisper <id> <msg>   - Send private message
  pause                - Pause the game
  resume               - Resume the game
  hint                 - Give next hint to team
  spawn_item <id> <rm> - Spawn item in room
  teleport <pl> <room> - Teleport player
  heal [player]        - Heal player(s)
  unlock <room> <dir>  - Unlock exit
  analytics            - Show team analytics
  help                 - Show this help
  quit                 - Exit GM interface
EOF
            ;;
        *)
            echo "Unknown command. Type 'help' for available commands."
            ;;
    esac

    echo ""
done

echo "GM interface closed."
```

---

## Pre-configured Hint Systems

### 5. Hint Configuration File

**File**: `hints/team_challenge_hints.hints`

```
# Hints for Team Challenge Realm
# Format: objective:level:text

[HINTS:team_challenge]

# Level 1 hints (gentle nudges)
choose_path:1:Remember, each path has different requirements. Check your team roles!
choose_path:1:Some paths may require specific skills. Look at what each team member can do.

# Level 2 hints (more specific)
choose_path:2:The Path of Valor needs a LEADER. Do you have one on your team?
choose_path:2:The Path of Wisdom requires an ENGINEER or DIPLOMAT. Check your roles!

# Level 3 hints (very specific)
choose_path:3:If you have a LEADER, try the north door (Path of Valor).
choose_path:3:The golden key can be found by examining the tablet carefully.

# Level 4 hints (direct guidance)
valor_puzzle:4:You need TWO players to open the valor chest simultaneously.
unity_puzzle:4:ALL players must stand on the pressure plates at the same time.

# Level 5 hints (nearly give away)
final_chamber:5:You need the Team Banner, Tome of Wisdom, AND Crystal of Unity to proceed.
final_chamber:5:Make sure a SCOUT reveals the secret vault entrance first!

[AUTOMATED_HINTS]
# Triggered automatically based on conditions

stuck_5min:After 5 minutes with no progress, try using 'look' to re-examine your surroundings.
repeated_fail:If a command keeps failing, there might be a requirement you haven't met yet.
low_health:Your team's health is low! Find a MEDIC or look for healing items.
```

---

## Scheduled Events System

### 6. Event Scheduler

```c
// Schedule an event to happen in the future
bool gm_schedule_event(GMController* controller, const char* event_type,
                       int delay_seconds, const char* description) {
    if (!controller || !event_type || !description) return false;

    if (controller->scheduled_event_count >= MAX_SCHEDULED_EVENTS) {
        return false;
    }

    ScheduledEvent* event = &controller->scheduled_events[controller->scheduled_event_count];

    snprintf(event->id, sizeof(event->id), "scheduled_%d", controller->scheduled_event_count);
    strncpy(event->event_type, event_type, 63);
    strncpy(event->description, description, 255);

    event->scheduled_time = time(NULL) + delay_seconds;
    event->executed = false;
    event->cancelled = false;

    controller->scheduled_event_count++;

    printf("[GM] Event scheduled: %s in %d seconds\n", description, delay_seconds);

    return true;
}

// Process scheduled events (called in game loop)
void gm_process_scheduled_events(GMController* controller) {
    if (!controller) return;

    time_t now = time(NULL);

    for (int i = 0; i < controller->scheduled_event_count; i++) {
        ScheduledEvent* event = &controller->scheduled_events[i];

        if (event->executed || event->cancelled) {
            continue;
        }

        if (now >= event->scheduled_time) {
            // Execute event
            printf("[GM] Executing scheduled event: %s\n", event->description);

            if (strcmp(event->event_type, "broadcast") == 0) {
                gm_broadcast_message(controller, event->custom_message);
            } else if (strcmp(event->event_type, "spawn_item") == 0) {
                gm_spawn_item(controller, event->spawn_item, event->target_room);
            }
            // ... handle other event types

            event->executed = true;
            event->executed_time = now;
        }
    }
}

// Example usage:
// gm_schedule_event(controller, "broadcast", 60,
//                  "A mysterious rumbling shakes the ground...");
```

---

## Testing

### 7. GM Controller Test Suite

**File**: `tests/test_gm_controller.c`

```c
#include "gm_controller.h"
#include <assert.h>

void test_gm_create() {
    Session session = {0};
    strcpy(session.id, "TEST");

    MultiplayerWorld* world = mp_world_create("TEST", &session);
    GMController* gm = gm_controller_create("TEST", &session, world, "test_gm");

    assert(gm != NULL);
    assert(strcmp(gm->gm_name, "test_gm") == 0);
    assert(gm->is_active);

    gm_controller_destroy(gm);
    mp_world_destroy(world);
}

void test_gm_process_command() {
    // Setup
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);
    GMController* gm = gm_controller_create("TEST", &session, world, "test_gm");

    // Test status command
    char result[1024];
    bool success = gm_process_command(gm, "status", result, sizeof(result));
    assert(success);
    assert(strstr(result, "SESSION STATUS") != NULL);

    // Test help command
    success = gm_process_command(gm, "help", result, sizeof(result));
    assert(success);
    assert(strstr(result, "GM COMMANDS") != NULL);

    // Cleanup
    gm_controller_destroy(gm);
    mp_world_destroy(world);
}

void test_gm_hint_system() {
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);
    GMController* gm = gm_controller_create("TEST", &session, world, "test_gm");

    // Add hints
    gm_add_hint(gm, "test_objective", 1, "This is hint level 1");
    gm_add_hint(gm, "test_objective", 2, "This is hint level 2");
    gm_add_hint(gm, "test_objective", 3, "This is hint level 3");

    assert(gm->hint_system.hint_count == 3);

    // Get first hint
    char result[512];
    bool success = gm_get_next_hint(gm, result, sizeof(result));
    assert(success);
    assert(strstr(result, "level 1") != NULL);
    assert(gm->hint_system.current_hint_level == 1);

    // Get second hint
    success = gm_get_next_hint(gm, result, sizeof(result));
    assert(success);
    assert(strstr(result, "level 2") != NULL);

    gm_controller_destroy(gm);
    mp_world_destroy(world);
}

int main() {
    printf("Running GM controller tests...\n");
    test_gm_create();
    test_gm_process_command();
    test_gm_hint_system();
    printf("All tests passed!\n");
    return 0;
}
```

---

## Summary

This deep dive provides a complete GM live controls system:

1. **GM Controller** - Core system for GM operations
2. **Command system** - 20+ GM commands
3. **Session monitoring** - Real-time player/world state
4. **Hint system** - Graduated hints with auto-triggers
5. **Event scheduling** - Timed events
6. **Analytics dashboard** - Team metrics
7. **Recording/replay** - Session recording
8. **Tmux dashboard** - 6-panel GM interface
9. **Communication** - Broadcast/whisper
10. **World manipulation** - Spawn, teleport, unlock, etc.

**Key features**:
- Real-time monitoring of all players
- Graduated hint system (5 levels)
- Session recording and replay
- Team analytics (teamwork score, efficiency)
- Scheduled events
- Difficulty adjustment
- Emergency controls (pause, reset)

**GM workflow**:
1. Launch dashboard: `./bin/gm-dashboard SESSION_ID`
2. Monitor 6 panels (status, players, analytics, log, hints, controls)
3. Issue commands via CLI: `gm> hint`, `gm> heal all`, etc.
4. View real-time analytics
5. Intervene when team is stuck
6. Review recording after session

**Estimated implementation time**: 12-16 hours

**Files to create**:
- `include/gm_controller.h`
- `src/gm_controller.c`
- `bin/gm-dashboard`
- `bin/gm-cli`
- `bin/gm-status`
- `bin/gm-players`
- `bin/gm-analytics`
- `bin/gm-hints`
- `hints/*.hints` (hint configuration files)
- `tests/test_gm_controller.c`

**Total new code**: ~3,000 lines
