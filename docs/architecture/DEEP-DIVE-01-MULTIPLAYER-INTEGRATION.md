# Deep Dive: Connecting Game Engine to Multiplayer Backend

**Status**: Implementation Blueprint
**Complexity**: High
**Estimated Effort**: 12-16 hours
**Priority**: Critical (foundation for all other features)

---

## Overview

Currently, the single-player game engine (`src/main.c`) and multiplayer coordinator (`src/session_coordinator.c`) exist as separate systems. This document provides an exhaustive plan to integrate them into a cohesive multiplayer experience.

---

## Architecture Analysis

### Current State

```
SINGLE-PLAYER ENGINE           MULTIPLAYER COORDINATOR
┌──────────────────┐          ┌──────────────────────┐
│ main.c           │          │ session_coordinator.c│
│ - Game loop      │          │ - Session management │
│ - Command parse  │          │ - Player registry    │
│ - World state    │          │ - IPC handling       │
│ - Local display  │          │                      │
└──────────────────┘          └──────────────────────┘
       ↓                              ↓
   Terminal                    Session metadata
   (ncurses)                   (files/registry)

NO CONNECTION BETWEEN THEM
```

### Target State

```
MULTIPLAYER GAME ARCHITECTURE
┌─────────────────────────────────────────────────────┐
│         SESSION COORDINATOR (daemon)                │
│  ┌───────────────────────────────────────────────┐ │
│  │  Session Manager                              │ │
│  │  - Active sessions                            │ │
│  │  - Player registry per session                │ │
│  └───────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────┐ │
│  │  Shared World State (per session)             │ │
│  │  - World struct                               │ │
│  │  - Player positions                           │ │
│  │  - Item states                                │ │
│  │  - Event queue                                │ │
│  └───────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────┐ │
│  │  Game Logic Engine                            │ │
│  │  - Command processing                         │ │
│  │  - State mutations                            │ │
│  │  - Event generation                           │ │
│  └───────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
         ↑                  ↓
      IPC (sockets)    Broadcasts
         ↑                  ↓
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ PLAYER       │  │ PLAYER       │  │ PLAYER       │
│ CLIENT 1     │  │ CLIENT 2     │  │ CLIENT 3     │
│              │  │              │  │              │
│ - Input      │  │ - Input      │  │ - Input      │
│ - Display    │  │ - Display    │  │ - Display    │
│ - Local UI   │  │ - Local UI   │  │ - Local UI   │
└──────────────┘  └──────────────┘  └──────────────┘
```

---

## Phase 1: Shared World State Structure

### 1.1 Define Multiplayer World State

**File**: `include/multiplayer_world.h`

```c
#ifndef MULTIPLAYER_WORLD_H
#define MULTIPLAYER_WORLD_H

#include "world.h"
#include "player.h"
#include "session.h"
#include <pthread.h>

#define MAX_EVENTS_QUEUED 100
#define MAX_RECENT_ACTIONS 50

// Multiplayer-aware world state
typedef struct {
    // Base world state (from world.h)
    World world;

    // Session information
    char session_id[64];
    Session* session;

    // Player tracking
    PlayerRegistry* player_registry;
    char current_room_per_player[MAX_PLAYERS][32];  // Room ID per player

    // Shared state
    bool room_visited_by_anyone[MAX_ROOMS];
    int item_taken_by_player[MAX_ITEMS];  // -1 if not taken, player_number if taken

    // Event queue
    GameEvent event_queue[MAX_EVENTS_QUEUED];
    int event_queue_head;
    int event_queue_tail;
    int event_count;

    // Recent actions (for replay/sync)
    PlayerAction recent_actions[MAX_RECENT_ACTIONS];
    int action_index;

    // Synchronization
    pthread_mutex_t world_mutex;
    uint64_t state_version;  // Incremented on every state change
    time_t last_update;

    // Flags
    bool world_loaded;
    bool game_started;
    bool game_completed;

} MultiplayerWorld;

// Game events
typedef enum {
    EVENT_PLAYER_MOVED,
    EVENT_ITEM_TAKEN,
    EVENT_ITEM_DROPPED,
    EVENT_ITEM_EXAMINED,
    EVENT_ROOM_ENTERED,
    EVENT_PUZZLE_SOLVED,
    EVENT_DOOR_UNLOCKED,
    EVENT_NPC_TALKED,
    EVENT_PLAYER_DIED,
    EVENT_TEAM_OBJECTIVE_COMPLETE,
    EVENT_CUSTOM
} EventType;

typedef struct {
    EventType type;
    int player_number;
    char player_name[64];
    char detail[256];  // e.g., "north", "golden_key", "main_hall"
    time_t timestamp;
    bool broadcasted;  // Has this been sent to all clients?
} GameEvent;

// Player actions (for history/replay)
typedef struct {
    int player_number;
    char command[256];
    char result[512];
    time_t timestamp;
    uint64_t state_version_after;
} PlayerAction;

// Function declarations

// World lifecycle
MultiplayerWorld* mp_world_create(const char* session_id, Session* session);
bool mp_world_load(MultiplayerWorld* mp_world, const char* world_file);
bool mp_world_start(MultiplayerWorld* mp_world);
void mp_world_destroy(MultiplayerWorld* mp_world);

// State access (thread-safe)
bool mp_world_lock(MultiplayerWorld* mp_world);
bool mp_world_unlock(MultiplayerWorld* mp_world);
uint64_t mp_world_get_version(MultiplayerWorld* mp_world);

// Player operations
bool mp_world_add_player(MultiplayerWorld* mp_world, Player* player);
bool mp_world_remove_player(MultiplayerWorld* mp_world, int player_number);
const char* mp_world_get_player_room(MultiplayerWorld* mp_world, int player_number);
bool mp_world_set_player_room(MultiplayerWorld* mp_world, int player_number, const char* room_id);

// Command processing
bool mp_world_process_command(MultiplayerWorld* mp_world, int player_number,
                              const char* command, char* result, int result_max);

// Event management
bool mp_world_add_event(MultiplayerWorld* mp_world, EventType type,
                       int player_number, const char* detail);
int mp_world_get_pending_events(MultiplayerWorld* mp_world, GameEvent* out_events, int max);
bool mp_world_mark_event_broadcasted(MultiplayerWorld* mp_world, int event_index);

// State serialization (for sync)
bool mp_world_serialize_state(MultiplayerWorld* mp_world, char* buffer, int max_len);
bool mp_world_deserialize_state(MultiplayerWorld* mp_world, const char* buffer);

// Persistence
bool mp_world_save_to_file(MultiplayerWorld* mp_world, const char* filepath);
bool mp_world_load_from_file(MultiplayerWorld* mp_world, const char* filepath);

#endif // MULTIPLAYER_WORLD_H
```

### 1.2 Implementation Scaffold

**File**: `src/multiplayer_world.c`

```c
#include "multiplayer_world.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create multiplayer world
MultiplayerWorld* mp_world_create(const char* session_id, Session* session) {
    MultiplayerWorld* mp_world = (MultiplayerWorld*)calloc(1, sizeof(MultiplayerWorld));
    if (!mp_world) {
        perror("Failed to allocate multiplayer world");
        return NULL;
    }

    strncpy(mp_world->session_id, session_id, 63);
    mp_world->session = session;

    // Initialize world state
    mp_world->world.room_count = 0;
    mp_world->world.item_count = 0;

    // Initialize player registry
    mp_world->player_registry = player_registry_init();

    // Initialize event queue
    mp_world->event_queue_head = 0;
    mp_world->event_queue_tail = 0;
    mp_world->event_count = 0;

    // Initialize action history
    mp_world->action_index = 0;

    // Initialize mutex
    pthread_mutex_init(&mp_world->world_mutex, NULL);

    mp_world->state_version = 0;
    mp_world->last_update = time(NULL);

    mp_world->world_loaded = false;
    mp_world->game_started = false;
    mp_world->game_completed = false;

    // Initialize player rooms to starting room (will be set on world load)
    for (int i = 0; i < MAX_PLAYERS; i++) {
        mp_world->current_room_per_player[i][0] = '\0';
    }

    // Initialize item ownership
    for (int i = 0; i < MAX_ITEMS; i++) {
        mp_world->item_taken_by_player[i] = -1;
    }

    return mp_world;
}

// Load world from file
bool mp_world_load(MultiplayerWorld* mp_world, const char* world_file) {
    if (!mp_world || !world_file) return false;

    // Use existing world loader
    if (!load_world(&mp_world->world, world_file)) {
        fprintf(stderr, "Failed to load world file: %s\n", world_file);
        return false;
    }

    // Set all players to starting room
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i < mp_world->player_registry->player_count) {
            strncpy(mp_world->current_room_per_player[i],
                   mp_world->world.start_room, 31);
        }
    }

    mp_world->world_loaded = true;
    mp_world->last_update = time(NULL);
    mp_world->state_version++;

    return true;
}

// Start game (all players ready)
bool mp_world_start(MultiplayerWorld* mp_world) {
    if (!mp_world || !mp_world->world_loaded) return false;

    mp_world_lock(mp_world);

    mp_world->game_started = true;
    mp_world->state_version++;

    // Add game start event
    mp_world_add_event(mp_world, EVENT_CUSTOM, -1, "Game started!");

    mp_world_unlock(mp_world);

    return true;
}

// Destroy multiplayer world
void mp_world_destroy(MultiplayerWorld* mp_world) {
    if (!mp_world) return;

    pthread_mutex_destroy(&mp_world->world_mutex);

    if (mp_world->player_registry) {
        free(mp_world->player_registry);
    }

    free(mp_world);
}

// Thread-safe locking
bool mp_world_lock(MultiplayerWorld* mp_world) {
    if (!mp_world) return false;
    return pthread_mutex_lock(&mp_world->world_mutex) == 0;
}

bool mp_world_unlock(MultiplayerWorld* mp_world) {
    if (!mp_world) return false;
    return pthread_mutex_unlock(&mp_world->world_mutex) == 0;
}

// Get current state version
uint64_t mp_world_get_version(MultiplayerWorld* mp_world) {
    if (!mp_world) return 0;

    mp_world_lock(mp_world);
    uint64_t version = mp_world->state_version;
    mp_world_unlock(mp_world);

    return version;
}

// Add player to world
bool mp_world_add_player(MultiplayerWorld* mp_world, Player* player) {
    if (!mp_world || !player) return false;

    mp_world_lock(mp_world);

    bool success = player_registry_add(mp_world->player_registry, player);

    if (success && mp_world->world_loaded) {
        // Set player to starting room
        strncpy(mp_world->current_room_per_player[player->player_number],
               mp_world->world.start_room, 31);

        // Add event
        char detail[256];
        snprintf(detail, sizeof(detail), "%s joined as %s",
                player->username, role_to_string(player->role));
        mp_world_add_event(mp_world, EVENT_CUSTOM, player->player_number, detail);

        mp_world->state_version++;
    }

    mp_world_unlock(mp_world);

    return success;
}

// Get player's current room
const char* mp_world_get_player_room(MultiplayerWorld* mp_world, int player_number) {
    if (!mp_world || player_number < 0 || player_number >= MAX_PLAYERS) {
        return NULL;
    }

    return mp_world->current_room_per_player[player_number];
}

// Set player's current room
bool mp_world_set_player_room(MultiplayerWorld* mp_world, int player_number,
                              const char* room_id) {
    if (!mp_world || player_number < 0 || player_number >= MAX_PLAYERS || !room_id) {
        return false;
    }

    mp_world_lock(mp_world);

    strncpy(mp_world->current_room_per_player[player_number], room_id, 31);
    mp_world->state_version++;
    mp_world->last_update = time(NULL);

    mp_world_unlock(mp_world);

    return true;
}

// Add event to queue
bool mp_world_add_event(MultiplayerWorld* mp_world, EventType type,
                       int player_number, const char* detail) {
    if (!mp_world) return false;

    // Check if queue is full
    if (mp_world->event_count >= MAX_EVENTS_QUEUED) {
        fprintf(stderr, "Event queue is full\n");
        return false;
    }

    GameEvent* event = &mp_world->event_queue[mp_world->event_queue_tail];
    event->type = type;
    event->player_number = player_number;
    event->timestamp = time(NULL);
    event->broadcasted = false;

    if (player_number >= 0 && player_number < mp_world->player_registry->player_count) {
        strncpy(event->player_name,
               mp_world->player_registry->players[player_number].username, 63);
    } else {
        strcpy(event->player_name, "SYSTEM");
    }

    if (detail) {
        strncpy(event->detail, detail, 255);
    } else {
        event->detail[0] = '\0';
    }

    mp_world->event_queue_tail = (mp_world->event_queue_tail + 1) % MAX_EVENTS_QUEUED;
    mp_world->event_count++;

    return true;
}

// Get pending (non-broadcasted) events
int mp_world_get_pending_events(MultiplayerWorld* mp_world, GameEvent* out_events, int max) {
    if (!mp_world || !out_events) return 0;

    mp_world_lock(mp_world);

    int count = 0;
    int index = mp_world->event_queue_head;

    while (index != mp_world->event_queue_tail && count < max) {
        if (!mp_world->event_queue[index].broadcasted) {
            memcpy(&out_events[count], &mp_world->event_queue[index], sizeof(GameEvent));
            count++;
        }
        index = (index + 1) % MAX_EVENTS_QUEUED;
    }

    mp_world_unlock(mp_world);

    return count;
}

// Process player command (CRITICAL - integrates game logic)
bool mp_world_process_command(MultiplayerWorld* mp_world, int player_number,
                              const char* command, char* result, int result_max) {
    if (!mp_world || !command || !result) return false;

    if (!mp_world->game_started) {
        snprintf(result, result_max, "Game has not started yet.");
        return false;
    }

    mp_world_lock(mp_world);

    // Parse command
    char verb[64], noun[128];
    sscanf(command, "%63s %127[^\n]", verb, noun);

    // Get player
    Player* player = &mp_world->player_registry->players[player_number];
    const char* current_room_id = mp_world->current_room_per_player[player_number];

    // Find current room
    Room* current_room = NULL;
    for (int i = 0; i < mp_world->world.room_count; i++) {
        if (strcmp(mp_world->world.rooms[i].id, current_room_id) == 0) {
            current_room = &mp_world->world.rooms[i];
            break;
        }
    }

    if (!current_room) {
        snprintf(result, result_max, "ERROR: Invalid room state");
        mp_world_unlock(mp_world);
        return false;
    }

    bool success = false;

    // Process command based on verb
    if (strcasecmp(verb, "look") == 0 || strcasecmp(verb, "l") == 0) {
        // LOOK command
        snprintf(result, result_max, "%s\n%s",
                current_room->name, current_room->description);

        // Show items in room
        // Show other players in room
        // etc.

        success = true;

    } else if (strcasecmp(verb, "go") == 0 || strcasecmp(verb, "north") == 0 ||
               strcasecmp(verb, "south") == 0 || strcasecmp(verb, "east") == 0 ||
               strcasecmp(verb, "west") == 0 || strcasecmp(verb, "up") == 0 ||
               strcasecmp(verb, "down") == 0) {

        // MOVEMENT command
        Direction dir;
        if (strcasecmp(verb, "go") == 0) {
            // Parse direction from noun
            // dir = parse_direction(noun);
        } else {
            // Verb is the direction
            // dir = parse_direction(verb);
        }

        // Check if exit exists
        // Move player
        // Update state
        // Add event

        success = true;

    } else if (strcasecmp(verb, "take") == 0 || strcasecmp(verb, "get") == 0) {
        // TAKE command
        // Find item in room
        // Check if takeable
        // Check role permissions
        // Add to player inventory
        // Remove from room
        // Add event

        success = true;

    } else {
        snprintf(result, result_max, "Unknown command: %s", verb);
        success = false;
    }

    // Record action
    if (mp_world->action_index < MAX_RECENT_ACTIONS) {
        PlayerAction* action = &mp_world->recent_actions[mp_world->action_index];
        action->player_number = player_number;
        strncpy(action->command, command, 255);
        strncpy(action->result, result, 511);
        action->timestamp = time(NULL);
        action->state_version_after = mp_world->state_version;
        mp_world->action_index++;
    }

    if (success) {
        mp_world->state_version++;
        mp_world->last_update = time(NULL);
    }

    mp_world_unlock(mp_world);

    return success;
}

// Serialize state for sync (simplified version)
bool mp_world_serialize_state(MultiplayerWorld* mp_world, char* buffer, int max_len) {
    if (!mp_world || !buffer) return false;

    mp_world_lock(mp_world);

    int offset = 0;

    // Version
    offset += snprintf(buffer + offset, max_len - offset,
                      "VERSION:%lu\n", mp_world->state_version);

    // Player positions
    for (int i = 0; i < mp_world->player_registry->player_count; i++) {
        offset += snprintf(buffer + offset, max_len - offset,
                          "PLAYER:%d:%s:%s\n",
                          i,
                          mp_world->player_registry->players[i].username,
                          mp_world->current_room_per_player[i]);
    }

    // Item states
    for (int i = 0; i < mp_world->world.item_count; i++) {
        offset += snprintf(buffer + offset, max_len - offset,
                          "ITEM:%s:%d\n",
                          mp_world->world.items[i].id,
                          mp_world->item_taken_by_player[i]);
    }

    mp_world_unlock(mp_world);

    return true;
}

// TODO: Implement remaining functions
// - mp_world_remove_player
// - mp_world_mark_event_broadcasted
// - mp_world_deserialize_state
// - mp_world_save_to_file
// - mp_world_load_from_file
```

---

## Phase 2: Command Routing Architecture

### 2.1 Client-Server Message Flow

```
PLAYER CLIENT                  COORDINATOR               WORLD STATE
     │                              │                         │
     │  1. CMD: "go north"          │                         │
     ├─────────────────────────────>│                         │
     │                              │  2. Validate player     │
     │                              ├────────────────────────>│
     │                              │  3. Process command     │
     │                              │<────────────────────────┤
     │                              │  4. Mutate state        │
     │                              │                         │
     │  5. RESULT: "You go north"   │                         │
     │<─────────────────────────────┤                         │
     │                              │                         │
     │                              │  6. Generate EVENT      │
     │                              │<────────────────────────┤
     │                              │                         │
     │                              │  7. BROADCAST to all    │
     ├──────────────────────────────┤  players                │
ALL │  "Alice moved north"          │                         │
PLAYERS│                            │                         │
```

### 2.2 Command Handler Registry

**File**: `include/command_handler.h`

```c
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "multiplayer_world.h"

// Command handler function type
typedef bool (*CommandHandler)(MultiplayerWorld* world, int player_number,
                              const char* args, char* result, int result_max);

// Command registration
typedef struct {
    char verb[32];
    CommandHandler handler;
    bool requires_args;
    char usage[128];
    char description[256];
} CommandRegistration;

// Built-in command handlers
bool cmd_look(MultiplayerWorld* world, int player_number,
             const char* args, char* result, int result_max);
bool cmd_go(MultiplayerWorld* world, int player_number,
           const char* args, char* result, int result_max);
bool cmd_take(MultiplayerWorld* world, int player_number,
             const char* args, char* result, int result_max);
bool cmd_drop(MultiplayerWorld* world, int player_number,
             const char* args, char* result, int result_max);
bool cmd_examine(MultiplayerWorld* world, int player_number,
                const char* args, char* result, int result_max);
bool cmd_inventory(MultiplayerWorld* world, int player_number,
                  const char* args, char* result, int result_max);
bool cmd_say(MultiplayerWorld* world, int player_number,
            const char* args, char* result, int result_max);
bool cmd_signal(MultiplayerWorld* world, int player_number,
               const char* args, char* result, int result_max);

// Role-specific commands
bool cmd_inspect(MultiplayerWorld* world, int player_number,
                const char* args, char* result, int result_max);  // SCOUT
bool cmd_unlock(MultiplayerWorld* world, int player_number,
               const char* args, char* result, int result_max);   // ENGINEER
bool cmd_heal(MultiplayerWorld* world, int player_number,
             const char* args, char* result, int result_max);     // MEDIC
bool cmd_negotiate(MultiplayerWorld* world, int player_number,
                  const char* args, char* result, int result_max); // DIPLOMAT
bool cmd_inspire(MultiplayerWorld* world, int player_number,
                const char* args, char* result, int result_max);  // LEADER

// Command registry management
void init_command_registry(void);
CommandRegistration* find_command(const char* verb);
bool dispatch_command(MultiplayerWorld* world, int player_number,
                     const char* input, char* result, int result_max);

#endif // COMMAND_HANDLER_H
```

### 2.3 Sample Command Implementation

**File**: `src/command_handler.c`

```c
#include "command_handler.h"
#include "parser.h"
#include <string.h>
#include <stdio.h>

static CommandRegistration g_commands[50];
static int g_command_count = 0;

void init_command_registry(void) {
    g_command_count = 0;

    // Register base commands
    CommandRegistration cmds[] = {
        {"look", cmd_look, false, "look", "Look around the current room"},
        {"l", cmd_look, false, "l", "Shortcut for look"},
        {"go", cmd_go, true, "go <direction>", "Move in a direction"},
        {"north", cmd_go, false, "north", "Go north"},
        {"south", cmd_go, false, "south", "Go south"},
        {"east", cmd_go, false, "east", "Go east"},
        {"west", cmd_go, false, "west", "Go west"},
        {"up", cmd_go, false, "up", "Go up"},
        {"down", cmd_go, false, "down", "Go down"},
        {"take", cmd_take, true, "take <item>", "Pick up an item"},
        {"get", cmd_take, true, "get <item>", "Pick up an item"},
        {"drop", cmd_drop, true, "drop <item>", "Drop an item"},
        {"examine", cmd_examine, true, "examine <item>", "Examine an item"},
        {"x", cmd_examine, true, "x <item>", "Shortcut for examine"},
        {"inventory", cmd_inventory, false, "inventory", "Show inventory"},
        {"i", cmd_inventory, false, "i", "Shortcut for inventory"},
        {"say", cmd_say, true, "say <message>", "Say something to team"},
        {"signal", cmd_signal, true, "signal <type>", "Send signal to team"},

        // Role-specific
        {"inspect", cmd_inspect, true, "inspect <target>", "[SCOUT] Inspect for secrets"},
        {"unlock", cmd_unlock, true, "unlock <target>", "[ENGINEER] Unlock mechanism"},
        {"heal", cmd_heal, true, "heal <player>", "[MEDIC] Heal a player"},
        {"negotiate", cmd_negotiate, true, "negotiate <npc>", "[DIPLOMAT] Talk to NPC"},
        {"inspire", cmd_inspire, false, "inspire", "[LEADER] Inspire the team"},
    };

    int num_cmds = sizeof(cmds) / sizeof(cmds[0]);
    for (int i = 0; i < num_cmds && g_command_count < 50; i++) {
        g_commands[g_command_count++] = cmds[i];
    }
}

CommandRegistration* find_command(const char* verb) {
    for (int i = 0; i < g_command_count; i++) {
        if (strcasecmp(g_commands[i].verb, verb) == 0) {
            return &g_commands[i];
        }
    }
    return NULL;
}

bool dispatch_command(MultiplayerWorld* world, int player_number,
                     const char* input, char* result, int result_max) {
    if (!world || !input || !result) return false;

    // Parse input
    char verb[64], args[256];
    sscanf(input, "%63s %255[^\n]", verb, args);

    // Find command
    CommandRegistration* cmd = find_command(verb);
    if (!cmd) {
        snprintf(result, result_max, "Unknown command: %s", verb);
        return false;
    }

    // Check if args required
    if (cmd->requires_args && strlen(args) == 0) {
        snprintf(result, result_max, "Usage: %s", cmd->usage);
        return false;
    }

    // Execute handler
    return cmd->handler(world, player_number, args, result, result_max);
}

// LOOK command implementation
bool cmd_look(MultiplayerWorld* world, int player_number,
             const char* args, char* result, int result_max) {
    (void)args;  // Unused

    const char* room_id = mp_world_get_player_room(world, player_number);
    if (!room_id) {
        snprintf(result, result_max, "ERROR: Cannot determine current room");
        return false;
    }

    mp_world_lock(world);

    // Find room
    Room* room = NULL;
    for (int i = 0; i < world->world.room_count; i++) {
        if (strcmp(world->world.rooms[i].id, room_id) == 0) {
            room = &world->world.rooms[i];
            break;
        }
    }

    if (!room) {
        snprintf(result, result_max, "ERROR: Room not found");
        mp_world_unlock(world);
        return false;
    }

    // Build description
    int offset = 0;
    offset += snprintf(result + offset, result_max - offset,
                      "\n=== %s ===\n", room->name);
    offset += snprintf(result + offset, result_max - offset,
                      "%s\n", room->description);

    // Show exits
    offset += snprintf(result + offset, result_max - offset, "\nExits: ");
    bool has_exits = false;
    for (int i = 0; i < DIR_COUNT; i++) {
        if (room->exits[i][0] != '\0') {
            if (has_exits) {
                offset += snprintf(result + offset, result_max - offset, ", ");
            }
            offset += snprintf(result + offset, result_max - offset, "%s",
                             direction_to_string((Direction)i));
            has_exits = true;
        }
    }
    if (!has_exits) {
        offset += snprintf(result + offset, result_max - offset, "none");
    }
    offset += snprintf(result + offset, result_max - offset, "\n");

    // Show items in room
    bool has_items = false;
    for (int i = 0; i < world->world.item_count; i++) {
        if (strcmp(world->world.items[i].location, room_id) == 0 &&
            world->item_taken_by_player[i] == -1) {
            if (!has_items) {
                offset += snprintf(result + offset, result_max - offset, "\nItems: ");
                has_items = true;
            } else {
                offset += snprintf(result + offset, result_max - offset, ", ");
            }
            offset += snprintf(result + offset, result_max - offset, "%s",
                             world->world.items[i].name);
        }
    }
    if (has_items) {
        offset += snprintf(result + offset, result_max - offset, "\n");
    }

    // Show other players in room
    bool has_players = false;
    for (int i = 0; i < world->player_registry->player_count; i++) {
        if (i != player_number &&
            strcmp(mp_world_get_player_room(world, i), room_id) == 0) {
            if (!has_players) {
                offset += snprintf(result + offset, result_max - offset, "\nPlayers here: ");
                has_players = true;
            } else {
                offset += snprintf(result + offset, result_max - offset, ", ");
            }
            offset += snprintf(result + offset, result_max - offset, "%s (%s)",
                             world->player_registry->players[i].username,
                             role_to_string(world->player_registry->players[i].role));
        }
    }
    if (has_players) {
        offset += snprintf(result + offset, result_max - offset, "\n");
    }

    mp_world_unlock(world);

    return true;
}

// GO command implementation
bool cmd_go(MultiplayerWorld* world, int player_number,
           const char* args, char* result, int result_max) {
    // Determine direction
    Direction dir;
    if (strcasecmp(args, "north") == 0 || strcasecmp(args, "n") == 0) {
        dir = DIR_NORTH;
    } else if (strcasecmp(args, "south") == 0 || strcasecmp(args, "s") == 0) {
        dir = DIR_SOUTH;
    } else if (strcasecmp(args, "east") == 0 || strcasecmp(args, "e") == 0) {
        dir = DIR_EAST;
    } else if (strcasecmp(args, "west") == 0 || strcasecmp(args, "w") == 0) {
        dir = DIR_WEST;
    } else if (strcasecmp(args, "up") == 0 || strcasecmp(args, "u") == 0) {
        dir = DIR_UP;
    } else if (strcasecmp(args, "down") == 0 || strcasecmp(args, "d") == 0) {
        dir = DIR_DOWN;
    } else {
        snprintf(result, result_max, "Invalid direction: %s", args);
        return false;
    }

    const char* current_room_id = mp_world_get_player_room(world, player_number);
    if (!current_room_id) {
        snprintf(result, result_max, "ERROR: Cannot determine current room");
        return false;
    }

    mp_world_lock(world);

    // Find current room
    Room* room = NULL;
    for (int i = 0; i < world->world.room_count; i++) {
        if (strcmp(world->world.rooms[i].id, current_room_id) == 0) {
            room = &world->world.rooms[i];
            break;
        }
    }

    if (!room) {
        snprintf(result, result_max, "ERROR: Room not found");
        mp_world_unlock(world);
        return false;
    }

    // Check if exit exists
    const char* next_room_id = room->exits[dir];
    if (next_room_id[0] == '\0') {
        snprintf(result, result_max, "You can't go that way.");
        mp_world_unlock(world);
        return false;
    }

    // Move player
    mp_world_set_player_room(world, player_number, next_room_id);

    // Find next room for name
    Room* next_room = NULL;
    for (int i = 0; i < world->world.room_count; i++) {
        if (strcmp(world->world.rooms[i].id, next_room_id) == 0) {
            next_room = &world->world.rooms[i];
            break;
        }
    }

    snprintf(result, result_max, "You go %s to %s.",
            direction_to_string(dir),
            next_room ? next_room->name : next_room_id);

    // Add event
    char detail[256];
    snprintf(detail, sizeof(detail), "%s:%s",
            direction_to_string(dir), next_room_id);
    mp_world_add_event(world, EVENT_PLAYER_MOVED, player_number, detail);

    mp_world_unlock(world);

    return true;
}

// TODO: Implement remaining commands
// - cmd_take
// - cmd_drop
// - cmd_examine
// - cmd_inventory
// - cmd_say
// - cmd_signal
// - cmd_inspect (SCOUT)
// - cmd_unlock (ENGINEER)
// - cmd_heal (MEDIC)
// - cmd_negotiate (DIPLOMAT)
// - cmd_inspire (LEADER)
```

---

## Phase 3: Integration with Session Coordinator

### 3.1 Modified Session Coordinator Architecture

**File**: `src/session_coordinator.c` (modifications)

```c
// Add to includes
#include "multiplayer_world.h"
#include "command_handler.h"

// Add to global state
typedef struct {
    Session session;
    MultiplayerWorld* mp_world;
    pthread_t game_thread;
    bool game_running;
} ActiveSession;

static ActiveSession g_active_sessions[MAX_SESSIONS];
static int g_active_session_count = 0;

// Game thread function
void* game_thread_func(void* arg) {
    ActiveSession* active_session = (ActiveSession*)arg;

    printf("[GAME THREAD] Started for session %s\n", active_session->session.id);

    while (active_session->game_running) {
        // Process pending events
        GameEvent events[10];
        int event_count = mp_world_get_pending_events(active_session->mp_world,
                                                       events, 10);

        for (int i = 0; i < event_count; i++) {
            // Broadcast event to all players
            // TODO: Send via IPC to all connected players

            // Mark as broadcasted
            // mp_world_mark_event_broadcasted(active_session->mp_world, i);
        }

        // Check for pending commands from players
        // TODO: Poll IPC channels for player commands

        // Sleep briefly
        usleep(50000);  // 50ms
    }

    printf("[GAME THREAD] Stopped for session %s\n", active_session->session.id);

    return NULL;
}

// Modified handle_start_session
bool handle_start_session_v2(const char* session_id) {
    // Find session in registry
    Session* session = registry_find_session(g_session_registry, session_id);
    if (!session) {
        fprintf(stderr, "Session not found: %s\n", session_id);
        return false;
    }

    // Create ActiveSession
    if (g_active_session_count >= MAX_SESSIONS) {
        fprintf(stderr, "Too many active sessions\n");
        return false;
    }

    ActiveSession* active = &g_active_sessions[g_active_session_count];
    memcpy(&active->session, session, sizeof(Session));

    // Create multiplayer world
    active->mp_world = mp_world_create(session_id, &active->session);
    if (!active->mp_world) {
        fprintf(stderr, "Failed to create multiplayer world\n");
        return false;
    }

    // Load world file for campaign
    char world_file[256];
    snprintf(world_file, sizeof(world_file), "realms/%s.realm",
            session->campaign_name);

    if (!mp_world_load(active->mp_world, world_file)) {
        fprintf(stderr, "Failed to load world file: %s\n", world_file);
        mp_world_destroy(active->mp_world);
        return false;
    }

    // Start game
    if (!mp_world_start(active->mp_world)) {
        fprintf(stderr, "Failed to start game\n");
        mp_world_destroy(active->mp_world);
        return false;
    }

    // Start game thread
    active->game_running = true;
    if (pthread_create(&active->game_thread, NULL, game_thread_func, active) != 0) {
        fprintf(stderr, "Failed to create game thread\n");
        mp_world_destroy(active->mp_world);
        return false;
    }

    g_active_session_count++;

    printf("Session %s started successfully\n", session_id);
    printf("  World: %s\n", world_file);
    printf("  Game thread running\n");

    return true;
}
```

### 3.2 Player Client Integration

**File**: `src/player_client.c` (NEW)

```c
/*
 * Player Client
 *
 * Connects to session coordinator and provides player UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "ipc.h"
#include "player.h"
#include "smartterm_simple.h"

typedef struct {
    char session_id[64];
    char username[64];
    PlayerRole role;
    int player_number;

    IPCChannel* channel;
    bool connected;
    bool running;

    pthread_t recv_thread;

} PlayerClient;

// Receive thread - handles messages from server
void* recv_thread_func(void* arg) {
    PlayerClient* client = (PlayerClient*)arg;

    while (client->running) {
        Message msg;
        if (ipc_receive(client->channel, &msg)) {
            // Process message based on type
            switch (msg.type) {
                case MSG_STATE:
                    // State update from server
                    st_append_output(msg.payload);
                    break;

                case MSG_EVENT:
                    // Game event
                    st_append_output(msg.payload);
                    st_append_output("\n");
                    break;

                case MSG_BROADCAST:
                    // Broadcast message
                    st_append_output("[BROADCAST] ");
                    st_append_output(msg.payload);
                    st_append_output("\n");
                    break;

                case MSG_ERROR:
                    // Error message
                    st_append_output("[ERROR] ");
                    st_append_output(msg.payload);
                    st_append_output("\n");
                    break;

                default:
                    break;
            }

            st_refresh();
        }

        usleep(10000);  // 10ms
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <session_id> <username> <role>\n", argv[0]);
        return 1;
    }

    PlayerClient client = {0};
    strncpy(client.session_id, argv[1], 63);
    strncpy(client.username, argv[2], 63);
    client.role = role_from_string(argv[3]);

    if (client.role == ROLE_NONE) {
        fprintf(stderr, "Invalid role: %s\n", argv[3]);
        return 1;
    }

    // Initialize IPC
    if (!ipc_init()) {
        fprintf(stderr, "Failed to initialize IPC\n");
        return 1;
    }

    // Create channel
    client.channel = ipc_channel_create(client.session_id, client.username);
    if (!client.channel) {
        fprintf(stderr, "Failed to create IPC channel\n");
        return 1;
    }

    if (!ipc_channel_open(client.channel)) {
        fprintf(stderr, "Failed to open IPC channel\n");
        return 1;
    }

    // Initialize smart terminal
    st_init("Adventure Engine - Multiplayer");

    st_append_output("=== ADVENTURE ENGINE - MULTIPLAYER ===\n");
    st_append_output("Session: ");
    st_append_output(client.session_id);
    st_append_output("\n");
    st_append_output("Player: ");
    st_append_output(client.username);
    st_append_output(" (");
    st_append_output(role_to_string(client.role));
    st_append_output(")\n\n");

    // Send join message
    char join_msg[256];
    snprintf(join_msg, sizeof(join_msg), "JOIN %s %s",
            client.username, role_to_string(client.role));
    ipc_send_string(client.channel, MSG_SIGNAL, join_msg);

    // Start receive thread
    client.running = true;
    pthread_create(&client.recv_thread, NULL, recv_thread_func, &client);

    // Main input loop
    char input[256];
    while (client.running) {
        if (st_get_input(input, sizeof(input))) {
            if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
                break;
            }

            // Send command to server
            ipc_send_string(client.channel, MSG_CMD, input);
        }
    }

    // Cleanup
    client.running = false;
    pthread_join(client.recv_thread, NULL);

    ipc_channel_close(client.channel);
    ipc_channel_destroy(client.channel);

    st_cleanup();

    return 0;
}
```

---

## Phase 4: Build System Integration

### 4.1 Makefile Updates

```makefile
# Multiplayer components (updated)
MP_CORE_SRC = $(SRC_DIR)/session.c $(SRC_DIR)/player.c $(SRC_DIR)/ipc.c \
              $(SRC_DIR)/multiplayer_world.c $(SRC_DIR)/command_handler.c
MP_CORE_OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(MP_CORE_SRC))

# Session coordinator (updated to include mp core)
MP_COORDINATOR_SRC = $(SRC_DIR)/session_coordinator.c
MP_COORDINATOR_OBJ = $(BUILD_DIR)/session_coordinator.o
MP_COORDINATOR_BIN = $(BUILD_DIR)/session-coordinator

# Player client (NEW)
MP_CLIENT_SRC = $(SRC_DIR)/player_client.c
MP_CLIENT_OBJ = $(BUILD_DIR)/player_client.o
MP_CLIENT_BIN = $(BUILD_DIR)/player-client

# Build coordinator
$(MP_COORDINATOR_BIN): $(MP_COORDINATOR_OBJ) $(MP_CORE_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

# Build player client
$(MP_CLIENT_BIN): $(MP_CLIENT_OBJ) $(MP_CORE_OBJ) $(LIB_PATH) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lpthread

multiplayer: $(MP_COORDINATOR_BIN) $(MP_CLIENT_BIN)

run-client: $(MP_CLIENT_BIN)
	$(MP_CLIENT_BIN) $(SESSION) $(USER) $(ROLE)
```

---

## Phase 5: Testing Strategy

### 5.1 Unit Tests

**File**: `tests/test_multiplayer_world.c`

```c
#include "multiplayer_world.h"
#include <assert.h>
#include <stdio.h>

void test_mp_world_create() {
    Session session = {0};
    strcpy(session.id, "TEST-SESSION");

    MultiplayerWorld* world = mp_world_create("TEST-SESSION", &session);
    assert(world != NULL);
    assert(strcmp(world->session_id, "TEST-SESSION") == 0);
    assert(world->state_version == 0);

    mp_world_destroy(world);
    printf("✓ test_mp_world_create passed\n");
}

void test_mp_world_add_player() {
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);

    Player player = {0};
    strcpy(player.username, "alice");
    player.role = ROLE_LEADER;
    player.player_number = 0;

    bool success = mp_world_add_player(world, &player);
    assert(success);
    assert(world->player_registry->player_count == 1);

    mp_world_destroy(world);
    printf("✓ test_mp_world_add_player passed\n");
}

void test_mp_world_event_queue() {
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);

    mp_world_add_event(world, EVENT_CUSTOM, -1, "Test event");
    assert(world->event_count == 1);

    GameEvent events[10];
    int count = mp_world_get_pending_events(world, events, 10);
    assert(count == 1);
    assert(events[0].type == EVENT_CUSTOM);

    mp_world_destroy(world);
    printf("✓ test_mp_world_event_queue passed\n");
}

int main() {
    printf("Running multiplayer world tests...\n");
    test_mp_world_create();
    test_mp_world_add_player();
    test_mp_world_event_queue();
    printf("All tests passed!\n");
    return 0;
}
```

### 5.2 Integration Test

**File**: `tests/integration_test.sh`

```bash
#!/bin/bash

echo "=== Multiplayer Integration Test ==="

# Start coordinator
./build/session-coordinator -d &
COORD_PID=$!
sleep 1

# Create session via coordinator
echo "create test_world gm 4 2" | ./build/session-coordinator -i &
sleep 1

# Start 4 player clients
./build/player-client TEST-SESSION alice LEADER &
P1=$!
./build/player-client TEST-SESSION bob SCOUT &
P2=$!
./build/player-client TEST-SESSION carol ENGINEER &
P3=$!
./build/player-client TEST-SESSION dave MEDIC &
P4=$!

echo "All players connected"
sleep 5

# Send commands
echo "look" > /tmp/adventure-engine/TEST-SESSION-alice.sock
echo "go north" > /tmp/adventure-engine/TEST-SESSION-alice.sock

sleep 2

# Cleanup
kill $P1 $P2 $P3 $P4 $COORD_PID
echo "Test complete"
```

---

## Phase 6: Deployment Checklist

### 6.1 Pre-deployment

- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Memory leak check (valgrind)
- [ ] Thread safety verified
- [ ] Error handling comprehensive
- [ ] Documentation updated

### 6.2 Deployment Steps

1. Build all components
2. Test single session with 2 players
3. Test multi-session with 8 players
4. Load test with rapid commands
5. Stress test with 100+ commands/sec
6. Failover test (kill coordinator, restart)

### 6.3 Monitoring

```bash
# Check active sessions
./build/session-coordinator -i
> list

# Check IPC sockets
ls -l /tmp/adventure-engine/

# Monitor logs
tail -f /tmp/adventure-sessions/*.log

# Check resource usage
top -p $(pgrep session-coordinator)
```

---

## Summary

This deep dive provides a complete blueprint for integrating the single-player game engine with the multiplayer backend. Key components:

1. **MultiplayerWorld structure** - Thread-safe shared state
2. **Command handler registry** - Extensible command system
3. **Player client** - Connects to coordinator
4. **Game thread** - Processes events and commands
5. **IPC integration** - Real-time communication
6. **Testing framework** - Unit and integration tests

**Estimated implementation time**: 12-16 hours

**Next steps**: Implement `multiplayer_world.c`, test with 2 players, expand to 8 players.

---

**Files to create**:
- `include/multiplayer_world.h`
- `include/command_handler.h`
- `src/multiplayer_world.c`
- `src/command_handler.c`
- `src/player_client.c`
- `tests/test_multiplayer_world.c`
- `tests/integration_test.sh`

**Total new code**: ~2,500 lines
