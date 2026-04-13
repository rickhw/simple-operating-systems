#include "stdio.h"
#include "syscall.h"

extern void parse_ip(char* str, uint8_t* ip);
extern int atoi(char* str);

int main(int argc, char** argv) {
    if (argc < 3) return 1;

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);

    printf("1. Initiating TCP 3-Way Handshake...\n");
    sys_tcp_connect(target_ip, port); // 觸發 ARP
    msleep(500);

    sys_tcp_connect(target_ip, port); // 真正的 SYN
    // 等待對方回傳 SYN, ACK，Kernel 會在背景自動回傳 ACK
    msleep(500);

    printf("2. Sending Data (PSH)...\n");
    char* message = "Hello macOS, this is Simple OS over TCP!\n";
    sys_tcp_send(target_ip, port, message);

    // 等待對方接收
    msleep(500);

    printf("3. Closing Connection (FIN)...\n");
    sys_tcp_close(target_ip, port);

    printf("TCP Sequence Complete.\n");
    return 0;
}
