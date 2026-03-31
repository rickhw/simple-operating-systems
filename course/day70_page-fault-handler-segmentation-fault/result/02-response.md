哈哈，Rick！這個 Bug 真的太精彩了！你現在遇到的情況，完美集結了作業系統開發中最經典的 **「三個終極陷阱」**！

從你的截圖和 Core Dump 來看，這一切就像一場精密的骨牌效應。讓我來為你進行這場超級深度的「驗屍解密」：

---

### 🕵️ 案發現場解密 1：為什麼 App 的靈魂互換了？

你說「輸入 `paint &`，卻印出了 `Prepare to crash...` (這是 `segfault.elf` 的字)」，然後「輸入 `segfault`，卻跑出了 System Status 視窗」。
這不是系統錯亂，這是 **GRUB Module 載入順序不符**！

去看看你 `kernel.c` 裡的 `filenames[]` 陣列：
```c
"free.elf", "segfault.elf", "status.elf", "paint.elf"
```
GRUB 在開機時會把硬碟裡的 `.elf` 檔當作 Module 塞進記憶體。**但 GRUB 不知道檔名，它只知道「第 1 個、第 2 個... 第 16 個 Module」！**
如果你的 `scripts/grub.cfg` 裡面的 `module` 順序，跟 `filenames[]` 陣列的順序**沒有 100% 完全對齊**，Kernel 就會把 `segfault.elf` 的二進位內容，命名為 `paint.elf` 存進硬碟裡！
👉 **【修復方式】**：請立刻檢查你的 `scripts/grub.cfg`，確保最下方的 `module` 載入順序，和 `kernel.c` 裡的 `filenames[]` 陣列一模一樣！

---

### 🕵️ 案發現場解密 2：為什麼 `segfault.elf` 沒有被擊斃？

當你輸入 `paint &` (實際上執行了 `segfault.elf`) 時，它執行了 `*bad_ptr = 42;` 去寫入位址 `0x00000000`。
結果它竟然印出了 `"Wait... I survived?!"`，這代表**它沒有觸發 Page Fault！** 為什麼？

因為我們在 `paging.c` 開了一個「上帝模式 (God Mode)」的安全漏洞：
```c
for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
```
旗標 `7` 代表：存在 (Present) | 可讀寫 (RW) | **使用者模式 (User Accessible)**！
也就是說，你的 OS 允許 Ring 3 的平民百姓，隨意讀寫 Kernel 的前 4MB 實體記憶體！所以它寫入 `0x0` 時，CPU 覺得完全合法，就放行了！
👉 **【修復方式 (`src/kernel/lib/paging.c`)】**：把 `first_page_table` 的權限改為 `3` (Supervisor Only)，剝奪 User Space 存取 Kernel 記憶體的權限！
```c
// 將 7 改為 3！這樣 User 程式一摸到前 4MB 就會立刻觸發 Page Fault！
for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 3; } 
```

---

### 🕵️ 案發現場解密 3：為什麼最後會 Double Fault 崩潰？

這是導致你畫面卡死、系統崩潰的真兇。
看看 Core Dump：`IP=0202:0000001d`。
程式的 `CS` (Code Segment) 暫存器竟然變成了 `0x0202` (這是 EFLAGS 的值)！這代表發生了嚴重的 **「堆疊錯位 (Stack Misalignment)」**。

因為我們在 Day 70 的 `registers_t` 結構裡加了 `err_code`，所有的中斷跳板 (`isr128`, `isr32` 等) 都已經乖乖加上 `add esp, 4` 來平衡堆疊了。
**但是！** 當 `sys_fork` 在徒手打造「子行程的 Kernel Stack」時，卻忘記把這 4 Bytes 的 `err_code` 算進去！導致子行程在執行 `iret` 回到 User Space 時，整個暫存器順序大亂，當場墜入虛空！

👉 **【修復方式 (`src/kernel/lib/task.c`)】**：
打開 `task.c`，找到 `sys_fork`，在推入暫存器的地方補上 `err_code`：

```c
    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    // ==========================================
    // 【核心防護】手動補上 4 Bytes 的假錯誤碼！
    // 這樣才能對齊 registers_t，讓 child_ret_stub 完美 iret！
    // ==========================================
    *(--kstack) = 0; 

    *(--kstack) = child_ebp;
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;
```

同時，為了確保子行程的跳板是 100% 正確的，請確認你的 **`src/kernel/asm/switch_task.S`** (或是你放 `child_ret_stub` 的組合語言檔) 裡的 `child_ret_stub` 長這樣：
```asm
global child_ret_stub
child_ret_stub:
    popa            ; 彈出 sys_fork 壓入的 eax, ecx, edx, ebx, esp, ebp, esi, edi
    add esp, 4      ; 【Day 70 關鍵】丟棄剛剛壓入的假 err_code！
    iret            ; 完美返回 User Space！
```

---

### 🚀 浴火重生：真正的現代化 OS

把這三個地方（`grub.cfg` 對齊、`paging.c` 權限、`task.c` 堆疊對齊）都修好後，執行 `make clean && make run`。

這次你再輸入 `segfault`：
你會看到它真的觸發了 `[Kernel] Segmentation Fault` 被無情擊斃。
再輸入 `paint &`：
你會看到那個迷幻的動態漸層畫布再次出現，而且系統穩如泰山！

這就是最硬核的底層 Debug 浪漫！確認這些都修復後，我們隨時準備挺進 **Day 71**！😎
