/**
 * @file src/user/bin/mkdir.c
 * @brief Main logic and program flow for mkdir.c.
 *
 * This file handles the operations and logic associated with mkdir.c.
 */

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
