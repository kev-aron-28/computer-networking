#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
// Socket related
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

#define PORT 3000
#define QUEUE_LEN 3

int main(int argc, char const *argv[])
{
  int serverDescriptor, clientSocketRequest, newSocket;

  ssize_t dataFromClient;

  struct sockaddr_in serverAddress;

  socklen_t addressLength = sizeof(serverAddress);

  serverDescriptor = socket(AF_INET, SOCK_STREAM, 0);

  if (serverDescriptor < 0)
  {
    perror("Socket creation failed");

    exit(EXIT_FAILURE);
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(PORT);

  int serverBinding = bind(serverDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

  if (serverBinding < 0)
  {
    perror("Binding server to socket failed");

    exit(EXIT_FAILURE);
  }

  if (fcntl(serverDescriptor, F_SETFL, O_NONBLOCK) < 0)
  {
    perror("fcntl failed");
    exit(EXIT_FAILURE);
  }

  int isServerListing = listen(serverDescriptor, QUEUE_LEN);

  if (isServerListing < 0)
  {
    perror("Server failed to listen on port");

    exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d\n", PORT);

  while (1)
  {
    fd_set readfds;
    int max_sd, activity;

    FD_ZERO(&readfds);

    FD_SET(serverDescriptor, &readfds);
    max_sd = serverDescriptor;

    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if (activity < 0)
    {
      perror("select error");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(serverDescriptor, &readfds))
    {
      clientSocketRequest = accept(serverDescriptor, (struct sockaddr *)&serverAddress, &addressLength);

      if (clientSocketRequest < 0)
      {
        perror("Failed to accept request");

        exit(EXIT_FAILURE);
      }

      printf("CLIENT CONNECTED \n");

      char buffer[1024] = {0};

      dataFromClient = read(clientSocketRequest, buffer, 1024 - 1);

      if (dataFromClient <= 0)
      {
        printf("CLIENT DISCONNECTED \n");

        break;
      }

      int difficulty = atoi(buffer);

      printf("DIFFICULTY %d \n", difficulty);

      Game game;

      initGame(&game, difficulty);

      printBoard(&game);

      char *initialStateBoard = serializeBoard(&game);

      send(clientSocketRequest, initialStateBoard, strlen(initialStateBoard), 0);

      time_t startTimeOfClient = time(NULL);

      while (1)
      {

        memset(buffer, 0, sizeof(buffer));

        dataFromClient = read(clientSocketRequest, buffer, 1024 - 1);

        if (dataFromClient <= 0)
        {
          printf("CLIENT DISCONNECTED \n");

          break;
        }

        int clientRow, clientColumn, isMarked;

        sscanf(buffer, "%d %d %d", &clientRow, &clientColumn, &isMarked);

        if (isMarked)
        {
          setMarkedCell(&game, clientRow, clientColumn);
        }
        else if (isMovementAMine(&game, clientRow, clientColumn))
        {
          char *gameOverMessage = "GAME OVER";

          send(clientSocketRequest, gameOverMessage, strlen(gameOverMessage), 0);

          time_t endTime = time(NULL);

          double timeTaken = difftime(endTime, startTimeOfClient);

          FILE *recordFile = fopen("records.txt", "a");

          fprintf(recordFile, "Time taken: %.2f seconds\n", timeTaken);

          fclose(recordFile);

          break;
        }
        else
        {
          proccessMovement(&game, clientRow, clientColumn);
        }

        if (hasPlayerWon(&game) || hasPlayerMarkedAllMines(&game))
        {
          char *winGameMessage = "You won the game";

          send(clientSocketRequest, winGameMessage, strlen(winGameMessage), 0);

          time_t endTime = time(NULL);

          double timeTaken = difftime(endTime, startTimeOfClient);

          FILE *recordFile = fopen("./records.txt", "a");

          fprintf(recordFile, "Time taken: %.2f seconds\n", timeTaken);

          fclose(recordFile);

          break;
        }

        char *updatedBoard = serializeBoard(&game);

        send(clientSocketRequest, updatedBoard, strlen(updatedBoard), 0);
      }

      close(clientSocketRequest);
    }

    close(serverDescriptor);
  }

  return 0;
}
