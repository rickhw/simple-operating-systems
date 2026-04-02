#include "stdio.h"
#include "unistd.h"

int main() {
    printf("Simple OS Top - Press ANY KEY to exit...\n");

    int child_pid = fork();

    if (child_pid == 0) {
        // ==========================================
        // 【子行程】負責在無窮迴圈裡畫畫面
        // ==========================================
        process_info_t old_procs[32];
        process_info_t new_procs[32];

        while (1) {
            int count = get_process_list(old_procs, 32);
            for(volatile int i = 0; i < 5000000; i++); // 休息一下
            get_process_list(new_procs, 32);

            clear_screen();
            printf("PID   NAME         STATE      MEMORY      %%CPU\n");
            printf("---   ----         -----      ------      ----\n");

            for (int i = 0; i < count; i++) {
                unsigned int dt = new_procs[i].total_ticks - old_procs[i].total_ticks;
                printf("%d     %s    %s    %d B      %d%%\n",
                    new_procs[i].pid, new_procs[i].name, "RUNNING", new_procs[i].memory_used, (int)dt);
            }
        }
    } else {
        // ==========================================
        // 【父行程】負責監聽鍵盤與當殺手
        // ==========================================
        // getchar() 會「阻塞 (Block)」，直到鍵盤驅動收到任何按鍵為止！
        getchar();

        // 使用者按了鍵盤！醒來後立刻殺掉還在無窮迴圈的子行程
        kill(child_pid);

        // 【關鍵修復】替兒子收屍，把它從 ZOMBIE 變成 DEAD！
        wait(child_pid);

        // 幫畫面擦屁股，然後優雅地結束自己，把控制權還給 Shell
        clear_screen();
        printf("Top exited. Goodbye!\n");
    }

    return 0;
}
