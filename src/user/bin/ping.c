/**
 * @file src/user/bin/ping.c
 * @brief Main logic and program flow for ping.c.
 *
 * This file handles the operations and logic associated with ping.c.
 */

#include "stdio.h"
#include "unistd.h"
// void sys_printf(char* msg) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory"); }
// void sys_send(char* msg) { __asm__ volatile ("int $0x80" : : "a"(11), "b"(msg) : "memory"); }

int main() {
    printf("[PING] I am Ping. I will leave a message in the Kernel Mailbox!\n");
    send("Hello from the Ping Universe! Can you hear me?");
    printf("[PING] Message sent. Ping is dying now. Bye!\n");
    return 0;
}
