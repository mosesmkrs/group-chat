# Online Group Chat Application

A multithreaded, client-server command-line application written in C that functions as an online Group Chatting application. Users can create, join, leave, search for groups and send messages to all members of a group they belong to.

Built specifically for Windows using the **Winsock2** API for networking and Windows native threading (`CreateThread`) for handling concurrent user connections.

## Features

- **🔄 Concurrent Connections:** The server can handle multiple clients simultaneously without blocking, utilizing dedicated worker threads for each user
- **👥 Group Management:** Users can dynamically create new chat rooms or join existing ones
- **📡 Targeted Broadcasting:** Messages are routed exclusively to the members of the specific group, rather than a global server broadcast
- **🔒 Strict Membership Validation:** The server acts as a bouncer; users cannot view or send messages to a group unless they have explicitly joined it
- **⚡ Asynchronous Client UI:** The client uses a background listener thread to receive messages in real-time without interrupting the user's typing prompt

## Project Structure

```
group_chat_project/
├── bin/                     # Compiled executables (.exe) are saved here
├── include/                 
│   └── protocol.h           # Shared data structures (Packets, Client, Group)
├── src/                     
│   ├── client/              
│   │   └── client_main.c    # Client UI, connection, and listening threads
│   └── server/              
│       └── server_main.c    # Server switchboard, routing, and state management
├── Makefile                 # Automated build script for MinGW
└── README.md                # Project documentation
```

## Prerequisites

To compile and run this project, you must be on a Windows machine with the following installed:

- **GCC Compiler (MinGW):** Used to compile the C code
- **Make (mingw32-make):** Used to execute the build script

> **Note:** This project natively links the ws2_32 (Winsock) library. It will not compile on Linux/macOS without modifying the sockets and threading libraries.

## How to Build

1. Open your PowerShell or Command Prompt terminal
2. Navigate to the root directory of the project
3. Run the following command:

```powershell
mingw32-make
```

The build script will create a `bin/` directory and compile both `server.exe` and `client.exe` into it.

> **To clean the build directory:** Run `mingw32-make clean`

## How to Run

You must start the server before any clients can connect.

### 1. Start the Server

Open a terminal and run:

```powershell
.\bin\server.exe
```

The server will initialize and begin listening for connections on port 8080.

### 2. Start the Clients

Open a new terminal window for each user you want to connect and run:

```powershell
.\bin\client.exe
```

## User Guide

Upon starting the client executable, you will be prompted to enter a username. After connecting, you will see the Main Menu:

### The Main Menu

1. **Create a new group:** Type `1` and enter a unique name for a new chat room. You are automatically added to the group you create
2. **Join an existing group:** Type `2` and enter the name of an existing group to add yourself to its member list
3. **Enter a group chat room:** Type `3` and enter the name of a group you are a member of. The server will verify your membership and drop you into the live chat interface
4. **Leave a group:** Type `4` to remove yourself from a group's roster. You will no longer receive messages from this group
5. **Exit Application:** Safely disconnects from the server and closes the program

### Chat Mode

Once you enter a chat room (Option 3), your terminal switches to Chat Mode.

- Any text you type and send will be instantly broadcast to all other active members of that specific group
- If another user sends a message, it will pop up above your typing prompt automatically
- To exit the chat room and return to the Main Menu, type exactly: `/back`

## Error Handling

The server rigorously checks all actions. If you attempt to enter a chat room that does not exist, or if you try to send a message to a group you have not joined, the server will block the action and send an Error Packet. The client will display a large alert banner and safely return you to the main menu.
