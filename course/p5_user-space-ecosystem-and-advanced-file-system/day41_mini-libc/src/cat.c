#include "stdio.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: cat <filename>\n");
        return 1;
    }

    int fd = open(argv[1]);
    if (fd == -1) {
        print("cat: file not found: ");
        print(argv[1]);
        print("\n");
        return 1;
    }

    char buffer[128];
    for(int i=0; i<128; i++) buffer[i] = 0; // 清空緩衝區

    read(fd, buffer, 100);
    print(buffer);
    print("\n");

    return 0;
}
