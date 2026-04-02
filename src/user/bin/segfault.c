/**
 * @file src/user/bin/segfault.c
 * @brief Main logic and program flow for segfault.c.
 *
 * This file handles the operations and logic associated with segfault.c.
 */

#include "stdio.h"
#include "unistd.h"

int main() {
    printf("Prepare to crash intentionally...\n");

    // 刻意製造一個空指標 (NULL Pointer)
    int *bad_ptr = (int*)0x00000000;

    // 嘗試寫入 0x00000000 (這塊記憶體沒有被 Paging 映射)
    // 這行會立刻觸發 CPU 硬體中斷 (Page Fault)！
    *bad_ptr = 42;

    // 如果 OS 防護失敗，或者沒有擋下來，這行就會印出來
    printf("Wait... I survived?! (This should never print)\n");

    return 0;
}
