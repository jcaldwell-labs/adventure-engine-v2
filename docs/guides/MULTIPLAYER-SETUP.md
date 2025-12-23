# Multiplayer Setup Guide

## Overview

Adventure Engine v3.0 introduces a powerful multiplayer system designed for team-building experiences. Using tmux for visualization and Unix IPC for communication, it enables 2-8 players to collaborate in real-time text adventures.

---

## Architecture

```
┌─────────────────────────────────────────┐
│         TMUX CAMPAIGN SESSION           │
│                                         │
│  ┌──────────┬──────────┐               │
│  │ NARRATIVE│   MAP    │               │
│  ├──────────┼──────────┤               │
│  │  STATS   │   LOG    │               │
│  └──────────┴──────────┘               │
│                                         │
│  [Player Windows: P1, P2, P3, P4]      │
│  [GM Dashboard]                         │
└─────────────────────────────────────────┘
          │
          │ Named Pipes & IPC
          ▼
┌─────────────────────────────────────────┐
│    SESSION COORDINATOR (daemon)         │
│  - Manages sessions                     │
│  - Synchronizes state                   │
│  - Routes messages                      │
│  - Tracks players                       │
└─────────────────────────────────────────┘
```

---

## Prerequisites

### System Requirements

- Linux or macOS
- tmux >= 2.0
- ncurses library
- readline library
- gcc compiler

### Installation

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install tmux libncurses-dev libreadline-dev gcc make

# Install dependencies (macOS)
brew install tmux ncurses readline

# Build the engine
cd adventure-engine-v2
make all
```

---

## Quick Start

### 1. Start a Campaign (Game Master)

```bash
./bin/start-campaign intro_training 4
```

This creates:
- A tmux session with 4-panel dashboard
- Windows for up to 4 players
- A GM control panel
- Named pipes for inter-panel communication

### 2. Join the Session (Players)

Players join from separate terminals:

```bash
./bin/join-session SESS-20251119-143022-1234 alice LEADER
./bin/join-session SESS-20251119-143022-1234 bob SCOUT
./bin/join-session SESS-20251119-143022-1234 carol ENGINEER
./bin/join-session SESS-20251119-143022-1234 dave MEDIC
```

### 3. Navigate the Tmux Session

Once in tmux:

- **Ctrl+b, w**: Show window list
- **Ctrl+b, n**: Next window
- **Ctrl+b, p**: Previous window
- **Ctrl+b, 0-9**: Jump to window number
- **Ctrl+b, arrow**: Move between panes
- **Ctrl+b, d**: Detach (session continues running)

To reattach:
```bash
tmux attach -t adventure-SESS-20251119-143022-1234
```

---

## Campaign Session Layout

### Panel Breakdown

#### Narrative Panel (Top-Left)
- Story text and descriptions
- NPC dialogue
- Event notifications
- Puzzle clues

#### Map Panel (Top-Right)
- ASCII art map
- Player positions
- Explored/unexplored areas
- Points of interest

#### Team Stats Panel (Bottom-Left)
- Player health and energy
- Team inventory
- Active objectives
- Time elapsed

#### Team Log Panel (Bottom-Right)
- Action history
- Team chat messages
- System notifications
- Achievement unlocks

#### Player Windows
- Individual command input
- Personal inventory
- Role-specific information
- Private messages

#### GM Dashboard
- Session overview
- Player status
- Manual event triggers
- Hint system
- Analytics

---

## Player Roles

### LEADER
**Abilities:**
- Can see all team objectives
- Makes team decisions
- Unlocks leadership-required paths
- Can inspire team (boost morale)

**Best for:** Natural organizers, decision-makers

### SCOUT
**Abilities:**
- Reveals hidden areas on map
- Detects secret passages
- Faster movement
- Can mark points of interest

**Best for:** Explorers, detail-oriented people

### ENGINEER
**Abilities:**
- Unlocks mechanical puzzles
- Repairs broken items
- Builds tools from components
- Solves technical challenges

**Best for:** Problem-solvers, technical thinkers

### MEDIC
**Abilities:**
- Heals team members
- Removes debuffs/status effects
- Uses healing items more effectively
- Can revive downed players

**Best for:** Supportive players, caregivers

### DIPLOMAT
**Abilities:**
- Talks to NPCs
- Negotiates better outcomes
- Reads ancient texts
- Gains information others can't

**Best for:** Communicators, socially-oriented

### SPECIALIST
**Abilities:**
- Wildcard - can perform any action
- Less effective than dedicated roles
- Good for flexibility

**Best for:** Versatile players, small teams

---

## Session Management

### Creating a Session

```bash
# Start session coordinator (in background)
./build/session-coordinator -d &

# Create a session
./build/session-coordinator -i
> create intro_training alice 4 2
```

### Listing Active Sessions

```bash
./build/session-coordinator -i
> list
```

### Starting a Session

```bash
> start SESS-20251119-143022-1234
```

### Pausing/Resuming

Sessions can be paused and resumed:

```bash
# Detach from tmux (session continues)
Ctrl+b, d

# Reattach later
tmux attach -t adventure-SESS-20251119-143022-1234
```

---

## Collaborative Mechanics

### Multi-Player Puzzles

Some puzzles require multiple players:

**Example: Pressure Plate Puzzle**
```
[REQUIRES: All players must stand on plates simultaneously]

Player actions:
P1> go south
P2> go south
P3> go south
P4> go south

[System: All plates activated - door opens!]
```

### Role-Specific Actions

Certain actions require specific roles:

```
> unlock mechanism
[ERROR: Only an ENGINEER can unlock this mechanism]

<carol is ENGINEER>
carol> unlock mechanism
[SUCCESS: Mechanism unlocked!]
```

### Shared Objectives

Teams work toward common goals:

```
OBJECTIVE: Collect 3 Artifacts
- [X] Artifact of Valor (found by alice)
- [X] Artifact of Wisdom (found by bob)
- [ ] Artifact of Unity (not found)

Progress: 2/3
```

---

## Communication

### Team Chat

Players can communicate via the log panel:

```
alice> Which path should we take?
bob> I can scout ahead on the east path
carol> I'll check the mechanism on the north door
dave> I'll stay central in case anyone needs healing
```

### Signals

Quick communication shortcuts:

```
> signal help
[alice signals for HELP!]

> signal ready
[bob signals READY]

> signal danger
[carol signals DANGER!]
```

---

## Game Master (GM) Controls

### Manual Event Triggers

```
gm> trigger event puzzle_hint
[System: A mysterious voice whispers a hint...]

gm> spawn item healing_potion library
[Healing potion appears in library]
```

### Difficulty Adjustment

```
gm> adjust difficulty easy
[Puzzle requirements reduced]

gm> adjust difficulty hard
[Enemy health increased]
```

### Hints

```
gm> hint show objective
[Shows current objective to all players]

gm> hint reveal map
[Reveals hidden map area]
```

---

## Troubleshooting

### Session Won't Start

```bash
# Check if tmux is installed
which tmux

# Check for existing session
tmux list-sessions

# Kill old session if stuck
tmux kill-session -t adventure-SESS-...
```

### Panels Not Updating

```bash
# Check named pipes
ls -l /tmp/adventure-campaign-*/pipe_*

# Restart panel scripts
./panels/narrative.sh SESSION_ID &
./panels/map.sh SESSION_ID &
./panels/stats.sh SESSION_ID &
./panels/log.sh SESSION_ID &
```

### Players Can't Join

```bash
# Verify session is running
tmux has-session -t adventure-SESS-...

# Check session coordinator
ps aux | grep session-coordinator

# Restart coordinator
./build/session-coordinator -i
```

---

## Advanced Configuration

### Custom Panel Layouts

Edit `bin/start-campaign` to customize tmux layout:

```bash
# 3-panel layout instead of 4
tmux split-window -h -t "$TMUX_SESSION:campaign.0"
tmux split-window -v -t "$TMUX_SESSION:campaign.1"
```

### Custom Realms

Create new `.realm` files in `realms/` directory:

```
[WORLD]
name: My Custom Realm
start: entrance

[ROOM:entrance]
name: Entry Hall
description: Custom description here...
exits: north=hallway
```

### Custom Campaigns

Define campaigns in `campaigns/` directory:

```
[CAMPAIGN]
name: My Campaign
realms: realm1,realm2,realm3
min_players: 2
max_players: 6
```

---

## Performance Tips

1. **Keep panel update frequency reasonable** (1-3 seconds)
2. **Limit chat log to last 20 messages**
3. **Use session cleanup after completion**
4. **Close unused tmux sessions**

---

## Best Practices

### For Game Masters
- Brief players on roles before starting
- Monitor team dynamics
- Provide hints if stuck > 5 minutes
- Debrief after session

### For Players
- Communicate frequently
- Leverage your role's strengths
- Help other players
- Don't rush - think strategically

### For Teams
- Assign roles before starting
- Establish communication norms
- Share discoveries immediately
- Celebrate wins together

---

## Next Steps

- [Campaign Creation Guide](CAMPAIGN-FORMAT.md)
- [Realm Design Guide](REALM-FORMAT.md)
- [GM Facilitation Guide](GM-GUIDE.md)
- [API Reference](API-REFERENCE.md)

---

## Support

For issues or questions:
- GitHub: https://github.com/jcaldwell-labs/adventure-engine-v2
- Docs: https://github.com/jcaldwell-labs/adventure-engine-v2/docs

Happy adventuring! 🎮🗺️⚔️
