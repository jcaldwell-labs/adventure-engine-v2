# Deep Dive: Real-Time State Synchronization

**Status**: Implementation Blueprint
**Complexity**: Very High
**Estimated Effort**: 16-20 hours
**Priority**: Critical (enables smooth multiplayer)

---

## Problem Statement

In multiplayer environments, each player's view of the game must stay synchronized with the authoritative server state. Challenges include:

1. **Latency**: Network/IPC delays between client and server
2. **Consistency**: All clients see same state
3. **Efficiency**: Minimize bandwidth/message overhead
4. **Responsiveness**: Players see immediate feedback
5. **Conflict Resolution**: Handle simultaneous actions
6. **Reliability**: Handle disconnects and reconnects

---

## Architecture Overview

### State Synchronization Models

```
MODEL 1: FULL STATE BROADCAST (Simple, Inefficient)
┌─────────────────────────────────────────────────┐
│ Server                                          │
│  Every tick (100ms):                            │
│   - Serialize ENTIRE world state                │
│   - Broadcast to ALL clients                    │
│                                                  │
│  Problem: Massive bandwidth (KB per tick)       │
│  Use Case: Small games, <10 entities            │
└─────────────────────────────────────────────────┘

MODEL 2: DELTA COMPRESSION (Efficient)
┌─────────────────────────────────────────────────┐
│ Server                                          │
│  Track state_version per change                 │
│  Every tick:                                     │
│   - Calculate DELTA since last broadcast        │
│   - Send only CHANGES                           │
│   - Clients apply deltas incrementally          │
│                                                  │
│  Benefit: 10-100x less bandwidth                │
│  Use Case: Our implementation                   │
└─────────────────────────────────────────────────┘

MODEL 3: EVENT SOURCING (Most Efficient)
┌─────────────────────────────────────────────────┐
│ Server                                          │
│  Don't sync state directly                      │
│  Instead:                                        │
│   - Send EVENTS (player_moved, item_taken)      │
│   - Clients replay events to rebuild state      │
│                                                  │
│  Benefit: Minimal data, replay capability       │
│  Use Case: Advanced optimization                │
└─────────────────────────────────────────────────┘
```

### Chosen Approach: Hybrid Delta + Event

We'll use a **hybrid approach**:
- **Events** for player actions (move, take, etc.)
- **Delta sync** for periodic full state consistency
- **Full sync** on connect/reconnect

---

## State Versioning System

### 1. State Version Tracking

**File**: `include/state_sync.h`

```c
#ifndef STATE_SYNC_H
#define STATE_SYNC_H

#include "multiplayer_world.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_STATE_DELTAS 100
#define MAX_CLIENTS_TRACKED 8

// State snapshot for versioning
typedef struct {
    uint64_t version;
    time_t timestamp;

    // Snapshot of world state
    char player_rooms[MAX_PLAYERS][32];
    int item_owners[MAX_ITEMS];
    bool room_visited[MAX_ROOMS];

    // Hash for quick comparison
    uint32_t state_hash;

} StateSnapshot;

// Delta between two state versions
typedef struct {
    uint64_t from_version;
    uint64_t to_version;

    // Changed fields
    int changed_player_rooms[MAX_PLAYERS];  // -1 if unchanged, else index
    int changed_item_owners[MAX_ITEMS];     // -1 if unchanged, else new owner
    bool changed_rooms_visited[MAX_ROOMS];

    int num_changes;
    time_t created_at;

} StateDelta;

// Client sync state
typedef struct {
    char player_id[64];
    uint64_t last_synced_version;
    time_t last_sync_time;
    int pending_deltas;
    bool needs_full_sync;
} ClientSyncState;

// Sync manager
typedef struct {
    MultiplayerWorld* world;

    // State snapshots (circular buffer)
    StateSnapshot snapshots[MAX_STATE_DELTAS];
    int snapshot_head;
    int snapshot_count;

    // Delta cache
    StateDelta deltas[MAX_STATE_DELTAS];
    int delta_count;

    // Client tracking
    ClientSyncState clients[MAX_CLIENTS_TRACKED];
    int client_count;

    // Configuration
    int snapshot_interval_ms;  // How often to snapshot (default: 1000ms)
    int sync_interval_ms;      // How often to sync (default: 100ms)
    int max_delta_chain;       // Max deltas before full sync (default: 10)

    // Stats
    uint64_t total_syncs;
    uint64_t total_deltas_sent;
    uint64_t total_full_syncs;
    uint64_t bytes_sent;

} SyncManager;

// Initialization
SyncManager* sync_manager_create(MultiplayerWorld* world);
void sync_manager_destroy(SyncManager* manager);

// Configuration
void sync_manager_set_intervals(SyncManager* manager, int snapshot_ms, int sync_ms);

// Client management
bool sync_manager_add_client(SyncManager* manager, const char* player_id);
bool sync_manager_remove_client(SyncManager* manager, const char* player_id);
ClientSyncState* sync_manager_get_client(SyncManager* manager, const char* player_id);

// Snapshot management
bool sync_manager_create_snapshot(SyncManager* manager);
StateSnapshot* sync_manager_get_snapshot(SyncManager* manager, uint64_t version);
StateSnapshot* sync_manager_get_latest_snapshot(SyncManager* manager);

// Delta calculation
StateDelta* sync_manager_calculate_delta(SyncManager* manager,
                                         uint64_t from_version,
                                         uint64_t to_version);

// Synchronization
bool sync_manager_sync_client(SyncManager* manager, const char* player_id,
                              char* out_message, int max_len);
bool sync_manager_sync_all_clients(SyncManager* manager);

// Serialization
bool sync_serialize_delta(const StateDelta* delta, char* buffer, int max_len);
bool sync_deserialize_delta(StateDelta* delta, const char* buffer);
bool sync_serialize_snapshot(const StateSnapshot* snapshot, char* buffer, int max_len);
bool sync_deserialize_snapshot(StateSnapshot* snapshot, const char* buffer);

// Utilities
uint32_t sync_calculate_state_hash(const MultiplayerWorld* world);
bool sync_validate_delta(const StateDelta* delta);

#endif // STATE_SYNC_H
```

### 2. Implementation - Core Sync Manager

**File**: `src/state_sync.c`

```c
#include "state_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create sync manager
SyncManager* sync_manager_create(MultiplayerWorld* world) {
    if (!world) return NULL;

    SyncManager* manager = (SyncManager*)calloc(1, sizeof(SyncManager));
    if (!manager) {
        perror("Failed to allocate sync manager");
        return NULL;
    }

    manager->world = world;
    manager->snapshot_head = 0;
    manager->snapshot_count = 0;
    manager->delta_count = 0;
    manager->client_count = 0;

    // Default intervals
    manager->snapshot_interval_ms = 1000;  // 1 second
    manager->sync_interval_ms = 100;       // 100ms
    manager->max_delta_chain = 10;

    // Stats
    manager->total_syncs = 0;
    manager->total_deltas_sent = 0;
    manager->total_full_syncs = 0;
    manager->bytes_sent = 0;

    // Create initial snapshot
    sync_manager_create_snapshot(manager);

    return manager;
}

void sync_manager_destroy(SyncManager* manager) {
    if (manager) {
        free(manager);
    }
}

void sync_manager_set_intervals(SyncManager* manager, int snapshot_ms, int sync_ms) {
    if (!manager) return;
    manager->snapshot_interval_ms = snapshot_ms;
    manager->sync_interval_ms = sync_ms;
}

// Add client
bool sync_manager_add_client(SyncManager* manager, const char* player_id) {
    if (!manager || !player_id || manager->client_count >= MAX_CLIENTS_TRACKED) {
        return false;
    }

    ClientSyncState* client = &manager->clients[manager->client_count];
    strncpy(client->player_id, player_id, 63);
    client->last_synced_version = 0;
    client->last_sync_time = time(NULL);
    client->pending_deltas = 0;
    client->needs_full_sync = true;  // First sync is always full

    manager->client_count++;

    return true;
}

// Get client state
ClientSyncState* sync_manager_get_client(SyncManager* manager, const char* player_id) {
    if (!manager || !player_id) return NULL;

    for (int i = 0; i < manager->client_count; i++) {
        if (strcmp(manager->clients[i].player_id, player_id) == 0) {
            return &manager->clients[i];
        }
    }

    return NULL;
}

// Create snapshot of current state
bool sync_manager_create_snapshot(SyncManager* manager) {
    if (!manager || !manager->world) return false;

    mp_world_lock(manager->world);

    int index = manager->snapshot_head;
    StateSnapshot* snapshot = &manager->snapshots[index];

    snapshot->version = manager->world->state_version;
    snapshot->timestamp = time(NULL);

    // Copy player positions
    for (int i = 0; i < MAX_PLAYERS; i++) {
        strncpy(snapshot->player_rooms[i],
               manager->world->current_room_per_player[i], 31);
    }

    // Copy item ownership
    for (int i = 0; i < MAX_ITEMS; i++) {
        snapshot->item_owners[i] = manager->world->item_taken_by_player[i];
    }

    // Copy room visited flags
    for (int i = 0; i < MAX_ROOMS; i++) {
        snapshot->room_visited[i] = manager->world->room_visited_by_anyone[i];
    }

    // Calculate hash
    snapshot->state_hash = sync_calculate_state_hash(manager->world);

    mp_world_unlock(manager->world);

    // Update circular buffer
    manager->snapshot_head = (manager->snapshot_head + 1) % MAX_STATE_DELTAS;
    if (manager->snapshot_count < MAX_STATE_DELTAS) {
        manager->snapshot_count++;
    }

    return true;
}

// Get snapshot by version
StateSnapshot* sync_manager_get_snapshot(SyncManager* manager, uint64_t version) {
    if (!manager) return NULL;

    for (int i = 0; i < manager->snapshot_count; i++) {
        if (manager->snapshots[i].version == version) {
            return &manager->snapshots[i];
        }
    }

    return NULL;
}

StateSnapshot* sync_manager_get_latest_snapshot(SyncManager* manager) {
    if (!manager || manager->snapshot_count == 0) return NULL;

    int latest_index = (manager->snapshot_head - 1 + MAX_STATE_DELTAS) % MAX_STATE_DELTAS;
    return &manager->snapshots[latest_index];
}

// Calculate delta between two versions
StateDelta* sync_manager_calculate_delta(SyncManager* manager,
                                         uint64_t from_version,
                                         uint64_t to_version) {
    if (!manager) return NULL;

    StateSnapshot* from_snap = sync_manager_get_snapshot(manager, from_version);
    StateSnapshot* to_snap = sync_manager_get_snapshot(manager, to_version);

    if (!from_snap || !to_snap) {
        return NULL;  // Versions not in cache
    }

    // Allocate delta (in practice, use delta cache)
    if (manager->delta_count >= MAX_STATE_DELTAS) {
        return NULL;  // Delta cache full
    }

    StateDelta* delta = &manager->deltas[manager->delta_count];
    delta->from_version = from_version;
    delta->to_version = to_version;
    delta->created_at = time(NULL);
    delta->num_changes = 0;

    // Compare player rooms
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (strcmp(from_snap->player_rooms[i], to_snap->player_rooms[i]) != 0) {
            delta->changed_player_rooms[i] = i;
            delta->num_changes++;
        } else {
            delta->changed_player_rooms[i] = -1;
        }
    }

    // Compare item ownership
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (from_snap->item_owners[i] != to_snap->item_owners[i]) {
            delta->changed_item_owners[i] = to_snap->item_owners[i];
            delta->num_changes++;
        } else {
            delta->changed_item_owners[i] = -1;
        }
    }

    // Compare room visited
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (from_snap->room_visited[i] != to_snap->room_visited[i]) {
            delta->changed_rooms_visited[i] = to_snap->room_visited[i];
            delta->num_changes++;
        } else {
            delta->changed_rooms_visited[i] = false;
        }
    }

    manager->delta_count++;

    return delta;
}

// Sync a specific client
bool sync_manager_sync_client(SyncManager* manager, const char* player_id,
                              char* out_message, int max_len) {
    if (!manager || !player_id || !out_message) return false;

    ClientSyncState* client = sync_manager_get_client(manager, player_id);
    if (!client) return false;

    uint64_t current_version = mp_world_get_version(manager->world);

    // Check if client needs full sync
    if (client->needs_full_sync ||
        client->last_synced_version == 0 ||
        client->pending_deltas > manager->max_delta_chain) {

        // FULL SYNC
        StateSnapshot* latest = sync_manager_get_latest_snapshot(manager);
        if (!latest) return false;

        bool success = sync_serialize_snapshot(latest, out_message, max_len);
        if (success) {
            client->last_synced_version = current_version;
            client->last_sync_time = time(NULL);
            client->pending_deltas = 0;
            client->needs_full_sync = false;

            manager->total_full_syncs++;
            manager->bytes_sent += strlen(out_message);
        }

        return success;

    } else if (client->last_synced_version < current_version) {

        // DELTA SYNC
        StateDelta* delta = sync_manager_calculate_delta(manager,
                                                         client->last_synced_version,
                                                         current_version);

        if (!delta) {
            // Delta calculation failed, fall back to full sync
            client->needs_full_sync = true;
            return sync_manager_sync_client(manager, player_id, out_message, max_len);
        }

        bool success = sync_serialize_delta(delta, out_message, max_len);
        if (success) {
            client->last_synced_version = current_version;
            client->last_sync_time = time(NULL);
            client->pending_deltas++;

            manager->total_deltas_sent++;
            manager->bytes_sent += strlen(out_message);
        }

        return success;

    } else {
        // Client is up to date
        out_message[0] = '\0';
        return true;
    }
}

// Sync all clients
bool sync_manager_sync_all_clients(SyncManager* manager) {
    if (!manager) return false;

    char message[4096];

    for (int i = 0; i < manager->client_count; i++) {
        sync_manager_sync_client(manager, manager->clients[i].player_id,
                                message, sizeof(message));

        // In real implementation, send message via IPC
        // ipc_send_to_client(manager->clients[i].player_id, message);
    }

    manager->total_syncs++;

    return true;
}

// Serialize delta to text format
bool sync_serialize_delta(const StateDelta* delta, char* buffer, int max_len) {
    if (!delta || !buffer) return false;

    int offset = 0;

    // Header
    offset += snprintf(buffer + offset, max_len - offset,
                      "DELTA:%lu:%lu:%d\n",
                      delta->from_version, delta->to_version, delta->num_changes);

    // Player room changes
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (delta->changed_player_rooms[i] >= 0) {
            offset += snprintf(buffer + offset, max_len - offset,
                             "PLAYER_ROOM:%d\n", i);
            // In full implementation, include new room ID
        }
    }

    // Item ownership changes
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (delta->changed_item_owners[i] >= 0) {
            offset += snprintf(buffer + offset, max_len - offset,
                             "ITEM_OWNER:%d:%d\n", i, delta->changed_item_owners[i]);
        }
    }

    // Room visited changes
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (delta->changed_rooms_visited[i]) {
            offset += snprintf(buffer + offset, max_len - offset,
                             "ROOM_VISITED:%d\n", i);
        }
    }

    offset += snprintf(buffer + offset, max_len - offset, "END\n");

    return true;
}

// Serialize full snapshot
bool sync_serialize_snapshot(const StateSnapshot* snapshot, char* buffer, int max_len) {
    if (!snapshot || !buffer) return false;

    int offset = 0;

    offset += snprintf(buffer + offset, max_len - offset,
                      "SNAPSHOT:%lu:%u\n",
                      snapshot->version, snapshot->state_hash);

    // Serialize all player rooms
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (snapshot->player_rooms[i][0] != '\0') {
            offset += snprintf(buffer + offset, max_len - offset,
                             "PLAYER:%d:%s\n", i, snapshot->player_rooms[i]);
        }
    }

    // Serialize all item owners
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (snapshot->item_owners[i] >= 0) {
            offset += snprintf(buffer + offset, max_len - offset,
                             "ITEM:%d:%d\n", i, snapshot->item_owners[i]);
        }
    }

    // Serialize room visited
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (snapshot->room_visited[i]) {
            offset += snprintf(buffer + offset, max_len - offset,
                             "VISITED:%d\n", i);
        }
    }

    offset += snprintf(buffer + offset, max_len - offset, "END\n");

    return true;
}

// Calculate state hash (simple CRC-like)
uint32_t sync_calculate_state_hash(const MultiplayerWorld* world) {
    if (!world) return 0;

    uint32_t hash = 0;

    // Hash player positions
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const char* room = world->current_room_per_player[i];
        for (int j = 0; room[j] != '\0'; j++) {
            hash = hash * 31 + room[j];
        }
    }

    // Hash item ownership
    for (int i = 0; i < MAX_ITEMS; i++) {
        hash = hash * 31 + world->item_taken_by_player[i];
    }

    // Hash version
    hash = hash * 31 + (uint32_t)world->state_version;

    return hash;
}
```

---

## Client-Side State Management

### 3. Client State Tracker

**File**: `include/client_state.h`

```c
#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_LOCAL_PLAYERS 8
#define MAX_LOCAL_ITEMS 50

// Client's local view of the world
typedef struct {
    uint64_t version;
    time_t last_update;

    // Players
    int player_count;
    struct {
        char username[64];
        char current_room[32];
        int health;
        int energy;
    } players[MAX_LOCAL_PLAYERS];

    // Items (simplified)
    int item_count;
    struct {
        char id[32];
        char name[64];
        int owner;  // -1 if in world, player_number if taken
        char location[32];
    } items[MAX_LOCAL_ITEMS];

    // Room visited tracking
    bool rooms_visited[MAX_ROOMS];

    // Pending local changes (optimistic updates)
    bool has_pending_changes;
    uint64_t pending_since_version;

} ClientState;

// Initialize client state
ClientState* client_state_create(void);
void client_state_destroy(ClientState* state);

// Apply updates from server
bool client_state_apply_snapshot(ClientState* state, const char* snapshot_data);
bool client_state_apply_delta(ClientState* state, const char* delta_data);

// Local optimistic updates (before server confirmation)
bool client_state_predict_move(ClientState* state, int player_num, const char* new_room);
bool client_state_predict_take_item(ClientState* state, int player_num, const char* item_id);

// Rollback if server rejects
bool client_state_rollback(ClientState* state, uint64_t to_version);

// Queries
const char* client_state_get_player_room(ClientState* state, int player_num);
int client_state_get_item_owner(ClientState* state, const char* item_id);

#endif // CLIENT_STATE_H
```

### 4. Optimistic Updates

**File**: `src/client_state.c`

```c
#include "client_state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ClientState* client_state_create(void) {
    ClientState* state = (ClientState*)calloc(1, sizeof(ClientState));
    if (!state) {
        perror("Failed to allocate client state");
        return NULL;
    }

    state->version = 0;
    state->last_update = time(NULL);
    state->player_count = 0;
    state->item_count = 0;
    state->has_pending_changes = false;

    return state;
}

void client_state_destroy(ClientState* state) {
    if (state) {
        free(state);
    }
}

// Apply snapshot from server
bool client_state_apply_snapshot(ClientState* state, const char* snapshot_data) {
    if (!state || !snapshot_data) return false;

    // Parse snapshot
    // Format: "SNAPSHOT:<version>:<hash>\nPLAYER:...\nITEM:...\n"

    char line[256];
    const char* ptr = snapshot_data;

    // Parse header
    if (sscanf(ptr, "SNAPSHOT:%lu:", &state->version) != 1) {
        return false;
    }

    // Find first newline
    ptr = strchr(ptr, '\n');
    if (!ptr) return false;
    ptr++;

    // Reset state
    state->player_count = 0;
    state->item_count = 0;

    // Parse lines
    while (*ptr != '\0') {
        // Copy line
        const char* line_end = strchr(ptr, '\n');
        if (!line_end) break;

        int line_len = line_end - ptr;
        if (line_len >= 256) line_len = 255;
        strncpy(line, ptr, line_len);
        line[line_len] = '\0';

        // Parse line
        if (strncmp(line, "PLAYER:", 7) == 0) {
            int player_num;
            char room[32];
            if (sscanf(line, "PLAYER:%d:%31s", &player_num, room) == 2) {
                if (player_num < MAX_LOCAL_PLAYERS) {
                    strncpy(state->players[player_num].current_room, room, 31);
                    if (player_num >= state->player_count) {
                        state->player_count = player_num + 1;
                    }
                }
            }
        } else if (strncmp(line, "ITEM:", 5) == 0) {
            int item_num, owner;
            if (sscanf(line, "ITEM:%d:%d", &item_num, &owner) == 2) {
                if (item_num < MAX_LOCAL_ITEMS) {
                    state->items[item_num].owner = owner;
                    if (item_num >= state->item_count) {
                        state->item_count = item_num + 1;
                    }
                }
            }
        } else if (strncmp(line, "VISITED:", 8) == 0) {
            int room_num;
            if (sscanf(line, "VISITED:%d", &room_num) == 1) {
                if (room_num < MAX_ROOMS) {
                    state->rooms_visited[room_num] = true;
                }
            }
        } else if (strcmp(line, "END") == 0) {
            break;
        }

        ptr = line_end + 1;
    }

    state->last_update = time(NULL);
    state->has_pending_changes = false;

    return true;
}

// Apply delta
bool client_state_apply_delta(ClientState* state, const char* delta_data) {
    if (!state || !delta_data) return false;

    // Similar parsing to snapshot, but only update changed fields
    // Format: "DELTA:<from>:<to>:<changes>\nPLAYER_ROOM:...\n"

    uint64_t from_version, to_version;
    int num_changes;

    if (sscanf(delta_data, "DELTA:%lu:%lu:%d", &from_version, &to_version, &num_changes) != 3) {
        return false;
    }

    // Verify we're applying delta to correct base version
    if (state->version != from_version) {
        fprintf(stderr, "Version mismatch: expected %lu, got %lu\n",
                state->version, from_version);
        return false;  // Need full sync
    }

    // Apply changes (implementation similar to snapshot parsing)
    // ...

    state->version = to_version;
    state->last_update = time(NULL);

    return true;
}

// Optimistic update - predict player movement before server confirms
bool client_state_predict_move(ClientState* state, int player_num, const char* new_room) {
    if (!state || player_num < 0 || player_num >= MAX_LOCAL_PLAYERS || !new_room) {
        return false;
    }

    strncpy(state->players[player_num].current_room, new_room, 31);
    state->has_pending_changes = true;
    state->pending_since_version = state->version;

    return true;
}

// Get player's current room
const char* client_state_get_player_room(ClientState* state, int player_num) {
    if (!state || player_num < 0 || player_num >= state->player_count) {
        return NULL;
    }

    return state->players[player_num].current_room;
}
```

---

## Synchronization Protocols

### 5. Sync Protocol Specification

```
PROTOCOL: Adventure Engine State Sync v1.0
==========================================

Message Types:
--------------

1. FULL_SYNC (Server → Client)
   When: Initial connect, reconnect, >10 deltas behind
   Format:
     SNAPSHOT:<version>:<hash>
     PLAYER:<id>:<room>:<health>:<energy>
     ITEM:<id>:<owner>:<location>
     VISITED:<room_id>
     END

   Size: ~2-5 KB for typical game

2. DELTA_SYNC (Server → Client)
   When: Regular sync (every 100ms)
   Format:
     DELTA:<from_version>:<to_version>:<num_changes>
     PLAYER_ROOM:<player_id>:<new_room>
     ITEM_OWNER:<item_id>:<new_owner>
     ROOM_VISITED:<room_id>
     END

   Size: ~100-500 bytes typical

3. EVENT (Server → All Clients)
   When: Significant game event occurs
   Format:
     EVENT:<type>:<player>:<detail>:<timestamp>

   Examples:
     EVENT:PLAYER_MOVED:alice:north:1234567890
     EVENT:ITEM_TAKEN:bob:golden_key:1234567891
     EVENT:PUZZLE_SOLVED:team:pressure_plates:1234567892

4. SYNC_REQUEST (Client → Server)
   When: Client detects desync (hash mismatch)
   Format:
     SYNC_REQUEST:<current_version>:<current_hash>

5. ACK (Client → Server)
   When: After applying sync
   Format:
     ACK:<version>:<hash>


Synchronization Flow:
--------------------

Normal Operation:
-----------------
Server                          Client
  │                               │
  │  1. Create snapshot (1s)      │
  ├──────────────────────────────>│
  │                               │
  │  2. Every 100ms:              │
  │     Calculate delta           │
  │     If changes:               │
  ├──DELTA:100:101:5─────────────>│
  │                               │ Apply delta
  │                               │ Update local state
  │<──ACK:101:abc123──────────────┤
  │                               │

Reconnect:
---------
Server                          Client (reconnecting)
  │                               │
  │<──SYNC_REQUEST:95:xyz────────┤ (client behind)
  │                               │
  │  Check version gap            │
  │  Gap = 6 (within threshold)   │
  ├──DELTA:95:101:──────────────>│
  │                               │ Apply delta
  │<──ACK:101:abc123──────────────┤
  │                               │

Large Desync:
------------
Server                          Client (way behind)
  │                               │
  │<──SYNC_REQUEST:50:old────────┤ (client 51 versions behind)
  │                               │
  │  Gap > threshold              │
  │  Send full snapshot           │
  ├──SNAPSHOT:101:abc123────────>│
  │     (full state)              │
  │                               │ Replace state
  │<──ACK:101:abc123──────────────┤
  │                               │


Conflict Resolution:
-------------------

Optimistic Prediction Conflict:
Client predicts: "go north"
Server rejects: "Door locked"

Client                          Server
  │  1. Local predict move        │
  ├──CMD:go north────────────────>│
  │  Show "Going north..."        │
  │                               │ Process
  │                               │ Check: door locked!
  │<──ERROR:door_locked───────────┤
  │  Rollback local prediction    │
  │  Show "Door is locked"        │
  │                               │
  │<──DELTA:101:102───────────────┤
  │  (no position change)         │


Hash Validation:
---------------

Every SNAPSHOT and periodic checks include state hash.
Client calculates local hash and compares.

If mismatch detected:
  Client sends SYNC_REQUEST
  Server sends FULL_SYNC

Hash algorithm (CRC-32 like):
  hash = 0
  for each player: hash = hash*31 + hash(player.room)
  for each item: hash = hash*31 + item.owner
  hash = hash*31 + state.version
```

---

## Performance Optimization

### 6. Bandwidth Optimization Techniques

```c
// Technique 1: Delta compression
// Instead of sending full room name "ancient_library",
// send room index: 5
// Savings: 15 bytes → 1 byte

// Technique 2: Binary protocol instead of text
// Text: "DELTA:100:101:5\n" (16 bytes)
// Binary: [0x02][0x64][0x65][0x05] (4 bytes)
// Savings: 75%

// Technique 3: Batch updates
// Instead of: PLAYER_ROOM:0\nPLAYER_ROOM:1\nPLAYER_ROOM:2\n
// Send: PLAYER_ROOMS:3:0,1,2\n
// Savings: ~40%

// Technique 4: Interest management
// Only sync what player can see/interact with
// If player in room A, don't send updates from distant room Z

// Technique 5: Interpolation
// Client interpolates between known states
// Server can send fewer updates

// Example implementation of binary protocol:
typedef struct {
    uint8_t msg_type;     // 1 byte
    uint64_t from_ver;    // 8 bytes
    uint64_t to_ver;      // 8 bytes
    uint16_t num_changes; // 2 bytes
    // Variable length change data
} __attribute__((packed)) BinaryDelta;
```

---

## Testing and Validation

### 7. Sync Test Suite

**File**: `tests/test_state_sync.c`

```c
#include "state_sync.h"
#include <assert.h>

void test_sync_create_snapshot() {
    // Setup
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);
    SyncManager* manager = sync_manager_create(world);

    // Test
    bool success = sync_manager_create_snapshot(manager);
    assert(success);
    assert(manager->snapshot_count == 2);  // Initial + new

    // Cleanup
    sync_manager_destroy(manager);
    mp_world_destroy(world);
}

void test_sync_calculate_delta() {
    // Setup
    Session session = {0};
    MultiplayerWorld* world = mp_world_create("TEST", &session);
    SyncManager* manager = sync_manager_create(world);

    // Create snapshots
    sync_manager_create_snapshot(manager);  // Version 0
    mp_world_set_player_room(world, 0, "new_room");  // Change state
    sync_manager_create_snapshot(manager);  // Version 1

    // Calculate delta
    StateDelta* delta = sync_manager_calculate_delta(manager, 0, 1);
    assert(delta != NULL);
    assert(delta->num_changes > 0);
    assert(delta->changed_player_rooms[0] >= 0);

    // Cleanup
    sync_manager_destroy(manager);
    mp_world_destroy(world);
}

void test_sync_serialize_delta() {
    // Test delta serialization
    StateDelta delta = {0};
    delta.from_version = 100;
    delta.to_version = 101;
    delta.num_changes = 1;
    delta.changed_player_rooms[0] = 0;

    char buffer[1024];
    bool success = sync_serialize_delta(&delta, buffer, sizeof(buffer));
    assert(success);
    assert(strstr(buffer, "DELTA:100:101:1") != NULL);
}

void test_client_state_apply_delta() {
    ClientState* state = client_state_create();
    state->version = 100;
    strcpy(state->players[0].current_room, "old_room");

    const char* delta_data = "DELTA:100:101:1\nPLAYER_ROOM:0\nEND\n";
    bool success = client_state_apply_delta(state, delta_data);
    assert(success);
    assert(state->version == 101);

    client_state_destroy(state);
}

int main() {
    printf("Running state sync tests...\n");
    test_sync_create_snapshot();
    test_sync_calculate_delta();
    test_sync_serialize_delta();
    test_client_state_apply_delta();
    printf("All tests passed!\n");
    return 0;
}
```

### 8. Load Testing

**File**: `tests/sync_load_test.c`

```c
// Simulate high-frequency state changes
void test_sync_under_load() {
    SyncManager* manager = /* ... */;

    // Simulate 100 state changes per second
    for (int i = 0; i < 1000; i++) {
        // Change state
        mp_world_set_player_room(world, rand() % 4, "some_room");

        // Create snapshot every 100 changes
        if (i % 100 == 0) {
            sync_manager_create_snapshot(manager);
        }

        // Sync all clients
        sync_manager_sync_all_clients(manager);

        usleep(10000);  // 10ms
    }

    // Verify no memory leaks
    // Verify all clients in sync
    // Check bandwidth used
}
```

---

## Deployment Checklist

### Sync System Validation

- [ ] Unit tests passing (snapshot, delta, serialize)
- [ ] Integration tests passing (client-server sync)
- [ ] Load tests passing (100 changes/sec)
- [ ] Memory leak check (valgrind)
- [ ] Bandwidth measurement (<1 KB/sec typical)
- [ ] Latency measurement (<50ms sync delay)
- [ ] Conflict resolution tested
- [ ] Reconnect scenarios tested
- [ ] Hash validation working

---

## Summary

This deep dive provides a complete real-time state synchronization system:

1. **SyncManager** - Tracks snapshots and deltas
2. **ClientState** - Client-side state representation
3. **Delta compression** - Efficient bandwidth usage
4. **Optimistic updates** - Responsive client prediction
5. **Conflict resolution** - Handle simultaneous actions
6. **Hash validation** - Detect and fix desyncs
7. **Protocol specification** - Clear message formats
8. **Performance optimization** - Minimize overhead

**Key metrics**:
- Sync frequency: 100ms (10 FPS)
- Snapshot frequency: 1000ms (1 per second)
- Delta size: ~100-500 bytes
- Full sync size: ~2-5 KB
- Bandwidth: <1 KB/sec typical, <10 KB/sec peak

**Estimated implementation time**: 16-20 hours

**Files to create**:
- `include/state_sync.h`
- `include/client_state.h`
- `src/state_sync.c`
- `src/client_state.c`
- `tests/test_state_sync.c`
- `tests/sync_load_test.c`

**Total new code**: ~2,000 lines
