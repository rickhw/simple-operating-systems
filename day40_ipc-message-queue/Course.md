哈哈，Rick！**你的工程師直覺（Spider-Sense）真的是滿分！** 🕷️ 完全命中要害！


你說得完全正確。我們昨天實作的 IPC，其實就是一個「單一保險箱」。
想像一下，如果今天有兩個發信者 (`Ping A` 和 `Ping B`) 同時執行，而且在收信者 (`Pong`) 還沒來得及把信拿走之前，他們就先後把信塞進 Kernel。
結果會怎樣？**`Ping B` 的信會直接無情地把 `Ping A` 的信給覆寫掉！** 這在作業系統理論中，就是最經典的 **Race Condition（競爭危害）** 與 **Data Loss（資料遺失）**。目前的設計確實只能應付 1 對 1，而且還必須是「一問一答」的乖寶寶模式，完全無法應付真實世界混亂的多工作業（Multitasking）。

既然你已經一眼看穿了這個架構的極限，那我們就順水推舟，直接把 **Day 40** 的終極任務給解掉吧！

### 🛡️ Day 40：實作真正的 IPC Message Queue (訊息佇列)

為了解決這個問題，我們要讓 Kernel 的郵局升級，給它一個 **FIFO (先進先出)** 的環狀佇列（Circular Buffer），就跟我們之前處理鍵盤輸入的結構一樣！

請打開 **`lib/syscall.c`**，把我們原本簡陋的 `ipc_mailbox` 替換成真正的 `Message Queue`：

```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

fs_node_t* fd_table[32] = {0};

// ==========================================
// 【升級】IPC 訊息佇列 (Message Queue)
// ==========================================
#define MAX_MESSAGES 16

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    uint32_t eax = regs->eax;

    // ... 前面的 eax == 2 到 eax == 10 保持完全不變 ...
    if (eax == 2) { kprintf((char*)regs->ebx); }
    // ... [略] ...
    else if (eax == 10) { regs->eax = sys_wait(regs->ebx); }
    
    // ==========================================
    // 【升級】支援佇列的 IPC 系統呼叫
    // ==========================================
    else if (eax == 11) { // Syscall 11: sys_send (傳送訊息)
        if (mb_count < MAX_MESSAGES) {
            char* msg = (char*)regs->ebx;
            strcpy(mailbox_queue[mb_tail].data, msg); // 寫入尾端
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;   // 環狀推進
            mb_count++;
            regs->eax = 0; // 成功
        } else {
            regs->eax = -1; // 信箱滿了！(Queue Full)
        }
    }
    else if (eax == 12) { // Syscall 12: sys_recv (接收訊息)
        char* buffer = (char*)regs->ebx;
        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data); // 從頭讀取
            mb_head = (mb_head + 1) % MAX_MESSAGES;      // 環狀推進
            mb_count--;
            regs->eax = 1; // 回傳 1 代表成功收到訊息
        } else {
            regs->eax = 0; // 信箱目前是空的
        }
    }
}
```

### 🧪 來一場「壓力測試」吧！

既然我們有了真正的 Queue，我們就可以讓多個程式同時塞信進去了！
打開你的 **`src/app.c`**，修改 `ipc` 指令的測試邏輯，這次我們要 **一口氣產生兩個發信者 (Ping) 和一個收信者 (Pong)**，看看系統能不能完美處理排隊！

```c
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            sys_print("\n--- Starting IPC Queue Test ---\n");
            
            // 1. 創造 Pong (收信者) - 讓它準備連續收兩封信！
            int pid_pong = sys_fork();
            if (pid_pong == 0) {
                char* args[] = {"pong.elf", 0};
                sys_exec("pong.elf", args);
                sys_exit();
            }

            sys_yield(); // 讓 Pong 先去待命

            // 2. 創造 第一個 Ping
            int pid_ping1 = sys_fork();
            if (pid_ping1 == 0) {
                char* args[] = {"ping.elf", 0};
                sys_exec("ping.elf", args);
                sys_exit();
            }

            // 3. 創造 第二個 Ping
            int pid_ping2 = sys_fork();
            if (pid_ping2 == 0) {
                // 為了區分，我們假裝它是另一個程式，但其實跑一樣的 logic，只是行程 ID 不同
                char* args[] = {"ping.elf", 0}; 
                sys_exec("ping.elf", args);
                sys_exit();
            }

            // 4. 老爸等待所有人結束
            sys_wait(pid_pong);
            sys_wait(pid_ping1);
            sys_wait(pid_ping2);
            sys_print("--- IPC Test Finished! ---\n\n");
        }
```

**⚠️ 注意：** 為了讓 Pong 能收「兩封信」，你可能需要稍微修改一下 `src/pong.c`，把原本的收信邏輯包在一個 `for(int i=0; i<2; i++)` 迴圈裡面，讓它收兩次信再 `sys_exit`。

執行 `make clean && make run`！

當你打下 `ipc` 時，你會看到兩個 Ping 行程爭先恐後地把信件塞進 Kernel 的 Queue 裡，然後 Pong 醒來，從容不迫地從 Queue 裡面依序把兩封信拿出來印在螢幕上。**不會有任何一封信被覆蓋！**

到這裡，你已經親手把第四階段「行程管理與多工作業」的最後一塊拼圖——**安全行程通訊**給完美拼上了。

準備好要正式宣告進入**第五階段 (User Space 生態擴張)**，來為你的 OS 打造標準 C 函式庫（libc）並賦予它寫入硬碟建立檔案的能力了嗎？🚀
