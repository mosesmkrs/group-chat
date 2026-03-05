#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "protocol.h"

#define PORT 8080
#define MAX_GROUPS 20

Client active_clients[MAX_CLIENTS_PER_GROUP];
int client_count = 0;

Group active_groups[MAX_GROUPS];
int group_count = 0;

CRITICAL_SECTION server_mutex;

Client *get_client_by_socket(SOCKET sock)
{
    for (int i = 0; i < client_count; i++)
    {
        if (active_clients[i].socket_fd == sock && active_clients[i].is_active)
        {
            return &active_clients[i];
        }
    }
    return NULL;
}

Group *get_group_by_name(char *name)
{
    for (int i = 0; i < group_count; i++)
    {
        if (strcmp(active_groups[i].group_name, name) == 0)
        {
            return &active_groups[i];
        }
    }
    return NULL;
}

DWORD WINAPI handle_client(LPVOID arg)
{
    SOCKET client_socket = *(SOCKET *)arg;
    free(arg);

    Packet incoming_packet;
    int recv_size;

    while ((recv_size = recv(client_socket, (char *)&incoming_packet, sizeof(Packet), 0)) > 0)
    {
        EnterCriticalSection(&server_mutex);

        switch (incoming_packet.type)
        {

        case CMD_JOIN_SERVER:
        {
            printf("[SERVER] %s connected.\n", incoming_packet.sender);
            active_clients[client_count].socket_fd = client_socket;
            strcpy(active_clients[client_count].username, incoming_packet.sender);
            active_clients[client_count].is_active = 1;
            client_count++;
            break;
        }

        case CMD_CREATE_GROUP:
        {
            if (group_count < MAX_GROUPS && get_group_by_name(incoming_packet.group_target) == NULL)
            {
                strcpy(active_groups[group_count].group_name, incoming_packet.group_target);
                active_groups[group_count].member_count = 0;

                Client *creator = get_client_by_socket(client_socket);
                if (creator != NULL)
                {
                    active_groups[group_count].members[0] = creator;
                    active_groups[group_count].member_count++;
                }
                printf("[SERVER] Group '%s' created by %s.\n", incoming_packet.group_target, incoming_packet.sender);
                group_count++;
            }
            break;
        }

        case CMD_JOIN_GROUP:
        {
            Group *target_group = get_group_by_name(incoming_packet.group_target);
            Client *joiner = get_client_by_socket(client_socket);

            if (target_group != NULL && joiner != NULL && target_group->member_count < MAX_CLIENTS_PER_GROUP)
            {
                // Prevent joining twice
                int already_in = 0;
                for (int i = 0; i < target_group->member_count; i++)
                {
                    if (target_group->members[i]->socket_fd == client_socket)
                        already_in = 1;
                }
                if (!already_in)
                {
                    target_group->members[target_group->member_count] = joiner;
                    target_group->member_count++;
                    printf("[SERVER] %s joined group '%s'.\n", incoming_packet.sender, incoming_packet.group_target);
                }
            }
            else if (target_group == NULL)
            {
                Packet err_pkt;
                err_pkt.type = CMD_ERROR;
                strcpy(err_pkt.sender, "SERVER");
                strcpy(err_pkt.group_target, incoming_packet.group_target);
                strcpy(err_pkt.payload, "Error: Group does not exist!");
                send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
            }
            break;
        }

        case CMD_SEARCH_GROUPS:
        {
            Packet list_pkt;
            list_pkt.type = CMD_SUCCESS;
            strcpy(list_pkt.sender, "SERVER");
            strcpy(list_pkt.group_target, "System");

            if (group_count == 0)
            {
                strcpy(list_pkt.payload, "There are currently no active groups.");
            }
            else
            {
                // Build a string list of all groups
                strcpy(list_pkt.payload, "\n--- Active Groups ---\n");
                for (int i = 0; i < group_count; i++)
                {
                    char temp[MAX_GROUP_NAME_LEN + 20];
                    // Format: "- GroupName (X members)"
                    snprintf(temp, sizeof(temp), "- %s (%d members)\n",
                             active_groups[i].group_name, active_groups[i].member_count);

                    // Prevent buffer overflow if there are tons of groups
                    if (strlen(list_pkt.payload) + strlen(temp) < MAX_MESSAGE_LEN)
                    {
                        strcat(list_pkt.payload, temp);
                    }
                }
            }

            // Send the formatted list back to the user
            send(client_socket, (char *)&list_pkt, sizeof(Packet), 0);
            break;
        }

        case CMD_LEAVE_GROUP:
        {
            Group *target_group = get_group_by_name(incoming_packet.group_target);

            if (target_group != NULL)
            {
                int found_index = -1;

                // 1. Find the user in the member array
                for (int i = 0; i < target_group->member_count; i++)
                {
                    if (target_group->members[i]->socket_fd == client_socket)
                    {
                        found_index = i;
                        break;
                    }
                }

                // 2. If found, remove them and shift the array left
                if (found_index != -1)
                {
                    for (int i = found_index; i < target_group->member_count - 1; i++)
                    {
                        target_group->members[i] = target_group->members[i + 1];
                    }
                    target_group->member_count--; // Reduce the total count

                    printf("[SERVER] %s left group '%s'.\n", incoming_packet.sender, incoming_packet.group_target);

                    // Send success message
                    Packet succ_pkt;
                    succ_pkt.type = CMD_SUCCESS;
                    strcpy(succ_pkt.sender, "SERVER");
                    strcpy(succ_pkt.group_target, incoming_packet.group_target);
                    strcpy(succ_pkt.payload, "You have successfully left the group.");
                    send(client_socket, (char *)&succ_pkt, sizeof(Packet), 0);
                }
                // 3. User isn't in the group
                else
                {
                    Packet err_pkt;
                    err_pkt.type = CMD_ERROR;
                    strcpy(err_pkt.sender, "SERVER");
                    strcpy(err_pkt.group_target, incoming_packet.group_target);
                    strcpy(err_pkt.payload, "Error: You are not a member of this group.");
                    send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
                }
            }
            // 4. Group does not exist
            else
            {
                Packet err_pkt;
                err_pkt.type = CMD_ERROR;
                strcpy(err_pkt.sender, "SERVER");
                strcpy(err_pkt.group_target, incoming_packet.group_target);
                strcpy(err_pkt.payload, "Error: Group does not exist!");
                send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
            }
            break;
        }

        case CMD_CHECK_MEMBERSHIP:
        {
            Group *target_group = get_group_by_name(incoming_packet.group_target);

            if (target_group != NULL)
            {
                // Check if they are on the list
                int is_member = 0;
                for (int i = 0; i < target_group->member_count; i++)
                {
                    if (target_group->members[i]->socket_fd == client_socket)
                    {
                        is_member = 1;
                        break;
                    }
                }

                if (is_member)
                {
                    Packet succ_pkt;
                    succ_pkt.type = CMD_SUCCESS;
                    strcpy(succ_pkt.sender, "SERVER");
                    strcpy(succ_pkt.group_target, incoming_packet.group_target);
                    strcpy(succ_pkt.payload, "Access Granted");
                    send(client_socket, (char *)&succ_pkt, sizeof(Packet), 0);
                }
                else
                {
                    Packet err_pkt;
                    err_pkt.type = CMD_ERROR;
                    strcpy(err_pkt.sender, "SERVER");
                    strcpy(err_pkt.group_target, incoming_packet.group_target);
                    strcpy(err_pkt.payload, "Error: You are not a member of the group. Join the group first.");
                    send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
                }
            }
            else
            {
                Packet err_pkt;
                err_pkt.type = CMD_ERROR;
                strcpy(err_pkt.sender, "SERVER");
                strcpy(err_pkt.group_target, incoming_packet.group_target);
                strcpy(err_pkt.payload, "Error: Group does not exist!");
                send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
            }
            break;
        }

        case CMD_SEND_MSG:
        {
            Group *target_group = get_group_by_name(incoming_packet.group_target);

            if (target_group != NULL)
            {
                // 1. STRICT MEMBERSHIP CHECK
                int is_member = 0;
                for (int i = 0; i < target_group->member_count; i++)
                {
                    if (target_group->members[i]->socket_fd == client_socket)
                    {
                        is_member = 1;
                        break;
                    }
                }

                // 2. Broadcast if allowed
                if (is_member)
                {
                    for (int i = 0; i < target_group->member_count; i++)
                    {
                        SOCKET member_sock = target_group->members[i]->socket_fd;
                        if (member_sock != client_socket)
                        {
                            send(member_sock, (char *)&incoming_packet, sizeof(Packet), 0);
                        }
                    }
                    printf("[ROUTED] Message from %s to group '%s'.\n", incoming_packet.sender, incoming_packet.group_target);
                }
                // 3. Block and send error if not allowed
                else
                {
                    Packet err_pkt;
                    err_pkt.type = CMD_ERROR;
                    strcpy(err_pkt.sender, "SERVER");
                    strcpy(err_pkt.group_target, incoming_packet.group_target);
                    strcpy(err_pkt.payload, "Error: You are not a member of the group. Join the group first.");
                    send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
                }
            }
            else
            {
                Packet err_pkt;
                err_pkt.type = CMD_ERROR;
                strcpy(err_pkt.sender, "SERVER");
                strcpy(err_pkt.group_target, incoming_packet.group_target);
                strcpy(err_pkt.payload, "Error: Group does not exist!");
                send(client_socket, (char *)&err_pkt, sizeof(Packet), 0);
            }
            break;
        }
        default:
            break;
        }

        LeaveCriticalSection(&server_mutex);
    }

    printf("[SERVER] A client disconnected.\n");
    EnterCriticalSection(&server_mutex);
    Client *disconnected_client = get_client_by_socket(client_socket);
    if (disconnected_client != NULL)
        disconnected_client->is_active = 0;
    LeaveCriticalSection(&server_mutex);

    closesocket(client_socket);
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    InitializeCriticalSection(&server_mutex);
    WSAStartup(MAKEWORD(2, 2), &wsa);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Server switchboard active and listening on port %d...\n", PORT);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET)
            continue;

        SOCKET *new_sock = malloc(sizeof(SOCKET));
        *new_sock = client_socket;

        HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)new_sock, 0, NULL);
        if (thread)
            CloseHandle(thread);
    }

    DeleteCriticalSection(&server_mutex);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}