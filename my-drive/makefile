# Directories
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
CLIENT_BIN := $(BIN_DIR)/client/client
SERVER_BIN := $(BIN_DIR)/server/server

# Source files
CLIENT_SRC := $(SRC_DIR)/client.c
SERVER_SRC := $(SRC_DIR)/server.c

# Header files
CLIENT_HEADERS := $(INC_DIR)/client.h
SERVER_HEADERS := $(INC_DIR)/server.h

# Compiler
CC := gcc
CFLAGS := -I$(INC_DIR)

# Targets
.PHONY: all clean

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC) $(CLIENT_HEADERS)
	@mkdir -p $(BIN_DIR)/client
	$(CC) $(CFLAGS) $< -o $@

$(SERVER_BIN): $(SERVER_SRC) $(SERVER_HEADERS)
	@mkdir -p $(BIN_DIR)/server
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)
