# Adventure Engine v3.0 - Multiplayer Team Building Platform

**Vision**: Transform the adventure engine into a tmux-based multiplayer team-building experience with real-time dashboards, collaborative mechanics, and campaign management.

---

## 🎯 Core Objectives

1. **Multiplayer Foundation**: Session-based multiplayer with shared state
2. **Tmux Integration**: Multi-panel dashboards for immersive experience
3. **Team Building**: Collaborative puzzles, roles, and shared objectives
4. **Realm Lifecycle**: Sophisticated state management and transitions
5. **Visual Enhancement**: boxes-live integration for ASCII art rendering
6. **Campaign Management**: Multi-realm progression with GM controls

---

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    TMUX SESSION MANAGER                      │
│  ┌──────────────┬──────────────┬──────────────┬──────────┐  │
│  │  NARRATIVE   │     MAP      │  TEAM STATS  │  GM CTRL │  │
│  │   PANEL      │    PANEL     │    PANEL     │  PANEL   │  │
│  ├──────────────┼──────────────┴──────────────┴──────────┤  │
│  │           PLAYER INPUT (multiplexed)                   │  │
│  └────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ▲ │
                            │ │ IPC (named pipes/unix sockets)
                            ▼ │
┌─────────────────────────────────────────────────────────────┐
│              SESSION COORDINATOR (daemon)                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Campaign State Manager                             │   │
│  │  - Active sessions                                  │   │
│  │  - Player registry                                  │   │
│  │  - Shared world state                               │   │
│  │  - Event queue & triggers                           │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Realm Lifecycle Engine                             │   │
│  │  - init → active → suspended → completed            │   │
│  │  - Event handlers                                   │   │
│  │  - State transitions                                │   │
│  │  - Condition evaluation                             │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ▲ │
                            │ │ File-based persistence
                            ▼ │
┌─────────────────────────────────────────────────────────────┐
│                    PERSISTENT STORAGE                        │
│  ~/.adventure-campaigns/                                     │
│    ├── active/        (running campaigns)                    │
│    ├── completed/     (finished campaigns)                   │
│    └── templates/     (campaign definitions)                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 Implementation Phases

### Phase 1: Multiplayer Foundation ⚡ HIGH PRIORITY

**Goal**: Enable multiple players to share game state and interact in real-time

#### 1.1 Session Management
- [x] Design session data structures
- [ ] Implement session coordinator daemon
- [ ] Create session lifecycle (create, join, leave, destroy)
- [ ] Add session discovery and listing
- [ ] Build session persistence

**Data Structures**:
```c
typedef struct {
    char id[64];              // Unique session ID
    char campaign_name[128];  // Campaign being played
    time_t created_at;
    time_t updated_at;
    int max_players;
    int current_players;
    SessionState state;       // LOBBY, ACTIVE, PAUSED, COMPLETED
} Session;

typedef struct {
    char username[64];
    char session_id[64];
    int player_number;
    PlayerRole role;          // LEADER, SCOUT, ENGINEER, MEDIC, etc.
    PlayerState state;        // CONNECTED, ACTIVE, DISCONNECTED
    time_t last_seen;
} Player;
```

#### 1.2 Shared State Protocol
- [ ] Design IPC mechanism (Unix domain sockets or named pipes)
- [ ] Implement state synchronization protocol
- [ ] Create message queue for commands/events
- [ ] Add conflict resolution for concurrent actions
- [ ] Build state broadcast system

**Protocol**:
```
MESSAGE_TYPE:SESSION_ID:PLAYER_ID:PAYLOAD

Types:
- CMD:    Player command
- STATE:  State update
- EVENT:  Game event
- SYNC:   Full state sync
- SIGNAL: Control signal
```

#### 1.3 Player Management
- [ ] Character creation system
- [ ] Role assignment mechanics
- [ ] Player authentication (simple token-based)
- [ ] Reconnection handling
- [ ] Player presence tracking

**Deliverable**: Multiple terminals can connect to same session and see synchronized state

---

### Phase 2: Tmux Integration 🖥️ HIGH PRIORITY

**Goal**: Create immersive multi-panel dashboard experience

#### 2.1 Tmux Session Manager
- [ ] Script to create tmux layouts programmatically
- [ ] Panel configuration system
- [ ] Dynamic panel spawning
- [ ] Panel focus management
- [ ] Cleanup and teardown scripts

**Script**: `bin/start-campaign`
```bash
#!/bin/bash
# Creates tmux session with 4-panel layout:
# ┌─────────────┬──────────┐
# │  NARRATIVE  │   MAP    │
# │             ├──────────┤
# │             │  STATS   │
# ├─────────────┴──────────┤
# │    PLAYER INPUT        │
# └────────────────────────┘
```

#### 2.2 Panel Implementations
- [ ] **Narrative Panel**: Story text, NPC dialogue, events
- [ ] **Map Panel**: ASCII art map with player positions
- [ ] **Stats Panel**: Team inventory, health, objectives
- [ ] **Log Panel**: Action history and team feed
- [ ] **GM Panel**: Admin controls for facilitator
- [ ] **Input Panel**: Command entry with readline

#### 2.3 Real-Time Updates
- [ ] Watch-based file updates for panels
- [ ] Efficient rendering (only changed content)
- [ ] Color coding and highlighting
- [ ] Panel-specific formatting
- [ ] Cursor management

**Technology**: Use `inotifywait` or polling with panel redraw scripts

---

### Phase 3: Enhanced Realm System 🌍 MEDIUM PRIORITY

**Goal**: Rich state management with lifecycle and events

#### 3.1 Realm Lifecycle
- [ ] Define lifecycle states: `INIT → ACTIVE → SUSPENDED → COMPLETED`
- [ ] State transition rules and validation
- [ ] Event hooks for lifecycle changes
- [ ] Conditional transitions based on objectives
- [ ] Lifecycle persistence

**Lifecycle Example**:
```
INIT: Realm loaded, NPCs initialized, items placed
  ↓
ACTIVE: Players can interact, events fire, time progresses
  ↓
SUSPENDED: Saved state, can resume later
  ↓
COMPLETED: Objectives met, transition to next realm
```

#### 3.2 Event System
- [ ] Event types: ENTER_ROOM, TAKE_ITEM, USE_ITEM, TALK_NPC, SOLVE_PUZZLE
- [ ] Event triggers and conditions
- [ ] Event handlers and callbacks
- [ ] Event queue and processing
- [ ] Event persistence and replay

**Event Format** (in .world files):
```
[EVENT:puzzle_solved]
trigger: item_used:key:door
condition: inventory_has:golden_key
action: unlock_exit:north
message: The door creaks open!
```

#### 3.3 Scripting Extensions
- [ ] Conditional logic in world files
- [ ] Variables and state flags
- [ ] Arithmetic and string operations
- [ ] Function-like event chains
- [ ] Script validation and testing

---

### Phase 4: Team Building Features 🤝 HIGH PRIORITY

**Goal**: Collaborative mechanics that require teamwork

#### 4.1 Role-Based Abilities
- [ ] Define roles: LEADER, SCOUT, ENGINEER, MEDIC, DIPLOMAT
- [ ] Role-specific commands and abilities
- [ ] Skill trees and progression
- [ ] Role synergies and combos
- [ ] Role-locked items and areas

**Roles**:
```c
typedef enum {
    ROLE_LEADER,    // Can inspire team, see objectives
    ROLE_SCOUT,     // Can reveal map, detect hidden items
    ROLE_ENGINEER,  // Can fix/build items, unlock mechanisms
    ROLE_MEDIC,     // Can heal team, remove debuffs
    ROLE_DIPLOMAT   // Can talk to NPCs, negotiate
} PlayerRole;
```

#### 4.2 Collaborative Puzzles
- [ ] Multi-player required puzzles (need 2+ people)
- [ ] Simultaneous action requirements
- [ ] Role-based puzzle components
- [ ] Communication challenges (info split across players)
- [ ] Trust-building mechanics

**Example Puzzle**:
```
PRESSURE PLATE PUZZLE:
- Two players must stand on plates simultaneously
- Third player must pull lever within 5 seconds
- Requires coordination and timing
```

#### 4.3 Shared Objectives
- [ ] Team-wide goals and win conditions
- [ ] Contribution tracking per player
- [ ] Milestone achievements
- [ ] Objective hint system
- [ ] Progress visualization

#### 4.4 Team Communication
- [ ] In-game chat/whisper system
- [ ] Team signals (alert, help, ready, etc.)
- [ ] Announcement broadcasting
- [ ] Location sharing
- [ ] Item trading between players

---

### Phase 5: Visual Enhancement 🎨 MEDIUM PRIORITY

**Goal**: Rich ASCII art rendering and visual feedback

#### 5.1 Boxes-Live Integration
- [ ] Research boxes library capabilities
- [ ] Integrate boxes-live rendering
- [ ] Create box templates for UI elements
- [ ] Add borders, frames, and decorations
- [ ] Implement color schemes

#### 5.2 Map Rendering
- [ ] Auto-generate ASCII maps from room connections
- [ ] Player position indicators
- [ ] Fog of war (unexplored areas)
- [ ] Item markers on map
- [ ] Dynamic map updates

**Example Map**:
```
╔═══════════════════════════╗
║    CRYSTAL CAVERNS MAP    ║
╠═══════════════════════════╣
║                           ║
║     [ ]─[ ]─[*]           ║  * = You are here
║      │       │            ║  @ = Other player
║     [@]─[ ]─[ ]           ║  ? = Unexplored
║      │                    ║  ! = Item/NPC
║     [!]─[?]               ║
║                           ║
╚═══════════════════════════╝
```

#### 5.3 Dynamic Visual Feedback
- [ ] Status indicators (health, energy, etc.)
- [ ] Progress bars for objectives
- [ ] Visual alerts and notifications
- [ ] ASCII art for special events
- [ ] Themed visual styles per realm

---

### Phase 6: Advanced Features 🚀 LOW PRIORITY

**Goal**: Deep gameplay systems

#### 6.1 NPC Dialogue System
- [ ] NPC data structures and AI states
- [ ] Branching dialogue trees
- [ ] Dialogue conditions and variables
- [ ] NPC memory and relationship tracking
- [ ] Quest giving and tracking

**Dialogue Format**:
```
[NPC:wizard]
name: Ancient Wizard
description: An old wizard in tattered robes.
dialogue: greeting
location: tower_top

[DIALOGUE:greeting]
npc: wizard
text: Greetings, travelers. What brings you to my tower?
options: 1=ask_quest, 2=ask_tower, 3=goodbye

[DIALOGUE:ask_quest]
text: We seek the Crystal of Light.
response: Ah, a noble quest! But first, you must prove yourselves...
action: give_quest:crystal_quest
next: quest_given
```

#### 6.2 Quest System
- [ ] Quest definitions in world files
- [ ] Quest stages and objectives
- [ ] Quest rewards and consequences
- [ ] Quest branching and choices
- [ ] Quest sharing across team

#### 6.3 Achievement System
- [ ] Achievement definitions
- [ ] Progress tracking
- [ ] Unlock conditions
- [ ] Rewards and badges
- [ ] Leaderboard integration

---

### Phase 7: Campaign Management 🎮 MEDIUM PRIORITY

**Goal**: Multi-realm campaigns with progression

#### 7.1 Campaign System
- [ ] Campaign definition format
- [ ] Multi-realm progression
- [ ] Campaign variables and state
- [ ] Campaign-wide objectives
- [ ] Campaign completion tracking

**Campaign Format**:
```
[CAMPAIGN:training_gauntlet]
name: Corporate Team Building Gauntlet
description: 5 realms testing communication, trust, and problem-solving
realms: intro_cabin, trust_fall_cave, puzzle_tower, diplomacy_hall, final_challenge
max_players: 4
min_players: 2
estimated_time: 90 minutes

[REALM_SEQUENCE]
1: intro_cabin (15 min)
2: trust_fall_cave (20 min)
3: puzzle_tower (25 min)
4: diplomacy_hall (15 min)
5: final_challenge (15 min)
```

#### 7.2 GM Dashboard
- [ ] Campaign overview panel
- [ ] Player monitoring
- [ ] Manual event triggers
- [ ] Hint system
- [ ] Difficulty adjustment
- [ ] Session recording/replay

#### 7.3 Analytics
- [ ] Session metrics (time, completion rate)
- [ ] Player contribution analysis
- [ ] Team dynamics insights
- [ ] Common failure points
- [ ] Export to reports

---

## 🛠️ Technical Stack

**Core**: C (existing codebase)
**IPC**: Unix domain sockets, named pipes
**UI**: tmux, ncurses, readline, boxes-live
**Scripting**: Bash for tmux automation
**Data**: Text-based formats (.world, .campaign, .session)
**Build**: Make with modular compilation

---

## 📦 New Directory Structure

```
adventure-engine-v2/
├── include/
│   ├── session.h           # Session management
│   ├── player.h            # Player/character system
│   ├── ipc.h              # Inter-process communication
│   ├── realm_lifecycle.h  # Realm state machine
│   ├── event.h            # Event system
│   ├── npc.h              # NPC and dialogue
│   ├── quest.h            # Quest system
│   └── campaign.h         # Campaign management
├── src/
│   ├── session_coordinator.c  # Main multiplayer daemon
│   ├── session.c
│   ├── player.c
│   ├── ipc.c
│   ├── realm_lifecycle.c
│   ├── event.c
│   ├── npc.c
│   ├── quest.c
│   └── campaign.c
├── bin/
│   ├── start-campaign        # Tmux session launcher
│   ├── join-session          # Player join script
│   ├── gm-dashboard          # GM control interface
│   ├── panel-narrative       # Narrative panel updater
│   ├── panel-map             # Map panel renderer
│   ├── panel-stats           # Stats panel updater
│   └── session-cleanup       # Cleanup script
├── panels/
│   ├── narrative.sh
│   ├── map.sh
│   ├── stats.sh
│   ├── log.sh
│   └── gm.sh
├── campaigns/
│   ├── intro_training.campaign
│   ├── advanced_team.campaign
│   └── custom/
├── realms/                   # Renamed from worlds/
│   ├── intro_cabin.realm
│   ├── trust_cave.realm
│   └── ...
└── docs/
    ├── MULTIPLAYER.md
    ├── TMUX-SETUP.md
    ├── CAMPAIGN-FORMAT.md
    ├── GM-GUIDE.md
    └── TEAM-BUILDING.md
```

---

## 🎯 Success Metrics

**Technical**:
- ✅ 4+ players can connect simultaneously
- ✅ State synchronization latency < 100ms
- ✅ Tmux panels update in real-time
- ✅ Session survives player disconnect/reconnect
- ✅ Campaign completion data persisted

**Gameplay**:
- ✅ 3+ collaborative puzzles requiring teamwork
- ✅ 5+ realms in complete campaign
- ✅ Role-based mechanics functional
- ✅ NPC dialogue with 3+ branches
- ✅ Visual map with live player positions

**Team Building**:
- ✅ Requires communication to succeed
- ✅ Trust-building mechanics present
- ✅ Role diversity encourages contribution
- ✅ Debrief analytics available
- ✅ Scalable to 2-8 players

---

## 🚀 Quick Start (Post-Implementation)

```bash
# Start a campaign as GM
./bin/start-campaign intro_training 4

# Players join from other terminals
./bin/join-session <session-id> <username> <role>

# Launch GM dashboard
./bin/gm-dashboard <session-id>

# Monitor session
tmux attach -t adventure-session-<id>
```

---

## 💡 Innovation Highlights

1. **Zero UI by Default**: Pure terminal, scriptable, no GUI bloat
2. **Tmux Native**: Leverages tmux panels for sophisticated layouts
3. **Text-Based Everything**: Campaigns, realms, events all in plain text
4. **IPC Architecture**: Modular design, can swap communication layer
5. **Team Building Focus**: Designed for corporate training and team bonding
6. **GM Facilitated**: Human facilitator can guide and adjust difficulty
7. **Observable State**: All state is readable and inspectable
8. **Session Recording**: Can replay and analyze sessions
9. **Boxes-Live Integration**: Professional ASCII art rendering
10. **Role-Based Design**: Encourages diverse participation

---

## 🎓 Use Cases

**Corporate Training**:
- Team communication workshops
- Leadership development
- Problem-solving exercises
- Remote team bonding

**Education**:
- Computer science teaching (state machines, IPC, protocols)
- Creative writing (world building)
- Game design fundamentals
- Collaborative learning

**Entertainment**:
- Multiplayer text adventures
- D&D-style campaigns
- Escape room experiences
- LAN party alternative

---

## ⏱️ Estimated Implementation Time

| Phase | Estimated Hours | Priority |
|-------|----------------|----------|
| Phase 1: Multiplayer Foundation | 12-16 hours | HIGH |
| Phase 2: Tmux Integration | 8-12 hours | HIGH |
| Phase 3: Realm System | 10-14 hours | MEDIUM |
| Phase 4: Team Building | 8-10 hours | HIGH |
| Phase 5: Visual Enhancement | 6-8 hours | MEDIUM |
| Phase 6: Advanced Features | 12-16 hours | LOW |
| Phase 7: Campaign Management | 8-10 hours | MEDIUM |
| **TOTAL** | **64-86 hours** | - |

**Realistic MVP**: Phases 1, 2, 4 (28-38 hours) = Basic multiplayer tmux experience

---

## 🔥 Let's Build This!

This is an ambitious transformation from single-player to multiplayer team-building platform. The architecture is modular, allowing incremental development and testing.

**Next Steps**:
1. Implement session coordinator daemon
2. Create IPC protocol
3. Build tmux launcher scripts
4. Develop first collaborative puzzle
5. Test with 4-player session

Ready to code! 🚀
