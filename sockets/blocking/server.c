#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>

#define PORT 8080

int main(int argc, char const *argv[])
{

  int socketDescriptor, newSocketDescriptor;

  ssize_t valueRead;

  struct sockaddr_in serverAddress;

  int opt = 1;

  socklen_t addressLen = sizeof(serverAddress);

  char buffer[1024] = { 0 };

  char * messageTest = "Hello world";

  if((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket failed");
  
    exit(EXIT_FAILURE);
  }

  serverAddress.sin_family = AF_INET;

  serverAddress.sin_addr.s_addr = INADDR_ANY;

  serverAddress.sin_port = htons(PORT);

  int serverBinding = bind(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

  if(serverBinding < 0) {
    perror("bind failed");

    exit(EXIT_FAILURE);
  }

  int isServerListening = listen(socketDescriptor, 3);

  if(isServerListening < 0) {
    perror("listen");

    exit(EXIT_FAILURE);
  }

  newSocketDescriptor = accept(socketDescriptor, (struct sockaddr*)&serverAddress, &addressLen);

  if(newSocketDescriptor < 0) {
    perror("accept");

    exit(EXIT_FAILURE);
  }

  valueRead = read(newSocketDescriptor, buffer, 1024 - 1);

  printf("%s\n", buffer);

  send(newSocketDescriptor, messageTest, strlen(messageTest), 0);

  printf("Message sent to client");

  close(newSocketDescriptor);

  close(socketDescriptor);

  return 0;
}
