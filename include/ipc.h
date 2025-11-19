#ifndef IPC_H
#define IPC_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define IPC_SOCKET_DIR "/tmp/adventure-engine"
#define MAX_MESSAGE_SIZE 4096
#define MAX_PAYLOAD_SIZE 3584
#define MAX_ID_LENGTH 64
#define MAX_SOCKET_PATH 256

// Message types
typedef enum {
    MSG_CMD,        // Player command
    MSG_STATE,      // State update
    MSG_EVENT,      // Game event
    MSG_SYNC,       // Full state synchronization
    MSG_SIGNAL,     // Control signal
    MSG_CHAT,       // Team chat message
    MSG_HEARTBEAT,  // Keep-alive
    MSG_ERROR,      // Error message
    MSG_ACK,        // Acknowledgment
    MSG_BROADCAST   // Broadcast to all players
} MessageType;

// Message priority
typedef enum {
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
} MessagePriority;

// Message structure
typedef struct {
    MessageType type;
    MessagePriority priority;

    char session_id[MAX_ID_LENGTH];
    char player_id[MAX_ID_LENGTH];
    uint32_t sequence;      // For ordering
    time_t timestamp;

    uint16_t payload_size;
    char payload[MAX_PAYLOAD_SIZE];

    // For acknowledgment
    bool requires_ack;
    uint32_t ack_sequence;

} Message;

// Message queue for buffering
typedef struct MessageNode {
    Message message;
    struct MessageNode* next;
} MessageNode;

typedef struct {
    MessageNode* head;
    MessageNode* tail;
    int count;
    int max_size;
} MessageQueue;

// IPC channel (per player connection)
typedef struct {
    char socket_path[MAX_SOCKET_PATH];
    int socket_fd;
    bool is_open;

    char session_id[MAX_ID_LENGTH];
    char player_id[MAX_ID_LENGTH];

    MessageQueue* send_queue;
    MessageQueue* recv_queue;

    uint32_t next_sequence;
    time_t last_send;
    time_t last_recv;

    // Statistics
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors;

} IPCChannel;

// Function declarations

// Channel management
IPCChannel* ipc_channel_create(const char* session_id, const char* player_id);
bool ipc_channel_open(IPCChannel* channel);
bool ipc_channel_close(IPCChannel* channel);
void ipc_channel_destroy(IPCChannel* channel);

// Message operations
Message* ipc_message_create(MessageType type, const char* session_id,
                           const char* player_id, const char* payload);
bool ipc_message_set_payload(Message* msg, const char* payload);
bool ipc_message_get_payload(const Message* msg, char* out_buffer, int max_len);
void ipc_message_destroy(Message* msg);

// Send/Receive
bool ipc_send(IPCChannel* channel, const Message* message);
bool ipc_receive(IPCChannel* channel, Message* out_message);
bool ipc_send_string(IPCChannel* channel, MessageType type, const char* payload);
bool ipc_receive_string(IPCChannel* channel, char* out_buffer, int max_len);

// Broadcast (send to all players in session)
bool ipc_broadcast(const char* session_id, const Message* message);
bool ipc_broadcast_string(const char* session_id, MessageType type, const char* payload);

// Message queue operations
MessageQueue* message_queue_create(int max_size);
void message_queue_destroy(MessageQueue* queue);
bool message_queue_push(MessageQueue* queue, const Message* message);
bool message_queue_pop(MessageQueue* queue, Message* out_message);
bool message_queue_peek(MessageQueue* queue, Message* out_message);
int message_queue_size(const MessageQueue* queue);
bool message_queue_is_empty(const MessageQueue* queue);
bool message_queue_is_full(const MessageQueue* queue);
void message_queue_clear(MessageQueue* queue);

// Utility
const char* message_type_to_string(MessageType type);
bool ipc_init(void);  // Create socket directory
void ipc_cleanup(void);  // Clean up old sockets
bool ipc_socket_exists(const char* session_id, const char* player_id);
void ipc_generate_socket_path(const char* session_id, const char* player_id,
                              char* out_path, int max_len);

// Command parsing helpers (for MSG_CMD payloads)
typedef struct {
    char verb[64];
    char noun[128];
    char target[64];
    char extra[256];
} CommandPayload;

bool ipc_parse_command(const char* payload, CommandPayload* out_cmd);
bool ipc_format_command(const CommandPayload* cmd, char* out_buffer, int max_len);

// State update helpers (for MSG_STATE payloads)
typedef struct {
    char key[128];
    char value[256];
    char realm[64];
} StateUpdate;

bool ipc_parse_state(const char* payload, StateUpdate* out_state);
bool ipc_format_state(const StateUpdate* state, char* out_buffer, int max_len);

// Event helpers (for MSG_EVENT payloads)
typedef struct {
    char event_type[64];
    char actor[64];
    char target[128];
    char data[256];
} EventPayload;

bool ipc_parse_event(const char* payload, EventPayload* out_event);
bool ipc_format_event(const EventPayload* event, char* out_buffer, int max_len);

#endif // IPC_H
