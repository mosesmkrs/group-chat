# Windows/MinGW Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
# LDFLAGS links the Windows Socket Library (Winsock2)
LDFLAGS = -lws2_32


# Default target
all: server client

# Compile the Server
server: 
	@if not exist bin mkdir bin
	$(CC) $(CFLAGS) src/server/server_main.c -o bin/server.exe $(LDFLAGS)
	@echo Server compiled into bin/server.exe

# Compile the Client
client: 
	@if not exist bin mkdir bin
	$(CC) $(CFLAGS) src/client/client_main.c -o bin/client.exe $(LDFLAGS)
	@echo Client compiled into bin/client.exe

# Clean up
clean:
	@if exist bin del /Q /S bin\*
	@echo Cleaned bin directory.