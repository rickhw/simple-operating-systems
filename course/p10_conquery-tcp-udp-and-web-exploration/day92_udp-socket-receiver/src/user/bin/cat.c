#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }

    int fd = open(argv[1]);
    if (fd == -1) {
        printf("cat: file not found: ");
        printf(argv[1]);
        printf("\n");
        return 1;
    }

    char buffer[128];
    for(int i=0; i<128; i++) buffer[i] = 0; // 清空緩衝區

    read(fd, buffer, 100);
    printf(buffer);
    printf("\n");

    return 0;
}
