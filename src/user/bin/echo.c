/**
 * @file src/user/bin/echo.c
 * @brief Main logic and program flow for echo.c.
 *
 * This file handles the operations and logic associated with echo.c.
 */

#include "stdio.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    printf("\n[MALLOC TEST] Requesting memory...\n");

    // 嘗試動態分配字串
    char* dynamic_str1 = (char*)malloc(32);
    char* dynamic_str2 = (char*)malloc(64);

    if (dynamic_str1 && dynamic_str2) {
        printf("[MALLOC TEST] Success! Addresses: str1=0x%x, str2=0x%x\n",
               (unsigned int)dynamic_str1, (unsigned int)dynamic_str2);

        // 賦值測試
        dynamic_str1[0] = 'H'; dynamic_str1[1] = 'I'; dynamic_str1[2] = '\0';
        printf("[MALLOC TEST] Wrote to str1: %s\n", dynamic_str1);

        // 釋放記憶體
        free(dynamic_str1);
        free(dynamic_str2);
        printf("[MALLOC TEST] Memory freed successfully.\n");
    } else {
        printf("[MALLOC TEST] FAILED to allocate memory.\n");
    }

    return 0;
}
