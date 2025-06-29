#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "game.h"

#define PORT 3000
#define QUEUE_LEN 3

int main(int argc, char const *argv[])
{
  int status, dataReadFromServer, clientSocketDescriptor;

  struct sockaddr_in serverAddress;

  clientSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

  if (clientSocketDescriptor < 0)
  {
    perror("Could not create the socket");
    return -1;
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
  {
    perror("Invalid address");
    return -1;
  }

  if (fcntl(clientSocketDescriptor, F_SETFL, O_NONBLOCK) < 0)
  {
    perror("fcntl failed");
    exit(EXIT_FAILURE);
  }

  status = connect(clientSocketDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

  if (status < 0 && errno != EINPROGRESS)
  {
    perror("Connection failed");
    exit(EXIT_FAILURE);
  }

  char buffer[1024] = {0};
  fd_set readfds;
  int max_sd = clientSocketDescriptor > STDIN_FILENO ? clientSocketDescriptor : STDIN_FILENO;

  int difficulty;

  printf("Choose difficulty (1: Easy, 2: Medium, 3: Hard): ");
  scanf("%d", &difficulty);

  sprintf(buffer, "%d", difficulty);

  send(clientSocketDescriptor, buffer, strlen(buffer), 0);

  memset(buffer, 0, sizeof(buffer));
  dataReadFromServer = read(clientSocketDescriptor, buffer, 1024 - 1);
  printf("Initial Board: \n%s\n", buffer);

  while (1)
  {
    FD_ZERO(&readfds);
    FD_SET(clientSocketDescriptor, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    // Esperar datos en el socket o entrada del usuario
    int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if (activity < 0 && errno != EINTR)
    {
      perror("Select error");
      break;
    }

    // Leer desde el servidor
    if (FD_ISSET(clientSocketDescriptor, &readfds))
    {
      char tab[1024] = {0};
      dataReadFromServer = read(clientSocketDescriptor, tab, 1024 - 1);

      if (dataReadFromServer > 0)
      {
        if (isWinMessage(tab) || isLostMessage(tab))
        {
          printf("%s \n", tab);
          break;
        }

        printf("\n%s\n", tab);
      }
      else if (dataReadFromServer == 0)
      {
        printf("Server closed connection.\n");
        break;
      }
    } else if(FD_ISSET(STDIN_FILENO, &readfds)) {
      int row, col, isMarked;
      printf("ROW COL ISMARKED: ");
      scanf("%d %d %d", &row, &col, &isMarked);

      char move[64];
      sprintf(move, "%d %d %d", row, col, isMarked);

      send(clientSocketDescriptor, move, strlen(move), 0);
    }
  }

  close(clientSocketDescriptor);
  return 0;
}
