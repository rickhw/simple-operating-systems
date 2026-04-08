#include "stdio.h"
#include "syscall.h"
#include "net.h"

int main() {
    uint16_t port = 8080;

    printf("Starting Simple OS UDP Server on port %d...\n", port);

    // 1. 綁定通訊埠
    sys_udp_bind(port);

    printf("Waiting for messages from macOS...\n");
    printf("(Run 'echo \"msg\" | nc -u 127.0.0.1 %d' on macOS)\n", port);
    printf("----------------------------------------\n");

    char buffer[256];

    // 2. 進入無限迴圈聆聽 (Server Loop)
    while (1) {
        // 如果成功讀取到新訊息
        if (sys_udp_recv(buffer) == 1) {
            printf(">> [INCOMING]: %s\n", buffer);
        }

        // 稍微延遲一下，避免 100% 佔用 CPU 資源
        for (volatile int i = 0; i < 5000000; i++) {}
    }

    return 0; // 實際上不會走到這裡
}
