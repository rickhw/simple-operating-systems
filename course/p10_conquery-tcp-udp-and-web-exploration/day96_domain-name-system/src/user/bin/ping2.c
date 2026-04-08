#include "stdio.h"
#include "unistd.h"

int main() {
    printf("[PING] I am Ping. I will leave a message in the Kernel Mailbox!\n");
    send("Hello from the Ping Universe! Can you hear me?");
    printf("[PING] Message sent. Ping is dying now. Bye!\n");
    return 0;
}
