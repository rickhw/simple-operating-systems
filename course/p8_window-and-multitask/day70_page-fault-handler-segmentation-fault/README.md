太神啦，Rick！這張截圖真的太美了！看到 `paint.elf` 在背景獨立運作，畫出那經典的 XOR 漸層圖案，而且還同時開了兩個視窗，這絕對是作業系統開發過程中最讓人感動的時刻！🎉

既然我們已經見識過因為記憶體越界而導致系統「全面崩潰 (Double Fault)」的可怕，**Day 70**，我們就來把這個未爆彈徹底拆除，為你的作業系統裝上現代 OS 最具指標性的防護機制：**Page Fault Handler (分頁錯誤攔截) 與 Segmentation Fault (記憶體區段錯誤)！**


在現代 OS 中，如果一個 User Space 的程式（Ring 3）存取了不屬於它的記憶體，Kernel **絕對不能**跟著當機。Kernel 必須像個無情的裁判，吹哨、把違規的程式紅牌出場 (Kill)，然後讓系統繼續正常運作。

請跟著我進行這 3 個步驟，打造這個強大的防護罩：

---

### 步驟 1：撰寫分頁錯誤處理器 (`src/kernel/lib/paging.c`)

當 CPU 發現程式存取了不合法的記憶體時，它會觸發 **中斷 14 (Interrupt 14)**。並且會把肇事的記憶體位址偷偷塞進 `CR2` 暫存器裡。

打開 **`src/kernel/lib/paging.c`**，在檔案的最下方加入這個完美的防護函數：

```c
#include "task.h" // 確保有引入 task.h 才能知道 current_task

// 【Day 70 新增】Page Fault 專屬的攔截與處理中心
void page_fault_handler(registers_t *regs) {
    // 1. 從 CR2 暫存器讀出「引發錯誤的記憶體位址」
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // 2. 檢查錯誤是發生在 User Space 還是 Kernel Space
    // CS 暫存器的最後兩個 bit (CPL) 如果是 3，代表是 Ring 3 (User Space)
    int is_user_space = (regs->cs & 0x3) == 3;

    if (is_user_space && current_task != 0) {
        // ==========================================
        // 【裁判吹哨】User 程式違規！判處死刑！
        // ==========================================
        kprintf("\n[Kernel] Segmentation Fault!\n");
        kprintf("  -> PID %d (%s) tried to access unmapped memory at 0x%x\n", 
                current_task->pid, current_task->name, faulting_address);
        kprintf("  -> Process has been terminated to protect the system.\n");

        // 呼叫我們 Day 66 寫的死神，直接把這個不受控的程式殺掉
        extern int sys_kill(int pid);
        sys_kill(current_task->pid);

        // 【極度重要】強制呼叫排程器切換任務！
        // 絕對不能 return 回去，否則 CPU 會無限重複執行那行錯誤的指令
        schedule(); 
    } else {
        // ==========================================
        // Kernel 自己踩雷了，這才是真正的世界末日 (Kernel Panic)
        // ==========================================
        kprintf("\n========================================\n");
        kprintf("KERNEL PANIC: PAGE FAULT IN RING 0!\n");
        kprintf("Faulting Address: 0x%x\n", faulting_address);
        kprintf("EIP: 0x%x\n", regs->eip);
        kprintf("========================================\n");
        
        while(1) { __asm__ volatile("cli; hlt"); } // 凍結系統
    }
}
```

---

### 步驟 2：將防護罩掛載到 IDT (中斷描述表)

我們必須告訴 CPU，當發生中斷 14 的時候，請把控制權交給我們剛剛寫好的 `page_fault_handler`。

這部分取決於你的中斷分發架構（通常在 `idt.c` 或 `isr.c`）。
假設你有一個用來註冊中斷的函數（例如 `register_interrupt_handler`），請在你的 Kernel 初始化階段（例如 **`src/kernel/kernel.c`** 裡的 `init_idt()` 之後，或是直接修改 `idt.c`），綁定這支函數：

```c
// 在 kernel.c 或 idt.c 的初始化區段中加入這行：
extern void page_fault_handler(registers_t *regs);

// 假設你的系統有類似這種註冊機制：
// register_interrupt_handler(14, page_fault_handler);

// 💡 【重要提示】：如果你目前的 OS 架構是把所有異常都統一導向 
// 一個類似 `check_exception` 或 `isr_handler` 的全域函數，
// 請在那個函數裡面加上判斷：
// if (regs->int_no == 14) { page_fault_handler(regs); return; }
```
*(如果你不確定你的 OS 目前怎麼掛載 Interrupt 14，可以把處理 ISR 分發的檔案貼給我看！)*

---

### 步驟 3：撰寫「作死」測試程式 (`src/user/bin/segfault.c`)

為了驗證防護罩是否生效，我們要刻意寫一支會「爆炸」的程式來測試它。

建立 **`src/user/bin/segfault.c`**：

```c
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
```

別忘了：
1. 將 `"segfault.elf"` 加入 `kernel.c` 的 `setup_filesystem` 陣列。
2. 將 `module /boot/segfault.elf` 加入 `scripts/grub.cfg`。

---

### 🚀 驗收：見證 Segmentation Fault 的誕生！

存檔編譯，執行 `make clean && make run`！

進入 Desktop 後打開 Terminal，準備好你的爆米花，大膽輸入：
```bash
segfault
```

**你將會看到：**
1. 程式印出 `Prepare to crash intentionally...`。
2. 接著，畫面上（或者 Kernel 的 Debug Log 中）會印出我們設定的紅色警戒：
   `[Kernel] Segmentation Fault!`
   `-> PID X (segfault.elf) tried to access unmapped memory at 0x0`
3. 最爽的是：**你的作業系統完全沒有當機！** `SimpleOS>` 提示字元會再次優雅地出現，你依然可以繼續輸入 `ls` 或打開 `paint`！

有了這個防護機制，你的 Kernel 就像穿上了防彈衣，User Space 的程式再怎麼亂搞、越界存取、Stack Overflow，都不可能把系統搞垮了！

完成這個壯舉後，告訴我你想在 **Day 71** 玩什麼！是「實作時鐘與日期 (Date/Time) API」還是「讓你的視窗支援滑鼠拖曳」呢？😎
