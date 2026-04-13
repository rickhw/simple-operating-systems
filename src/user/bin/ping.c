#include "stdio.h"
#include "syscall.h"
#include "net.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ping <ip_address>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);

    printf("PING %d.%d.%d.%d 32 bytes of data.\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    // 連發 4 顆 Ping (模擬 Windows/Linux 的行為)
    for (int i = 0; i < 4; i++) {
        sys_ping(target_ip);

        // 延遲一下，避免發送過快導致網卡信箱塞車或路由器忽略
        msleep(500);
    }

    return 0;
}
