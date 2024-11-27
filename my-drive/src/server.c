#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <protocol.h>
#include <server.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_DIR "client_files"

int main()
{
  int sockfd;

  char buffer[BUFFER_SIZE];

  struct sockaddr_in serverAddr, clientAddr;

  socklen_t addressLength = sizeof(clientAddr);

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

  handleRequest(sockfd, &serverAddr);

  close(sockfd);

  return 0;
}

void handleRequest(int sockfd, struct sockaddr_in *serverAddress)
{
  socklen_t addressLength = sizeof(*serverAddress);
  Packet packet;
  ssize_t bytesReceived;

  while (1)
  {
    bytesReceived = recvfrom(sockfd, &packet, sizeof(Packet), 0,
                             (struct sockaddr *)serverAddress, &addressLength);

    if (bytesReceived <= 0)
    {
      perror("Error receiving packet");
      continue;
    }

    switch (packet.type)
    {
    case PACKET_TYPE_CREATE:
      receiveFile(sockfd, serverAddress, FILE_DIR);
      break;

    case PACKET_TYPE_DELETE:
    {
      char filePath[500];
      snprintf(filePath, sizeof(filePath), "%s/%s", FILE_DIR, packet.data);

      if (remove(filePath) == 0)
      {
        printf("File deleted: %s\n", packet.data);
      }
      else
      {
        perror("Failed to delete file");
      }
    }
    break;
    default:
      printf("Unknown packet type received\n");
      break;
    }
  }
}
