# Quick Start - Multiplayer Campaign

Get a team-building adventure running in 60 seconds!

---

## Step 1: Build (30 seconds)

```bash
cd adventure-engine-v2
make all
```

---

## Step 2: Launch Campaign (10 seconds)

```bash
./bin/start-campaign intro_training 4
```

When prompted, press `y` to attach to the session.

---

## Step 3: View the Dashboard (20 seconds)

You'll see a 4-panel tmux layout:

```
┌─────────────────┬──────────────┐
│   NARRATIVE     │     MAP      │
│   (story)       │  (players)   │
├─────────────────┼──────────────┤
│   TEAM STATS    │   LOG/CHAT   │
│   (health,inv)  │  (actions)   │
└─────────────────┴──────────────┘
```

**Navigate between panels:**
- `Ctrl+b` then arrow keys

**Switch to player windows:**
- `Ctrl+b, w` (show window list)
- Select with arrow keys

---

## Step 4: Simulate Players (In Reality)

In reality, other players would join from separate terminals:

```bash
# Terminal 2 (different computer/terminal)
./bin/join-session SESS-20251119-143022-1234 alice LEADER

# Terminal 3
./bin/join-session SESS-20251119-143022-1234 bob SCOUT

# Terminal 4
./bin/join-session SESS-20251119-143022-1234 carol ENGINEER

# Terminal 5
./bin/join-session SESS-20251119-143022-1234 dave MEDIC
```

---

## Step 5: Play!

### Common Commands

```
look              - Examine current room
go north          - Move north (also: south, east, west, up, down)
take <item>       - Pick up item
examine <item>    - Look at item closely
inventory         - Check your inventory
help              - Show all commands
```

### Role-Specific Actions

- **LEADER**: `inspire` - Boost team morale
- **SCOUT**: `scout` - Reveal hidden areas
- **ENGINEER**: `unlock` - Open mechanisms
- **MEDIC**: `heal <player>` - Heal teammate
- **DIPLOMAT**: `negotiate` - Talk to NPCs

---

## Detach and Reattach

### Detach (session keeps running)
```
Ctrl+b, d
```

### Reattach later
```bash
tmux attach -t adventure-SESS-20251119-143022-1234
```

---

## Cleanup

### End session
```bash
tmux kill-session -t adventure-SESS-20251119-143022-1234
```

### Clean up temp files
```bash
rm -rf /tmp/adventure-*
```

---

## What's Next?

- **Create custom realms**: Edit files in `realms/`
- **Design campaigns**: Edit files in `campaigns/`
- **Read full docs**: See `docs/MULTIPLAYER-SETUP.md`

---

## Roles Quick Reference

| Role       | Ability                  | Best For              |
|------------|--------------------------|-----------------------|
| LEADER     | Leadership, objectives   | Organizers            |
| SCOUT      | Reveal hidden areas      | Explorers             |
| ENGINEER   | Unlock mechanisms        | Problem-solvers       |
| MEDIC      | Heal teammates           | Supporters            |
| DIPLOMAT   | Talk to NPCs             | Communicators         |
| SPECIALIST | Wildcard (any ability)   | Flexible players      |

---

That's it! You now have a multiplayer team-building adventure engine running! 🎮
