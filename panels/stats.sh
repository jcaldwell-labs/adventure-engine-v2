#!/bin/bash
#
# Stats Panel Updater
# Shows team health, inventory, and objectives
#

SESSION_ID="$1"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"
PIPE_STATS="$STATE_DIR/pipe_stats"

if [ ! -p "$PIPE_STATS" ]; then
    echo "ERROR: Stats pipe not found at $PIPE_STATS"
    exit 1
fi

# Function to render stats
render_stats() {
    cat << 'EOF'
╔══════════════════════════════════════╗
║         TEAM STATUS                  ║
╠══════════════════════════════════════╣
║                                      ║
║ PLAYERS:                             ║
║  @alice    (LEADER)    HP: ████ 100% ║
║  #bob      (SCOUT)     HP: ████ 100% ║
║  &carol    (ENGINEER)  HP: ████ 100% ║
║  +dave     (MEDIC)     HP: ████ 100% ║
║                                      ║
║ TEAM INVENTORY:                      ║
║  • Torch (alice)                     ║
║  • Rope (bob)                        ║
║  • Toolkit (carol)                   ║
║  • Med Kit (dave)                    ║
║                                      ║
║ OBJECTIVES:                          ║
║  ☐ Choose a path                     ║
║  ☐ Find the ancient artifact         ║
║  ☐ Solve the team puzzle             ║
║  ☐ Escape the chamber                ║
║                                      ║
║ TEAM SCORE: 0                        ║
║ TIME ELAPSED: 00:05:23               ║
║                                      ║
╚══════════════════════════════════════╝
EOF
}

# Initial render
exec > "$PIPE_STATS"
render_stats

# Update loop
while true; do
    sleep 1
    render_stats
done
