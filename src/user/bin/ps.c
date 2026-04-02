/**
 * @file src/user/bin/ps.c
 * @brief Main logic and program flow for ps.c.
 *
 * This file handles the operations and logic associated with ps.c.
 */

#include "stdio.h"
#include "unistd.h"

// 狀態碼轉換 (對應 Kernel 裡的 TASK_RUNNING, WAITING 等)
const char* state_to_string(unsigned int state) {
    switch (state) {
        case 0: return "RUNNING";
        case 1: return "DEAD   ";
        case 2: return "WAITING";
        case 3: return "ZOMBIE ";
        default: return "UNKNOWN";
    }
}

int main(int argc, char** argv) {
    process_info_t procs[32]; // 假設系統最多同時有 32 個行程

    // 【新增】安全起見，先將陣列全部清零 (假設你的 User Space 有 memset)
    for (int i = 0; i < 32 * sizeof(process_info_t); i++) {
        ((char*)procs)[i] = 0;
    }

    // 呼叫 Syscall，取得目前所有的行程清單
    int count = get_process_list(procs, 32);

    if (count == 0) {
        printf("Error: Could not retrieve process list.\n");
        return -1;
    }

    printf("\n");
    // 簡單的表格標頭 (因為我們的 printf 還沒實作 %8d 這種對齊，所以先用空白稍微排版)
    printf("PID   PPID   STATE      MEMORY      CMD\n");
    printf("---   ----   -----      ------      ---\n");

    for (int i = 0; i < count; i++) {
        // 印出每一筆行程的資料
        printf("%d     %d      %s    %d B       %s\n",
            procs[i].pid,
            procs[i].ppid,
            state_to_string(procs[i].state),
            procs[i].memory_used,
            procs[i].name
        );
    }
    printf("\nTotal processes: %d\n", count);

    return 0;
}
