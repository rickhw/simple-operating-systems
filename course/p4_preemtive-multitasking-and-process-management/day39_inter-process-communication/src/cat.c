int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        sys_print("Usage: cat <filename>\n");
        return 1;
    }

    int fd = sys_open(argv[1]);
    if (fd == -1) {
        sys_print("cat: file not found: ");
        sys_print(argv[1]);
        sys_print("\n");
        return 1;
    }

    char buffer[128];
    for(int i=0; i<128; i++) buffer[i] = 0; // 清空緩衝區

    sys_read(fd, buffer, 100);
    sys_print(buffer);
    sys_print("\n");

    return 0;
}
