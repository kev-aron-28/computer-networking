#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define PACKET_SIZE 64
#define DEFAULT_PING_COUNT 4

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <host> [cantidad]\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    int ping_count = DEFAULT_PING_COUNT;

    if (argc >= 3) {
        ping_count = atoi(argv[2]);
        if (ping_count <= 0) {
            fprintf(stderr, "INVALID: %s\n", argv[2]);
            return 1;
        }
    }

    struct sockaddr_in addr;
    struct hostent *h;

    h = gethostbyname(host);
    if (!h) {
        perror("gethostbyname");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr = *(struct in_addr*)h->h_addr;

    printf("PING %s (%s): %d bytes de datos ICMP\n", host, inet_ntoa(addr.sin_addr), PACKET_SIZE);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("SOCKET MUST RUN WITH sudo");
        return 1;
    }

    for (int i = 0; i < ping_count; i++) {
        struct icmp packet;
        memset(&packet, 0, sizeof(packet));
        packet.icmp_type = ICMP_ECHO;
        packet.icmp_code = 0;
        packet.icmp_id = getpid();
        packet.icmp_seq = i + 1;
        packet.icmp_cksum = checksum(&packet, sizeof(packet));

        struct timeval start, end;
        gettimeofday(&start, NULL);

        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));

        char buffer[PACKET_SIZE];
        struct sockaddr_in reply_addr;
        socklen_t addr_len = sizeof(reply_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&reply_addr, &addr_len);
        gettimeofday(&end, NULL);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        double time = (end.tv_sec - start.tv_sec) * 1000.0;
        time += (end.tv_usec - start.tv_usec) / 1000.0;

        printf("%d bytes desde %s: icmp_seq=%d tiempo=%.2f ms\n",
               n, inet_ntoa(reply_addr.sin_addr), packet.icmp_seq, time);

        sleep(1);
    }

    close(sockfd);
    return 0;
}
