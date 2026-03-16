#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h" // [Day28] add - 為了使用 simplefs_find
#include "keyboard.h" // [Day30] [新增] 為了使用 keyboard_getchar()
#include "task.h"      // [新增] 為了使用 schedule() 和 exit_task()

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

// 核心的檔案描述符表 (最多允許同時打開 32 個檔案)
// FD 0, 1, 2 通常保留給 stdin, stdout, stderr，所以我們從 3 開始用
fs_node_t* fd_table[32] = {0};

// Syscall 分發中心
void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {

    // 忽略未使用的參數警告
    (void)edi; (void)esi; (void)ebp; (void)esp; (void)edx; (void)ecx;

    if (eax == 2) {
        // Syscall 2: 印出字串 (ebx 存放字串指標)
        kprintf((char*)ebx);
    }
    else if (eax == 3) {
        // Syscall 3: Open File (ebx 存放檔名指標)
        char* filename = (char*)ebx;
        fs_node_t* node = simplefs_find(filename);

        if (node == 0) {
            // 找不到檔案，回傳 -1 (錯誤碼) 放在 eax 讓 Ring 3 讀取
            __asm__ volatile("mov $-1, %%eax" : :);
            return;
        }

        // 尋找一個空的 FD 欄位
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                // 把拿到的號碼牌 (FD) 放進 eax 回傳給 User App
                __asm__ volatile("mov %0, %%eax" : : "r"(i));
                return;
            }
        }
        __asm__ volatile("mov $-1, %%eax" : :); // 表格滿了
    }
    else if (eax == 4) {
        // Syscall 4: Read File (ebx=FD, ecx=Buffer, edx=Size)
        int fd = (int)ebx;
        uint8_t* buffer = (uint8_t*)ecx;
        uint32_t size = (uint32_t)edx;

        // 檢查 FD 是否合法
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            uint32_t bytes_read = vfs_read(fd_table[fd], 0, size, buffer);
            __asm__ volatile("mov %0, %%eax" : : "r"(bytes_read));
        } else {
            __asm__ volatile("mov $-1, %%eax" : :);
        }
    }
    // [Day30][新增] Syscall 5: 讀取一個鍵盤字元
    else if (eax == 5) {
        char c = keyboard_getchar();

        // 為了讓 Shell 有互動感，使用者打什麼，Kernel 就幫忙印在螢幕上 (Echo)
        char str[2] = {c, '\0'};
        kprintf(str);

        __asm__ volatile("mov %0, %%eax" : : "r"((uint32_t)c));
    }
    // [Day33] add -- start
    else if (eax == 6) {
        // [新增] Syscall 6: sys_yield (自願讓出 CPU 時間片)
        schedule();
    }
    else if (eax == 7) {
        // [新增] Syscall 7: sys_exit (終止並退出程式)
        exit_task();
    }
    // [Day33] add -- end
}


// [Day28] delete -- start
// // 這是核心在防彈玻璃後面做事的邏輯
// // 參數會由組合語言從 CPU 暫存器 (eax, ebx, ecx) 裡面撈出來傳給我們
// void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
//     switch (syscall_num) {
//         case 1:
//             // 假設 Syscall 1 是「印出數字」
//             kprintf("[Kernel] Syscall 1 Executed. Arg: %d\n", arg1);
//             break;

//         case 2:
//             // 假設 Syscall 2 是「印出字串」 (arg1 當作指標)
//             kprintf("[Kernel] Syscall 2 Executed. String: %s\n", (char*)arg1);
//             break;

//         default:
//             kprintf("[Kernel] Unknown System Call: %d\n", syscall_num);
//             break;
//     }

//     // 為了避免編譯器警告 arg2 沒用到
//     (void)arg2;
// }
// [Day28] delete -- end
