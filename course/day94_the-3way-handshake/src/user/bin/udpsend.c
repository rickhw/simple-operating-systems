#include "net.h"
#include "stdio.h"
#include "syscall.h"
#include "string.h"


int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: udpsend <ip> <port> <message>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);
    char* message = argv[3];

    // 先發第一顆當作 ARP 探路 (如果沒有 MAC，這顆會掉)
    sys_udp_send(target_ip, port, message);

    // 等待 Kernel ARP 解析
    for (volatile int j = 0; j < 50000000; j++) {}

    // 再發送第二顆確保成功送達
    sys_udp_send(target_ip, port, message);

    printf("UDP Message sent to %d.%d.%d.%d:%d -> '%s'\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3], port, message);

    return 0;
}
