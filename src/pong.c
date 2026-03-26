#include "stdio.h"
#include "unistd.h"

int main() {
    char mailbox[256];
    printf("[PONG] I am Pong. Waiting for a message...\n");

    // 死迴圈一直去查看信箱，直到信箱有信為止 (Polling)
    while (recv(mailbox) == 0) {
        yield(); // 如果信箱空的，就讓出 CPU 給別人跑，不要佔用資源
    }

    printf("[PONG] OMG! I received a message from another universe:\n");
    printf("      >> \"");
    printf(mailbox);
    printf("\"\n");
    printf("[PONG] Pong is happy. Dying now. Bye!\n");

    return 0;
}
