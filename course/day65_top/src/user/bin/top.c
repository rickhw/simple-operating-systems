#include "stdio.h"
#include "unistd.h"

int main() {
    process_info_t old_procs[32];
    process_info_t new_procs[32];

    printf("Simple OS Top - Press Ctrl+C to stop\n");

    while (1) {
        // 1. 第一波採樣
        int count = get_process_list(old_procs, 32);

        // 2. 休息一下 (暴力空轉迴圈)
        for(volatile int i = 0; i < 5000000; i++);

        // 3. 第二波採樣
        get_process_list(new_procs, 32);

        clear_screen();
        printf("PID   NAME         STATE      MEMORY      %%CPU\n");
        printf("---   ----         -----      ------      ----\n");

        for (int i = 0; i < count; i++) {
            // 【修改】將 uint32_t 換成 unsigned int！
            unsigned int dt = new_procs[i].total_ticks - old_procs[i].total_ticks;

            // 粗略計算 CPU 使用率
            int cpu_usage = (int)dt;

            printf("%d     %s    %s    %d B      %d%%\n",
                new_procs[i].pid,
                new_procs[i].name,
                "RUNNING",
                new_procs[i].memory_used,
                cpu_usage
            );
        }
    }
    return 0;
}
