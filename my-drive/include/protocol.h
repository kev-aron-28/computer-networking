#ifndef PROTOCOL_H
#define PROTOCOL_H

#define WINDOW_SIZE 1000
#define PACKET_SIZE 256
#define MAX_PACKETS 1000
#define BUFFER_SIZE 1024
#define TIMEOUT 2

#define PACKET_TYPE_FILENAME 0
#define PACKET_TYPE_DATA 1
#define PACKET_TYPE_END 2


#define PACKET_TYPE_CREATE 3
#define PACKET_TYPE_DELETE 4
#define PACKET_TYPE_LIST 5

typedef struct {
    int sequenceNumber;
    int type;
    int isLastPacket;
    char data[PACKET_SIZE];
} Packet;

void sendPacket(int socketFileDescriptor, struct sockaddr_in *serverAddress, Packet *packet);
void sendAck(int socketFileDescriptor, struct sockaddr_in *clientAddress, int ackNum);
void sendFile(int socketFileDescriptor, struct sockaddr_in *clientAddress, const char *fileName);
void receiveFile(int socketFileDescriptor, struct sockaddr_in *clientAddress, const char *outputDir);

#endif
