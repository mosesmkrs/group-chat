#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "protocol.h"

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int in_chat_mode = 0;
char current_group[MAX_GROUP_NAME_LEN] = "";

DWORD WINAPI receive_messages(LPVOID arg)
{
    SOCKET sock = *(SOCKET *)arg;
    Packet incoming;
    int recv_size;

    while ((recv_size = recv(sock, (char *)&incoming, sizeof(Packet), 0)) > 0)
    {

        // --- ERROR HANDLING UX ---
        if (incoming.type == CMD_ERROR)
        {
            printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("[SERVER ALERT]: %s\n", incoming.payload);
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

            if (in_chat_mode)
            {
                in_chat_mode = 0; // Kick out of chat mode
                printf("--- Kicked back to Menu ---\n");
                printf("Press ENTER to acknowledge and continue...\n");
            }
            fflush(stdout);
            continue;
        }

        // --- NORMAL CHAT HANDLING ---
        if (in_chat_mode && strcmp(incoming.group_target, current_group) == 0)
        {
            printf("\r[%s]: %s\n> ", incoming.sender, incoming.payload);
            fflush(stdout);
        }
        // --- SUCCESS HANDLING UX ---
        if (incoming.type == CMD_SUCCESS)
        {
            // Only enter chat mode if this was an access request
            if (strcmp(incoming.payload, "Access Granted") == 0)
            {
                in_chat_mode = 1;
                strcpy(current_group, incoming.group_target);
                printf("\n--- Entered Chat: %s (Type /back to return to menu) ---\n> ", current_group);
            }
            else
            {
                // Otherwise, just print the success notification (like leaving a group)
                printf("\n\n[SUCCESS]: %s\n> ", incoming.payload);
            }
            fflush(stdout);
            continue;
        }
        else if (!in_chat_mode)
        {
            printf("\r[Notification: New message in '%s' from %s]\n", incoming.group_target, incoming.sender);
            printf("Select option (1-4): ");
            fflush(stdout);
        }
    }

    printf("\nDisconnected from server.\n");
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char username[MAX_USERNAME_LEN];

    WSAStartup(MAKEWORD(2, 2), &wsa);
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        puts("Connect error");
        return 1;
    }

    printf("Enter your username: ");
    fgets(username, MAX_USERNAME_LEN, stdin);
    username[strcspn(username, "\n")] = 0;

    Packet reg_packet;
    reg_packet.type = CMD_JOIN_SERVER;
    strcpy(reg_packet.sender, username);
    strcpy(reg_packet.group_target, "Server");
    send(client_socket, (char *)&reg_packet, sizeof(Packet), 0);

    HANDLE listener = CreateThread(NULL, 0, receive_messages, &client_socket, 0, NULL);
    if (listener)
        CloseHandle(listener);

    char choice_str[10];
    Packet my_packet;
    strcpy(my_packet.sender, username);

    while (1)
    {
        if (!in_chat_mode)
        {
            printf("\n=================================\n");
            printf("      ONLINE GROUP CHAT          \n");
            printf("=================================\n");
            printf("1. Create a new group\n");
            printf("2. Join an existing group\n");
            printf("3. Enter a group chat room\n");
            printf("4. Leave a group\n");    // NEW OPTION
            printf("5. Exit Application\n"); // MOVED TO 5
            printf("=================================\n");
            printf("Select option (1-5): ");

            fgets(choice_str, 10, stdin);
            int choice = atoi(choice_str);

            if (choice == 1)
            {
                my_packet.type = CMD_CREATE_GROUP;
                printf("Enter new group name: ");
                fgets(my_packet.group_target, MAX_GROUP_NAME_LEN, stdin);
                my_packet.group_target[strcspn(my_packet.group_target, "\n")] = 0;
                send(client_socket, (char *)&my_packet, sizeof(Packet), 0);
                Sleep(500);
            }
            else if (choice == 2)
            {
                my_packet.type = CMD_JOIN_GROUP;
                printf("Enter name of group to join: ");
                fgets(my_packet.group_target, MAX_GROUP_NAME_LEN, stdin);
                my_packet.group_target[strcspn(my_packet.group_target, "\n")] = 0;
                send(client_socket, (char *)&my_packet, sizeof(Packet), 0);
                Sleep(500);
            }
            else if (choice == 3)
            {
                printf("Enter the name of the group you want to chat in: ");
                char attempt_group[MAX_GROUP_NAME_LEN];
                fgets(attempt_group, MAX_GROUP_NAME_LEN, stdin);
                attempt_group[strcspn(attempt_group, "\n")] = 0;

                my_packet.type = CMD_CHECK_MEMBERSHIP;
                strcpy(my_packet.group_target, attempt_group);
                send(client_socket, (char *)&my_packet, sizeof(Packet), 0);
                Sleep(500);
            }
            else if (choice == 4)
            { // NEW LOGIC FOR LEAVING
                my_packet.type = CMD_LEAVE_GROUP;
                printf("Enter name of group to leave: ");
                fgets(my_packet.group_target, MAX_GROUP_NAME_LEN, stdin);
                my_packet.group_target[strcspn(my_packet.group_target, "\n")] = 0;
                send(client_socket, (char *)&my_packet, sizeof(Packet), 0);
                Sleep(500);
            }
            else if (choice == 5)
            {
                break;
            }
        }
        else
        {
            // Chat Mode Loop
            my_packet.type = CMD_SEND_MSG;
            strcpy(my_packet.group_target, current_group);

            printf("> ");
            fgets(my_packet.payload, MAX_MESSAGE_LEN, stdin);
            my_packet.payload[strcspn(my_packet.payload, "\n")] = 0;

            if (strcmp(my_packet.payload, "/back") == 0)
            {
                in_chat_mode = 0;
            }
            else if (strlen(my_packet.payload) > 0)
            {
                send(client_socket, (char *)&my_packet, sizeof(Packet), 0);
            }
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}