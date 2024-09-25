CC = gcc
CFLAGS = -Iinclude -Wall -g  # Incluye cabeceras desde la carpeta include y habilita warnings
BUILD_DIR = build

# Archivos compartidos (lógica del juego)
GAME_SRC = src/game/game.c
GAME_OBJ = $(BUILD_DIR)/game/game.o

# Cliente
CLIENT_SRC = src/client/client.c
CLIENT_OBJ = $(BUILD_DIR)/client/client.o
CLIENT_EXE = $(BUILD_DIR)/client/client

# Servidor
SERVER_SRC = src/server/server.c
SERVER_OBJ = $(BUILD_DIR)/server/server.o
SERVER_EXE = $(BUILD_DIR)/server/server

# Compila el proyecto completo
all: $(CLIENT_EXE) $(SERVER_EXE)

# Crear los directorios necesarios en build si no existen
$(BUILD_DIR)/client $(BUILD_DIR)/server $(BUILD_DIR)/game:
	mkdir -p $(BUILD_DIR)/client $(BUILD_DIR)/server $(BUILD_DIR)/game

# Ejecutable del cliente
$(CLIENT_EXE): $(CLIENT_OBJ) $(GAME_OBJ) | $(BUILD_DIR)/client
	$(CC) $(CFLAGS) -o $(CLIENT_EXE) $(CLIENT_OBJ) $(GAME_OBJ)

# Ejecutable del servidor
$(SERVER_EXE): $(SERVER_OBJ) $(GAME_OBJ) | $(BUILD_DIR)/server
	$(CC) $(CFLAGS) -o $(SERVER_EXE) $(SERVER_OBJ) $(GAME_OBJ)

# Reglas para generar los archivos .o desde los .c
$(BUILD_DIR)/client/client.o: src/client/client.c | $(BUILD_DIR)/client
	$(CC) $(CFLAGS) -c src/client/client.c -o $(BUILD_DIR)/client/client.o

$(BUILD_DIR)/server/server.o: src/server/server.c | $(BUILD_DIR)/server
	$(CC) $(CFLAGS) -c src/server/server.c -o $(BUILD_DIR)/server/server.o

$(BUILD_DIR)/game/game.o: src/game/game.c | $(BUILD_DIR)/game
	$(CC) $(CFLAGS) -c src/game/game.c -o $(BUILD_DIR)/game/game.o

# Limpia archivos objeto y ejecutables
clean:
	rm -rf $(BUILD_DIR)
