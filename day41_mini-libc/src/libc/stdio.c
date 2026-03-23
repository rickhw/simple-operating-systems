#include "stdio.h"
#include "syscall.h"

void print(const char* str) {
    syscall(2, (int)str, 0, 0, 0, 0); // Syscall 2: sys_print
}

char getchar() {
    return (char)syscall(5, 0, 0, 0, 0, 0); // Syscall 5: sys_getchar
}

int open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}
