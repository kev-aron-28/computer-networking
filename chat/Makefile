# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
CLIENT_SRC = $(SRC_DIR)/client.c
PROTOCOL_SRC = $(SRC_DIR)/protocol.c
SERVER_SRC = $(SRC_DIR)/server.c

# Header files
CLIENT_HEADER = $(INCLUDE_DIR)/client.h
PROTOCOL_HEADER = $(INCLUDE_DIR)/protocol.h

# Object files
CLIENT_OBJ = $(BUILD_DIR)/client.o
PROTOCOL_OBJ = $(BUILD_DIR)/protocol.o
SERVER_OBJ = $(BUILD_DIR)/server.o

# Executables
CLIENT_EXEC = $(BUILD_DIR)/client
SERVER_EXEC = $(BUILD_DIR)/server

# Compiler and flags
CC = gcc
CFLAGS = -I$(INCLUDE_DIR) -Wall -g
LDFLAGS =  -lncurses -lrt -lpthread # Link ncurses library

# Targets
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Build client executable
$(CLIENT_EXEC): $(CLIENT_OBJ) $(PROTOCOL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build server executable
$(SERVER_EXEC): $(SERVER_OBJ) $(PROTOCOL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile client.c
$(CLIENT_OBJ): $(CLIENT_SRC) $(CLIENT_HEADER) $(PROTOCOL_HEADER)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile server.c
$(SERVER_OBJ): $(SERVER_SRC) $(PROTOCOL_HEADER)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile protocol.c
$(PROTOCOL_OBJ): $(PROTOCOL_SRC) $(PROTOCOL_HEADER)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR)/*
