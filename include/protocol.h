#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_USERNAME_LEN 32
#define MAX_GROUP_NAME_LEN 32
#define MAX_MESSAGE_LEN 512
#define MAX_CLIENTS_PER_GROUP 50

// Command types so the server knows what the client wants to do
typedef enum
{
    CMD_JOIN_SERVER,
    CMD_CREATE_GROUP,
    CMD_JOIN_GROUP,
    CMD_LEAVE_GROUP,
    CMD_SEND_MSG,
    CMD_REPLY_MSG,
    CMD_ERROR,
    CMD_CHECK_MEMBERSHIP,
    CMD_SUCCESS,
    CMD_SEARCH_GROUPS // NEW: Request a list of active groups
} CommandType;

// The standard Packet that gets sent over the network via send() and recv()
typedef struct
{
    CommandType type;                      // What kind of action is this?
    char sender[MAX_USERNAME_LEN];         // Who sent it?
    char group_target[MAX_GROUP_NAME_LEN]; // Which group is this for?
    char payload[MAX_MESSAGE_LEN];         // The actual text message or reply ID
} Packet;

// --- SERVER INTERNAL STATE (Only used by the server) ---

// Represents a single connected user
typedef struct
{
    int socket_fd;                   // The client's unique socket descriptor
    char username[MAX_USERNAME_LEN]; // The client's chosen name
    int is_active;                   // 1 if connected, 0 if disconnected
} Client;

// Represents a chat group and its members
typedef struct
{
    char group_name[MAX_GROUP_NAME_LEN];
    Client *members[MAX_CLIENTS_PER_GROUP]; // Array of pointers to connected clients
    int member_count;                       // How many people are currently in it
} Group;

#endif