# Directories
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
CLIENT_BIN := $(BIN_DIR)/client/client
SERVER_BIN := $(BIN_DIR)/server/server

# Source files
CLIENT_SRC := $(SRC_DIR)/client.c
SERVER_SRC := $(SRC_DIR)/server.c
PROTOCOL_SRC := $(SRC_DIR)/protocol.c

# Header files
CLIENT_HEADERS := $(INC_DIR)/client.h $(INC_DIR)/protocol.h
SERVER_HEADERS := $(INC_DIR)/server.h $(INC_DIR)/protocol.h

# Object files
PROTOCOL_OBJ := $(PROTOCOL_SRC:.c=.o)

# Compiler
CC := gcc
CFLAGS := -I$(INC_DIR)

# Targets
.PHONY: all clean

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC) $(CLIENT_HEADERS) $(PROTOCOL_OBJ)
	@mkdir -p $(BIN_DIR)/client
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(PROTOCOL_OBJ) -o $@

$(SERVER_BIN): $(SERVER_SRC) $(SERVER_HEADERS) $(PROTOCOL_OBJ)
	@mkdir -p $(BIN_DIR)/server
	$(CC) $(CFLAGS) $(SERVER_SRC) $(PROTOCOL_OBJ) -o $@

$(PROTOCOL_OBJ): $(PROTOCOL_SRC) $(INC_DIR)/protocol.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(PROTOCOL_OBJ)
	