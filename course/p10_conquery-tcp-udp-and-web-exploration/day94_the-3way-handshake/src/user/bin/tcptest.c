#include "stdio.h"
#include "syscall.h"
#include "net.h"

extern void parse_ip(char* str, uint8_t* ip);
extern int atoi(char* str);

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: tcptest <ip> <port>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);

    printf("Initiating TCP 3-Way Handshake with %d.%d.%d.%d:%d...\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3], port);

    // 第一發：換 ARP
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {}

    // 第二發：真實的 SYN
    sys_tcp_connect(target_ip, port);

    return 0;
}
