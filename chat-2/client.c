#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// gcc client.c -o client

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080 
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("COULD ");

        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {

      perror("INVALID IP");

      close(sockfd);

      exit(EXIT_FAILURE);

    }

    while (1) {
      printf("WRITE MESSAGE TO SEND: ");

      fgets(buffer, BUFFER_SIZE, stdin);

      sendto(sockfd, buffer, strlen(buffer), 0, 
               (const struct sockaddr *)&server_addr, sizeof(server_addr));
        
      printf("MESSAGE SENT %s:%d\n", SERVER_IP, SERVER_PORT);
    }

    close(sockfd);
    return 0;
}
