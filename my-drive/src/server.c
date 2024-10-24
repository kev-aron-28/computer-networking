#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
  int sockfd;
  
  char buffer[BUFFER_SIZE];

  struct sockaddr_in serverAddr, clientAddr;
  
  socklen_t addr_len = sizeof(clientAddr);

  // Create a UDP socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  // Clear and initialize the server address structure
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);

  // Bind the socket to the server address and port
  if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", PORT);

  // Wait to receive messages from the client
  while (1)
  {
    ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addr_len);
    if (n < 0)
    {
      perror("Failed to receive message");
      continue;
    }

    buffer[n] = '\0'; // Null-terminate the received message
    printf("Received message: %s\n", buffer);

    // Send a response back to the client
    char *response = "Message received!";
    sendto(sockfd, response, strlen(response), 0, (const struct sockaddr *)&clientAddr, addr_len);
  }

  close(sockfd);
  return 0;
}
