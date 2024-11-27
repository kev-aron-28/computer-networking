#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <protocol.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // Localhost for testing

void sendCreateRequest(int sockfd, struct sockaddr_in *serverAddr, const char *fileName) {
    Packet packet;
    packet.type = PACKET_TYPE_CREATE;
    snprintf(packet.data, sizeof(packet.data), "%s", fileName);

    // Send the CREATE packet to the server
    sendPacket(sockfd, serverAddr, &packet);
    printf("Sent create request for file: %s\n", fileName);
}

// Function to send a DELETE request
void sendDeleteRequest(int sockfd, struct sockaddr_in *serverAddr, const char *fileName) {
    Packet packet;
    packet.type = PACKET_TYPE_DELETE;
    snprintf(packet.data, sizeof(packet.data), "%s", fileName);

    // Send the DELETE packet to the server
    sendPacket(sockfd, serverAddr, &packet);
    printf("Sent delete request for file: %s\n", fileName);
}

// Function to send a LIST request
void sendListRequest(int sockfd, struct sockaddr_in *serverAddr) {
    Packet packet;
    packet.type = PACKET_TYPE_LIST;
    // Send the LIST packet to the server
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)serverAddr, sizeof(*serverAddr));
    printf("Sent list request to the server\n");
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addressLength = sizeof(serverAddr);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Clear and initialize the server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Convert the IP address from string format to binary format
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "create") == 0 && argc == 3)
    {
        // Send the file upload request
        sendCreateRequest(sockfd, &serverAddr, argv[2]);
        sendFile(sockfd, &serverAddr, argv[2]);
    }
    else if (strcmp(argv[1], "delete") == 0 && argc == 3)
    {
        // Send the file deletion request
        sendDeleteRequest(sockfd, &serverAddr, argv[2]);
    }
    else
    {
        printf("Invalid arguments\n");
        printf("Usage: %s <operation> <filename>\n", argv[0]);
        printf("Operations: create <filename>, delete <filename>, list\n");
    }

    // Close the socket
    close(sockfd);
     
    return EXIT_SUCCESS;
}
