#!/bin/bash
#
# Map Panel Updater
# Renders ASCII map showing player positions and explored areas
#

SESSION_ID="$1"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"
PIPE_MAP="$STATE_DIR/pipe_map"

if [ ! -p "$PIPE_MAP" ]; then
    echo "ERROR: Map pipe not found at $PIPE_MAP"
    exit 1
fi

# Function to render map
render_map() {
    cat << 'EOF'
╔══════════════════════════════════════╗
║          CAMPAIGN MAP                ║
╠══════════════════════════════════════╣
║                                      ║
║   [?]───[?]                          ║
║    │                                 ║
║   [*]───[?]───[?]                    ║
║    │     │                           ║
║   [@]───[?]───[?]                    ║
║                                      ║
║  LEGEND:                             ║
║  * = Starting Chamber (current)      ║
║  @ = Player 1 (Leader)               ║
║  # = Player 2 (Scout)                ║
║  & = Player 3 (Engineer)             ║
║  + = Player 4 (Medic)                ║
║  ? = Unexplored                      ║
║  X = Locked/Blocked                  ║
║                                      ║
╚══════════════════════════════════════╝
EOF
}

# Initial render
exec > "$PIPE_MAP"
render_map

# Update loop
while true; do
    # In real implementation, read session state and re-render
    sleep 2
    render_map
done
