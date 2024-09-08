#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main(int argc, char const *argv[])
{
  
  int status, valueRead;

  int clientDescriptor;

  struct sockaddr_in serverAddress;

  char * hello = "Hello world from client";

  char buffer[1024] = { 0 };

  clientDescriptor = socket(AF_INET, SOCK_STREAM, 0);

  if(clientDescriptor < 0) {
    printf("Could not create the client socket \n");

    return -1;
  }

  serverAddress.sin_family = AF_INET;

  serverAddress.sin_port = htons(PORT);

  int convertIpToBinary = inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

  if(convertIpToBinary <= 0) {
    printf("Invalid address");

    return -1;
  }

  status = connect(clientDescriptor, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

  if(status < 0) {
    printf("Connection failed with server \n");
    
    return -1;
  }

  send(clientDescriptor, hello, strlen(hello), 0);

  printf("Message sent");


  valueRead = read(clientDescriptor, buffer, 1024 - 1);

  printf("%s \n", buffer);


  close(clientDescriptor);

  return 0;
}
