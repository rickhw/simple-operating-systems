#include "stdio.h"
#include "unistd.h"


// void sys_print(char* msg) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory"); }
// int sys_recv(char* buffer) {
//     int has_msg;
//     __asm__ volatile ("int $0x80" : "=a"(has_msg) : "a"(12), "b"(buffer) : "memory");
//     return has_msg;
// }
// void sys_yield() { __asm__ volatile ("int $0x80" : : "a"(6) : "memory"); }

int main() {
    char mailbox[256];
    print("[PONG] I am Pong. Waiting for a message...\n");

    // 死迴圈一直去查看信箱，直到信箱有信為止 (Polling)
    while (recv(mailbox) == 0) {
        yield(); // 如果信箱空的，就讓出 CPU 給別人跑，不要佔用資源
    }

    print("[PONG] OMG! I received a message from another universe:\n");
    print("      >> \"");
    print(mailbox);
    print("\"\n");
    print("[PONG] Pong is happy. Dying now. Bye!\n");

    return 0;
}
