#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "game.h"

#define PORT 3000
#define QUEUE_LEN 3

int main(int argc, char const *argv[])
{
  int status, dataReadFromServer;

  int clientSocketDescriptor;

  struct sockaddr_in serverAddress;


  clientSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

  if(clientSocketDescriptor < 0) {
    printf("Could not create the socket \n");

    return -1;
  }

  serverAddress.sin_family = AF_INET;

  serverAddress.sin_port = htons(PORT);

  int convertIpToBinary = inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

  if(convertIpToBinary <= 0) {
    printf("Invalid address");

    return -1;
  }

  status = connect(clientSocketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

  if(status < 0) {
    printf("Connection failed with server \n");

    return -1;
  }

  char buffer[1024] = { 0 };
  
  int difficulty;

  printf("Choose difficulty (1: Easy, 2: Medium, 3: Hard): ");
  
  scanf("%d", &difficulty);

  sprintf(buffer, "%d", difficulty);

  send(clientSocketDescriptor, buffer, strlen(buffer), 0);

  memset(buffer, 0, sizeof(buffer));

  dataReadFromServer = read(clientSocketDescriptor, buffer, 1024 - 1);

  printf("%s \n", buffer);

  memset(buffer, 0, sizeof(buffer));

  while(1) {
    int row, col, isMarked;

    printf("ROW COL ISMARKED: \n");

    scanf("%d %d %d", &row, &col, &isMarked);

    char move[6];

    sprintf(move, "%d %d %d", row, col, isMarked);

    send(clientSocketDescriptor, move, strlen(move), 0);

    char tab[1024] = { 0 };

    dataReadFromServer = read(clientSocketDescriptor, tab, 1024 - 1);

    if(dataReadFromServer > 0) {
      
      if(isWinMessage(tab) || isLostMessage(tab)) {
        printf("%s \n", tab);
        
        break;
      }

      printf("%s\n", tab);

      memset(tab, 0, sizeof(tab));
    } else {
      printf("Server didn't send data\n");
    }

  }

  close(clientSocketDescriptor);
  
  return 0;
}
