#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include "protocol.h"

void sendPacket(int socketFileDescriptor, struct sockaddr_in *serverAddress, Packet *packet) {
    ssize_t bytesSent = sendto(
        socketFileDescriptor,
        packet,
        sizeof(Packet),
        0,
        (struct sockaddr *)serverAddress,
        sizeof(*serverAddress)
    );
    
    if (bytesSent == -1) {
        perror("Error sending packet");
    }
}

void sendAck(int socketFileDescriptor, struct sockaddr_in *clientAddr, int ackNum) {
    ssize_t bytesSent = sendto(
        socketFileDescriptor,
        &ackNum,
        sizeof(ackNum),
        0,
        (struct sockaddr *)clientAddr,
        sizeof(*clientAddr)
    );
    
    if (bytesSent == -1) {
        perror("Error sending ACK");
    } else {
        printf("Sent ACK for packet %d\n", ackNum);
    }
}

void sendFile(int sockfd, struct sockaddr_in *clientAddress, const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    socklen_t addressLength = sizeof(*clientAddress);
    printf("BEGIN TO SEND FILE: %s\n", fileName);

    // Send filename packet first
    Packet filenamePacket = {.type = PACKET_TYPE_FILENAME, .sequenceNumber = 0};
    snprintf(filenamePacket.data, PACKET_SIZE, "%s", fileName);
    sendPacket(sockfd, clientAddress, &filenamePacket);
    printf("Filename packet sent: %s\n", fileName);

    //SLIDING WINDOW - GO BACK N

    int basePointer = 0, nextSequenceNumber = 0;
    Packet window[WINDOW_SIZE];
    int ack[MAX_PACKETS] = {0};
    struct timeval timeout = {TIMEOUT, 0};  // Timeout structure

    int bytesRead;
    while (1) {
        // Send packets in the window
        while (nextSequenceNumber < basePointer + WINDOW_SIZE) {
            if (nextSequenceNumber >= MAX_PACKETS) break; // Limit to MAX_PACKETS

            Packet *packet = &window[nextSequenceNumber % WINDOW_SIZE];
            packet->type = PACKET_TYPE_DATA;
            packet->sequenceNumber = nextSequenceNumber;
            bytesRead = fread(packet->data, 1, PACKET_SIZE, file);

            if (bytesRead <= 0) {
                // End of file reached, break out of the loop
                printf("End of file reached\n");
                break;
            }

            sendPacket(sockfd, clientAddress, packet);
            printf("Data packet sent: %d\n", nextSequenceNumber);
            nextSequenceNumber++;
        }

        // Exit if the end of the file has been reached
        if (bytesRead <= 0) {
            break;
        }

        // Set socket timeout for receiving ACKs
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Wait for ACKs and adjust window
        int ackNumber;
        ssize_t ackReceived = recvfrom(sockfd, &ackNumber, sizeof(ackNumber), 0, 
                                       (struct sockaddr *)clientAddress, &addressLength);
        if (ackReceived > 0) {
            printf("ACK received for packet %d\n", ackNumber);
            ack[ackNumber] = 1;
            while (ack[basePointer] && basePointer < MAX_PACKETS) {
                basePointer++;
            }
        } else {
            // Timeout - Retransmit all packets in the window
            printf("Timeout, retransmitting all packets in the window\n");
            for (int i = basePointer; i < nextSequenceNumber; i++) {
                sendPacket(sockfd, clientAddress, &window[i % WINDOW_SIZE]);
                printf("Retransmitted packet: %d\n", i);
            }
        }
    }

    // Send end-of-file packet
    Packet endPacket = {.type = PACKET_TYPE_END, .sequenceNumber = nextSequenceNumber};
    sendPacket(sockfd, clientAddress, &endPacket);
    printf("End-of-file packet sent\n");

    fclose(file);
    printf("File transfer complete\n");
}

void receiveFile(int sockfd, struct sockaddr_in *clientAddress, const char *outputDir) {
    socklen_t addressLength = sizeof(*clientAddress);

    int expectedSequenceNumber = 0;
    Packet packetBuffer[WINDOW_SIZE] = {0};
    int received[WINDOW_SIZE] = {0};

    FILE *file = NULL;
    int fileInitialized = 0;

    while (1) {
        Packet packet;
        int bytesReceived = recvfrom(sockfd, &packet, sizeof(Packet), 0, 
                                     (struct sockaddr *)clientAddress, &addressLength);
        if (bytesReceived <= 0) {
            perror("Failed to receive packet");
            continue;
        }

        if (packet.type == PACKET_TYPE_END) {
            printf("File transfer complete\n");
            break;  // Stop receiving packets once EOF is received
        }

        if (packet.type == PACKET_TYPE_FILENAME) {
            if (!fileInitialized) {
                char filePath[BUFFER_SIZE];

                snprintf(filePath, BUFFER_SIZE, "%s/%s", outputDir, packet.data);

                printf("Saving file as: %s\n", filePath);
                
                file = fopen(filePath, "wb");

                if (!file) {
                    perror("Failed to open file");
                    return;
                }
                fileInitialized = 1;
            }
            continue;
        }

        if (packet.type == PACKET_TYPE_DATA) {
            printf("Received packet %d\n", packet.sequenceNumber);

            int seqNum = packet.sequenceNumber;
            if (seqNum == expectedSequenceNumber) {
                // Write data to file
                fwrite(packet.data, 1, PACKET_SIZE, file);
                expectedSequenceNumber++;
            }

            // Send ACK for the last correctly received packet
            sendAck(sockfd, clientAddress, expectedSequenceNumber - 1);
        }
    }

    if (file) fclose(file);
}
