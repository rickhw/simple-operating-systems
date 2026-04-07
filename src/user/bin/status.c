#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

int main() {
    // 1. 向 Window Manager 申請一個名為 "System Status" 的視窗
    int win_id = create_gui_window("System Status", 300, 200);

    if (win_id < 0) {
        printf("Failed to create GUI Window.\n");
        return -1;
    }

    // 2. 進入守護迴圈 (Daemon Loop)
    // 只要這個 while(1) 活著，視窗就會一直存在。
    // 當使用者點擊視窗的 [X]，Kernel 會直接從底層 sys_kill 這個程式，迴圈就會被強制終結！
    while (1) {
        yield(); // 讓出 CPU，避免佔用 100% 資源
    }

    return 0;
}
