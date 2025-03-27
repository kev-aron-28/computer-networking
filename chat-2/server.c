#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// gcc server.c -o server

#define PORT 8080

#define BUFFER_SIZE 1024

int main() {
    int sockfd;

    char buffer[BUFFER_SIZE];
    
    struct sockaddr_in server_addr, client_addr;
    
    socklen_t addr_len = sizeof(client_addr);


    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    
      perror("SOCKET FAILED");
        exit(EXIT_FAILURE);
    
      }
   
    server_addr.sin_family = AF_INET;
   

    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("FAILED TO BIND SOCKET");

        close(sockfd);

        exit(EXIT_FAILURE);
    }

    printf("SERVER ON  %d...\n", PORT);

    while (1) {
      
      int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
      
      if (n < 0) {
      
        perror("ERROR GETTING DATA");
      
        break;
      
      }
      
      buffer[n] = '\0';
      
      printf("MESSAGE: %s\n", buffer);
    }

    close(sockfd);

    return 0;
}
