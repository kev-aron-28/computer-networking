#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAXLINE 1024

int main()
{
  int sockfd;
  char buffer[MAXLINE];
  char *message = "Hello from client";
  struct sockaddr_in servaddr;

  // Create socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));

  // Server address setup
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  servaddr.sin_addr.s_addr = INADDR_ANY;

  int n, len;

  // Send message to server
  sendto(sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
  printf("Hello message sent.\n");

  // Receive response from server
  n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
  buffer[n] = '\0';
  printf("Server: %s\n", buffer);

  close(sockfd);
  return 0;
}
