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
  char users[MAX_USERS][100];
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

  printf("Username: %s\n", username);

  strncpy(users.users[users.total_users], username, strlen(username));
  users.users[users.total_users][strlen(username)] = '\0';
  users.total_users++;

  printf("USER REGISTERED\n");

  pthread_mutex_unlock(&users.lock);
}

void sendUserList(int sockfd, struct sockaddr_in *clientAddress)
{
  pthread_mutex_lock(&users.lock);

  Packet userListPacket = {0};
  userListPacket.type = 4; // Type 4 for user list response

  char userList[BUFFER_SIZE] = "";
  for (size_t i = 0; i < users.total_users; i++)
  {
    strncat(userList, users.users[i], sizeof(userList) - strlen(userList) - 1);
    if (i < users.total_users - 1)
    {
      strncat(userList, ",", sizeof(userList) - strlen(userList) - 1);
    }
  }

  printf("User list: %s\n", userList);

  strncpy(userListPacket.data, userList, sizeof(userListPacket.data) - 1);
  pthread_mutex_unlock(&users.lock);

  sendPacket(sockfd, clientAddress, &userListPacket);
}

void deleteUsername(const char *username)
{
  pthread_mutex_lock(&users.lock);

  int found = 0;
  for (size_t i = 0; i < users.total_users; i++)
  {
    if (strcmp(users.users[i], username) == 0)
    {
      found = 1;
      for (size_t j = i; j < users.total_users - 1; j++)
      {
        strncpy(users.users[j], users.users[j + 1], sizeof(users.users[j]));
      }
      users.total_users--;
      printf("USER DELETED: %s\n", username);
      break;
    }
  }

  if (!found)
  {
    printf("User not found: %s\n", username);
  }

  pthread_mutex_unlock(&users.lock);
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
  case 1:
    saveUsername(packet.data);
    sendUserList(sockfd, clientAddress);
    
    break;
  case 3:
    sendUserList(sockfd, clientAddress);
    
    break;
  case 5:
    deleteUsername(packet.data);
    
    sendUserList(sockfd, clientAddress);
    
    break;
  case 6:
    receiveFile(sockfd, clientAddress, "server_files");

    break;
  case 7:
    char filename[BUFFER_SIZE];

    snprintf(filename, sizeof(filename), "%s/%s", "server_files", packet.data);

    sendFile(sockfd, clientAddress, filename);
    
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
