#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // Localhost for testing
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    socklen_t addr_len = sizeof(serverAddr);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Clear and initialize the server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    // Convert the IP address from string format to binary format
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send a message to the server
    char *message = "Hello from UDP client!";
    sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&serverAddr, addr_len);
    printf("Message sent to server: %s\n", message);

    // Receive a response from the server
    ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addr_len);
    if (n < 0) {
        perror("Failed to receive message from server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0'; // Null-terminate the received message
    printf("Server response: %s\n", buffer);

    close(sockfd);
    return 0;
}
