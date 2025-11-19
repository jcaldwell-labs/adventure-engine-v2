#!/bin/bash
#
# Log Panel Updater
# Shows team action log and chat
#

SESSION_ID="$1"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"
PIPE_LOG="$STATE_DIR/pipe_log"

if [ ! -p "$PIPE_LOG" ]; then
    echo "ERROR: Log pipe not found at $PIPE_LOG"
    exit 1
fi

# Function to render log
render_log() {
    local timestamp=$(date +%H:%M:%S)

    cat << EOF
╔══════════════════════════════════════╗
║         TEAM LOG                     ║
╠══════════════════════════════════════╣
║                                      ║
║ [00:00:01] Session started           ║
║ [00:00:02] alice joined as LEADER    ║
║ [00:00:05] bob joined as SCOUT       ║
║ [00:00:08] carol joined as ENGINEER  ║
║ [00:00:11] dave joined as MEDIC      ║
║ [00:00:15] Team ready - game begins  ║
║ [$timestamp] Waiting for first move  ║
║                                      ║
║ TEAM CHAT:                           ║
║ <alice> Which path should we take?   ║
║ <bob> I can scout ahead if needed    ║
║ <carol> I can unlock mechanisms      ║
║ <dave> I'll keep everyone healthy    ║
║                                      ║
╚══════════════════════════════════════╝
EOF
}

# Initial render
exec > "$PIPE_LOG"
render_log

# Update loop
while true; do
    sleep 3
    render_log
done
