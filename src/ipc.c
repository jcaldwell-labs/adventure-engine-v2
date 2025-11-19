#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

// Initialize IPC system
bool ipc_init(void) {
    struct stat st = {0};
    if (stat(IPC_SOCKET_DIR, &st) == -1) {
        if (mkdir(IPC_SOCKET_DIR, 0700) != 0) {
            perror("Failed to create IPC socket directory");
            return false;
        }
    }
    return true;
}

// Clean up old sockets
void ipc_cleanup(void) {
    // Remove all .sock files in IPC_SOCKET_DIR
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -f %s/*.sock 2>/dev/null", IPC_SOCKET_DIR);
    system(cmd);
}

// Generate socket path for session/player
void ipc_generate_socket_path(const char* session_id, const char* player_id,
                              char* out_path, int max_len) {
    if (!session_id || !player_id || !out_path) return;

    snprintf(out_path, max_len, "%s/%s-%s.sock",
             IPC_SOCKET_DIR, session_id, player_id);
}

// Check if socket exists
bool ipc_socket_exists(const char* session_id, const char* player_id) {
    char path[256];
    ipc_generate_socket_path(session_id, player_id, path, sizeof(path));
    return access(path, F_OK) == 0;
}

// Convert message type to string
const char* message_type_to_string(MessageType type) {
    switch (type) {
        case MSG_CMD:       return "CMD";
        case MSG_STATE:     return "STATE";
        case MSG_EVENT:     return "EVENT";
        case MSG_SYNC:      return "SYNC";
        case MSG_SIGNAL:    return "SIGNAL";
        case MSG_CHAT:      return "CHAT";
        case MSG_HEARTBEAT: return "HEARTBEAT";
        case MSG_ERROR:     return "ERROR";
        case MSG_ACK:       return "ACK";
        case MSG_BROADCAST: return "BROADCAST";
        default:            return "UNKNOWN";
    }
}

// ============================================================================
// MESSAGE OPERATIONS
// ============================================================================

// Create a new message
Message* ipc_message_create(MessageType type, const char* session_id,
                           const char* player_id, const char* payload) {
    Message* msg = (Message*)calloc(1, sizeof(Message));
    if (!msg) {
        perror("Failed to allocate message");
        return NULL;
    }

    msg->type = type;
    msg->priority = PRIORITY_NORMAL;

    if (session_id) {
        strncpy(msg->session_id, session_id, 63);
    }

    if (player_id) {
        strncpy(msg->player_id, player_id, 63);
    }

    msg->sequence = 0;
    msg->timestamp = time(NULL);
    msg->requires_ack = false;
    msg->ack_sequence = 0;

    if (payload) {
        ipc_message_set_payload(msg, payload);
    }

    return msg;
}

// Set message payload
bool ipc_message_set_payload(Message* msg, const char* payload) {
    if (!msg || !payload) {
        return false;
    }

    size_t len = strlen(payload);
    if (len >= MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "Payload too large: %zu bytes (max %d)\n",
                len, MAX_PAYLOAD_SIZE);
        return false;
    }

    strncpy(msg->payload, payload, MAX_PAYLOAD_SIZE - 1);
    msg->payload[MAX_PAYLOAD_SIZE - 1] = '\0';
    msg->payload_size = strlen(msg->payload);

    return true;
}

// Get message payload
bool ipc_message_get_payload(const Message* msg, char* out_buffer, int max_len) {
    if (!msg || !out_buffer) {
        return false;
    }

    strncpy(out_buffer, msg->payload, max_len - 1);
    out_buffer[max_len - 1] = '\0';

    return true;
}

// Destroy message
void ipc_message_destroy(Message* msg) {
    if (msg) {
        free(msg);
    }
}

// ============================================================================
// MESSAGE QUEUE
// ============================================================================

// Create message queue
MessageQueue* message_queue_create(int max_size) {
    MessageQueue* queue = (MessageQueue*)calloc(1, sizeof(MessageQueue));
    if (!queue) {
        perror("Failed to allocate message queue");
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->max_size = (max_size > 0) ? max_size : 100;

    return queue;
}

// Destroy message queue
void message_queue_destroy(MessageQueue* queue) {
    if (!queue) return;

    message_queue_clear(queue);
    free(queue);
}

// Push message to queue
bool message_queue_push(MessageQueue* queue, const Message* message) {
    if (!queue || !message) {
        return false;
    }

    if (message_queue_is_full(queue)) {
        fprintf(stderr, "Message queue is full\n");
        return false;
    }

    MessageNode* node = (MessageNode*)malloc(sizeof(MessageNode));
    if (!node) {
        perror("Failed to allocate message node");
        return false;
    }

    memcpy(&node->message, message, sizeof(Message));
    node->next = NULL;

    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }

    queue->tail = node;
    queue->count++;

    return true;
}

// Pop message from queue
bool message_queue_pop(MessageQueue* queue, Message* out_message) {
    if (!queue || !out_message) {
        return false;
    }

    if (message_queue_is_empty(queue)) {
        return false;
    }

    MessageNode* node = queue->head;
    memcpy(out_message, &node->message, sizeof(Message));

    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }

    free(node);
    queue->count--;

    return true;
}

// Peek at message without removing
bool message_queue_peek(MessageQueue* queue, Message* out_message) {
    if (!queue || !out_message || message_queue_is_empty(queue)) {
        return false;
    }

    memcpy(out_message, &queue->head->message, sizeof(Message));
    return true;
}

// Get queue size
int message_queue_size(const MessageQueue* queue) {
    return queue ? queue->count : 0;
}

// Check if queue is empty
bool message_queue_is_empty(const MessageQueue* queue) {
    return queue ? (queue->count == 0) : true;
}

// Check if queue is full
bool message_queue_is_full(const MessageQueue* queue) {
    return queue ? (queue->count >= queue->max_size) : true;
}

// Clear queue
void message_queue_clear(MessageQueue* queue) {
    if (!queue) return;

    Message msg;
    while (message_queue_pop(queue, &msg)) {
        // Just drain the queue
    }
}

// ============================================================================
// IPC CHANNEL
// ============================================================================

// Create IPC channel
IPCChannel* ipc_channel_create(const char* session_id, const char* player_id) {
    if (!session_id || !player_id) {
        return NULL;
    }

    IPCChannel* channel = (IPCChannel*)calloc(1, sizeof(IPCChannel));
    if (!channel) {
        perror("Failed to allocate IPC channel");
        return NULL;
    }

    strncpy(channel->session_id, session_id, 63);
    strncpy(channel->player_id, player_id, 63);

    ipc_generate_socket_path(session_id, player_id,
                            channel->socket_path, sizeof(channel->socket_path));

    channel->socket_fd = -1;
    channel->is_open = false;

    channel->send_queue = message_queue_create(100);
    channel->recv_queue = message_queue_create(100);

    channel->next_sequence = 1;
    channel->last_send = 0;
    channel->last_recv = 0;

    channel->bytes_sent = 0;
    channel->bytes_received = 0;
    channel->messages_sent = 0;
    channel->messages_received = 0;
    channel->errors = 0;

    return channel;
}

// Open IPC channel (create Unix domain socket)
bool ipc_channel_open(IPCChannel* channel) {
    if (!channel) {
        return false;
    }

    if (channel->is_open) {
        return true;  // Already open
    }

    // Create socket
    channel->socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (channel->socket_fd < 0) {
        perror("Failed to create socket");
        return false;
    }

    // Remove old socket file if it exists
    unlink(channel->socket_path);

    // Bind socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, channel->socket_path, sizeof(addr.sun_path) - 1);

    if (bind(channel->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(channel->socket_fd);
        channel->socket_fd = -1;
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(channel->socket_fd, F_GETFL, 0);
    fcntl(channel->socket_fd, F_SETFL, flags | O_NONBLOCK);

    channel->is_open = true;
    return true;
}

// Close IPC channel
bool ipc_channel_close(IPCChannel* channel) {
    if (!channel) {
        return false;
    }

    if (channel->socket_fd >= 0) {
        close(channel->socket_fd);
        channel->socket_fd = -1;
    }

    unlink(channel->socket_path);
    channel->is_open = false;

    return true;
}

// Destroy IPC channel
void ipc_channel_destroy(IPCChannel* channel) {
    if (!channel) {
        return;
    }

    ipc_channel_close(channel);

    if (channel->send_queue) {
        message_queue_destroy(channel->send_queue);
    }

    if (channel->recv_queue) {
        message_queue_destroy(channel->recv_queue);
    }

    free(channel);
}

// Send message through channel
bool ipc_send(IPCChannel* channel, const Message* message) {
    if (!channel || !message) {
        return false;
    }

    if (!channel->is_open) {
        fprintf(stderr, "Channel is not open\n");
        return false;
    }

    // Assign sequence number
    Message msg_copy;
    memcpy(&msg_copy, message, sizeof(Message));
    msg_copy.sequence = channel->next_sequence++;

    // For now, just queue it (actual socket send would go here)
    if (!message_queue_push(channel->send_queue, &msg_copy)) {
        channel->errors++;
        return false;
    }

    channel->messages_sent++;
    channel->last_send = time(NULL);

    return true;
}

// Receive message from channel
bool ipc_receive(IPCChannel* channel, Message* out_message) {
    if (!channel || !out_message) {
        return false;
    }

    if (!channel->is_open) {
        return false;
    }

    // Pop from receive queue
    if (message_queue_pop(channel->recv_queue, out_message)) {
        channel->messages_received++;
        channel->last_recv = time(NULL);
        return true;
    }

    return false;
}

// Send string message (convenience function)
bool ipc_send_string(IPCChannel* channel, MessageType type, const char* payload) {
    if (!channel || !payload) {
        return false;
    }

    Message* msg = ipc_message_create(type, channel->session_id,
                                     channel->player_id, payload);
    if (!msg) {
        return false;
    }

    bool result = ipc_send(channel, msg);
    ipc_message_destroy(msg);

    return result;
}

// Receive string message (convenience function)
bool ipc_receive_string(IPCChannel* channel, char* out_buffer, int max_len) {
    if (!channel || !out_buffer) {
        return false;
    }

    Message msg;
    if (ipc_receive(channel, &msg)) {
        return ipc_message_get_payload(&msg, out_buffer, max_len);
    }

    return false;
}

// Broadcast message to all players in session (stub for now)
bool ipc_broadcast(const char* session_id, const Message* message) {
    // TODO: Implement actual broadcast by sending to all player sockets
    // For now, this is a placeholder
    (void)session_id;
    (void)message;
    return true;
}

// Broadcast string message
bool ipc_broadcast_string(const char* session_id, MessageType type, const char* payload) {
    Message* msg = ipc_message_create(type, session_id, "broadcast", payload);
    if (!msg) {
        return false;
    }

    bool result = ipc_broadcast(session_id, msg);
    ipc_message_destroy(msg);

    return result;
}

// ============================================================================
// PAYLOAD PARSING HELPERS
// ============================================================================

// Parse command payload
bool ipc_parse_command(const char* payload, CommandPayload* out_cmd) {
    if (!payload || !out_cmd) {
        return false;
    }

    memset(out_cmd, 0, sizeof(CommandPayload));

    // Format: "VERB NOUN TARGET EXTRA"
    sscanf(payload, "%63s %127s %63s %255[^\n]",
           out_cmd->verb, out_cmd->noun, out_cmd->target, out_cmd->extra);

    return (out_cmd->verb[0] != '\0');
}

// Format command payload
bool ipc_format_command(const CommandPayload* cmd, char* out_buffer, int max_len) {
    if (!cmd || !out_buffer) {
        return false;
    }

    snprintf(out_buffer, max_len, "%s %s %s %s",
             cmd->verb, cmd->noun, cmd->target, cmd->extra);

    return true;
}

// Parse state update payload
bool ipc_parse_state(const char* payload, StateUpdate* out_state) {
    if (!payload || !out_state) {
        return false;
    }

    memset(out_state, 0, sizeof(StateUpdate));

    // Format: "KEY=VALUE REALM"
    char temp[512];
    strncpy(temp, payload, sizeof(temp) - 1);

    char* equals = strchr(temp, '=');
    if (equals) {
        *equals = '\0';
        strncpy(out_state->key, temp, sizeof(out_state->key) - 1);

        char* space = strchr(equals + 1, ' ');
        if (space) {
            *space = '\0';
            strncpy(out_state->value, equals + 1, sizeof(out_state->value) - 1);
            strncpy(out_state->realm, space + 1, sizeof(out_state->realm) - 1);
        } else {
            strncpy(out_state->value, equals + 1, sizeof(out_state->value) - 1);
        }
    }

    return (out_state->key[0] != '\0');
}

// Format state update payload
bool ipc_format_state(const StateUpdate* state, char* out_buffer, int max_len) {
    if (!state || !out_buffer) {
        return false;
    }

    if (state->realm[0]) {
        snprintf(out_buffer, max_len, "%s=%s %s",
                 state->key, state->value, state->realm);
    } else {
        snprintf(out_buffer, max_len, "%s=%s",
                 state->key, state->value);
    }

    return true;
}

// Parse event payload
bool ipc_parse_event(const char* payload, EventPayload* out_event) {
    if (!payload || !out_event) {
        return false;
    }

    memset(out_event, 0, sizeof(EventPayload));

    // Format: "TYPE ACTOR TARGET DATA"
    sscanf(payload, "%63s %63s %127s %255[^\n]",
           out_event->event_type, out_event->actor,
           out_event->target, out_event->data);

    return (out_event->event_type[0] != '\0');
}

// Format event payload
bool ipc_format_event(const EventPayload* event, char* out_buffer, int max_len) {
    if (!event || !out_buffer) {
        return false;
    }

    snprintf(out_buffer, max_len, "%s %s %s %s",
             event->event_type, event->actor, event->target, event->data);

    return true;
}
