#!/bin/bash
#
# Narrative Panel Updater
# Updates the narrative panel with story text, NPC dialogue, and events
#

SESSION_ID="$1"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"
PIPE_NARRATIVE="$STATE_DIR/pipe_narrative"

if [ ! -p "$PIPE_NARRATIVE" ]; then
    echo "ERROR: Narrative pipe not found at $PIPE_NARRATIVE"
    exit 1
fi

# Clear screen and show header
clear
cat << 'EOF'
╔══════════════════════════════════════════════════════════════════╗
║                         NARRATIVE PANEL                          ║
╚══════════════════════════════════════════════════════════════════╝

EOF

# Sample narrative content
cat << 'EOF'
You and your team awaken in a dimly lit stone chamber. The air is
thick with dust and the smell of ancient secrets. Torches flicker
on the walls, casting dancing shadows across intricate carvings.

A booming voice echoes through the chamber:

"WELCOME, TRAVELERS. YOU HAVE BEEN CHOSEN FOR A GREAT CHALLENGE.
 WORK TOGETHER, OR PERISH ALONE. THE TRIALS AWAIT..."

The floor beneath you rumbles, and three doors appear on the far
wall, each marked with a different symbol:

  ⚔️  SWORD    - The Path of Valor
  📚 BOOK     - The Path of Wisdom
  🤝 HANDS    - The Path of Unity

Your team must decide which path to take. Some paths may require
specific roles or skills. Choose wisely...

EOF

# Keep the pipe open for updates
exec > "$PIPE_NARRATIVE"

while true; do
    # In a real implementation, this would read from session state
    # and update the narrative based on player actions
    sleep 1
done
