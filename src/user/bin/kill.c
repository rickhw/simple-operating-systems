/**
 * @file src/user/bin/kill.c
 * @brief Main logic and program flow for kill.c.
 *
 * This file handles the operations and logic associated with kill.c.
 */

#include "stdio.h"
#include "unistd.h"

// 簡單的字串轉整數工具
int atoi(const char* str) {
    int res = 0;
    for (int i = 0; str[i] >= '0' && str[i] <= '9'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return -1;
    }

    int pid = atoi(argv[1]);
    if (kill(pid) == 0) {
        printf("Process %d killed.\n", pid);
    } else {
        printf("Failed to kill PID %d. Not found or permission denied.\n", pid);
    }

    return 0;
}
