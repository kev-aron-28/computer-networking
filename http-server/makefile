CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/server

SRCS = $(SRC_DIR)/server.c
HEADERS = $(SRC_DIR)/server.h
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(OBJS)

.PHONY: all clean
