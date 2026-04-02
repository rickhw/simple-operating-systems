/**
 * @file src/user/bin/rm.c
 * @brief Main logic and program flow for rm.c.
 *
 * This file handles the operations and logic associated with rm.c.
 */

#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return 1;
    }

    char* filename = argv[1];

    int result = remove(filename);

    if (result == 0) {
        printf("[RM] File '%s' has been deleted forever.\n", filename);
    } else {
        printf("[RM] Error: File '%s' not found!\n", filename);
    }

    return 0;
}
