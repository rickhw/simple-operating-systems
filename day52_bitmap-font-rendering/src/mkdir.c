#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory_name>\n");
        return 1;
    }

    char* dirname = argv[1];
    printf("[MKDIR] Creating directory '%s'...\n", dirname);

    int result = mkdir(dirname);

    if (result == 0) {
        printf("[MKDIR] Success!\n");
    } else {
        printf("[MKDIR] Failed. Directory might be full.\n");
    }

    return 0;
}
