#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <protocol.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_USERS 10

typedef struct
{
  char users[10][100];
  size_t total_users;
  pthread_mutex_t lock;
} UserList;

UserList users;

void saveUsername(const char *username)
{
  pthread_mutex_lock(&users.lock);

  if (users.total_users >= MAX_USERS)
  {
    fprintf(stderr, "User list is full. Cannot add more users.\n");
    pthread_mutex_unlock(&users.lock);
    return;
  }

  strncpy(users.users[users.total_users], username, strlen(username) - 1);
  users.users[users.total_users][strlen(username)] = '\0';
  users.total_users++;

  printf("USER REGISTERED \n");

  pthread_mutex_unlock(&users.lock);
}

void sendUserList(int sockfd, struct sockaddr_in *clientAddress)
{
  pthread_mutex_lock(&users.lock);

  Packet userListPacket = {0};
  userListPacket.type = 4; // Type 4 for user list response

  // Prepare user list as a comma-separated string
  char userList[BUFFER_SIZE] = "";
  for (size_t i = 0; i < users.total_users; i++)
  {
    strncat(userList, users.users[i], sizeof(userList) - strlen(userList) - 1);
    if (i < users.total_users - 1)
    {
      strncat(userList, ", ", sizeof(userList) - strlen(userList) - 1);
    }
  }

  strncpy(userListPacket.data, userList, sizeof(userListPacket.data) - 1);
  pthread_mutex_unlock(&users.lock);

  sendPacket(sockfd, clientAddress, &packet);
}

void handleRequest(int sockfd, struct sockaddr_in *clientAddress)
{
  socklen_t addressLength = sizeof(*clientAddress);
  Packet packet;
  ssize_t bytesReceived;

  bytesReceived = recvfrom(sockfd, &packet, sizeof(Packet), 0,
                           (struct sockaddr *)clientAddress, &addressLength);

  if (bytesReceived < 0)
  {
    perror("Failed to receive packet");
    return;
  }

  switch (packet.type)
  {
  case 1: // Type 1 for username registration
    saveUsername(packet.data);

    // Send acknowledgment
    Packet ack = {0};
    ack.sequenceNumber = packet.sequenceNumber;
    ack.type = 2; // Type 2 for acknowledgment

    sendto(sockfd, &ack, sizeof(Packet), 0, (struct sockaddr *)clientAddress, addressLength);
    break;

  case 3: // Type 3 for user list request
    sendUserList(sockfd, clientAddress);
    break;

  default:
    fprintf(stderr, "Unknown packet type received: %d\n", packet.type);
    break;
  }
}

int main()
{
  int sockfd;
  struct sockaddr_in serverAddr, clientAddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);

  if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", PORT);

  while (1)
  {
    handleRequest(sockfd, &clientAddr);
  }

  close(sockfd);
  return 0;
}
