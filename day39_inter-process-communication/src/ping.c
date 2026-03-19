void sys_print(char* msg) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory"); }
void sys_send(char* msg) { __asm__ volatile ("int $0x80" : : "a"(11), "b"(msg) : "memory"); }

int main() {
    sys_print("[PING] I am Ping. I will leave a message in the Kernel Mailbox!\n");
    sys_send("Hello from the Ping Universe! Can you hear me?");
    sys_print("[PING] Message sent. Ping is dying now. Bye!\n");
    return 0;
}
