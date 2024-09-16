#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// Socket related
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define PORT 3000
#define QUEUE_LEN 3

int main(int argc, char const *argv[])
{
  int serverDescriptor, clientSocketRequest;

  ssize_t dataFromClient;

  struct sockaddr_in serverAddress;

  socklen_t addressLength = sizeof(serverAddress);

  serverDescriptor  = socket(AF_INET, SOCK_STREAM, 0);

  if(serverDescriptor < 0) {
    perror("Socket creation failed");

    exit(EXIT_FAILURE);
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(PORT);

  int serverBinding = bind(serverDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

  if(serverBinding < 0) {
    perror("Binding server to socket failed");
    
    exit(EXIT_FAILURE);
  }

  int isServerListing = listen(serverDescriptor, QUEUE_LEN);

  if(isServerListing < 0) {
    perror("Server failed to listen on port");

    exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d\n", PORT);

  Game game;

  initGame(&game, 1);

  printBoard(&game);

  while(1) {
    clientSocketRequest = accept(serverDescriptor, (struct sockaddr*) &serverAddress, &addressLength);

    printf("CLIENT CONNECTED \n");

    if(clientSocketRequest < 0) {
      perror("Failed to accept request");

      exit(EXIT_FAILURE);
    }

    char * initialStateBoard = serializeBoard(&game);
    
    send(clientSocketRequest, initialStateBoard, strlen(initialStateBoard), 0);

    while (1) {
      char buffer[1024] = { 0 };
      
      dataFromClient = read(clientSocketRequest, buffer, 1024 - 1); 

      if(dataFromClient <= 0) {
        printf("CLIENT DISCONNECTED \n");

        break;
      }

      int clientRow, clientColumn, isMarked;

      sscanf(buffer, "%d %d %d", &clientRow, &clientColumn, &isMarked);

      if(isMarked) {
        setMarkedCell(&game, clientRow, clientColumn);

      } else if(isMovementAMine(&game, clientRow, clientColumn)) {
        char *gameOverMessage = "GAME OVER";

        send(clientSocketRequest, gameOverMessage, strlen(gameOverMessage), 0);
 
        break;
      } else {
        proccessMovement(&game, clientRow, clientColumn);
      }

      if(hasPlayerWon(&game)) {
        char *winGameMessage = "You won the game";

        send(clientSocketRequest, winGameMessage, strlen(winGameMessage), 0);
        
        break;
      }
      
      char * updatedBoard =  serializeBoard(&game);

      send(clientSocketRequest, updatedBoard, strlen(updatedBoard), 0);
    }

    close(clientSocketRequest);
  }

  close(serverDescriptor);

  return 0;
}
