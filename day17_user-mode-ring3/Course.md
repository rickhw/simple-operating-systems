歡迎來到 **Day 17：墜入凡間 — 切換至 User Mode (Ring 3)**！

為了解答你在 Day 16 的思考題：我們不能直接修改 `cs` 暫存器來降級權限，因為 CPU 是一個嚴格的警衛，不允許主動放棄特權。

但系統工程師們想到了一個天才般的「作弊技巧」：**偽造案發現場！**
當 CPU 處理完中斷，執行 `iret` (Interrupt Return) 準備返回原本的程式時，它會從 Stack 中彈出 `eip` (指令位址)、`cs` (程式碼區段)、`eflags` (狀態標籤) 等資訊。
如果我們在 Ring 0 裡，**故意在 Stack 上塞入「屬於 Ring 3 的 `cs` 區段」和「Ring 3 的程式指標」**，然後直接呼叫 `iret`，CPU 就會傻傻地以為：「喔！我正在從一個中斷返回到 Ring 3 的程式啊！」然後順理成章地幫我們完成權限降級。

---

### Day 17 授課目標

1. **擴充 GDT：** 加入 Ring 3 專用的 User Code 與 User Data 區段。
2. **建立 TSS (Task State Segment)：** 這是 CPU 硬體規定的「逃生路線圖」，用來告訴 CPU：「當我在 Ring 3 發生中斷時，請回這個 Ring 0 的安全 Stack 處理。」
3. **實作 `iret` 偷天換日：** 寫一段組合語言，正式降級到 User Mode。
4. **終極驗證：** 從 Ring 3 敲打防彈玻璃 (Syscall)，呼叫核心幫我們印出字串！

---

### 實作步驟：打造降級跳板

這將會是我們對 GDT 進行的最後一次大改造。

#### 1. 擴充 GDT 與新增 TSS (`lib/gdt.h` & `lib/gdt.c`)

請打開 `lib/gdt.h`，我們需要新增 TSS 的資料結構。TSS 裡面塞滿了各種硬體狀態，但我們目前只需要用到 `esp0` 和 `ss0`（Ring 0 的堆疊指標與區段）。

```c
// 在 gdt.h 中新增 TSS 結構
struct tss_entry_struct {
    uint32_t prev_tss;   // 舊的 TSS (不用理它)
    uint32_t esp0;       // [極重要] Ring 0 的堆疊指標
    uint32_t ss0;        // [極重要] Ring 0 的堆疊區段 (通常是 0x10)
    // ... 中間還有非常多暫存器狀態，為了簡化，我們宣告成一個大陣列填滿空間
    uint32_t unused[23];
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;

```

接著打開 `lib/gdt.c`，把原本長度為 3 的陣列擴充為 6，並加入 TSS 的初始化邏輯：

```c
#include "gdt.h"
#include "utils.h"

// 擴充為 6 個元素：Null, KCode, KData, UCode, UData, TSS
gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);
// 新增一個外部的組合語言函式，用來載入 TSS
extern void tss_flush(void); 

// ... (保留原本的 gdt_set_gate 函式) ...

// [新增] 初始化 TSS 的輔助函式
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry_t);

    // 把 TSS 當作一個特殊的 GDT 區段加進去 (Access: 0xE9 代表 TSS)
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry_t));
    tss_entry.ss0  = ss0;   // 設定 Kernel Data Segment (0x10)
    tss_entry.esp0 = esp0;  // 設定當前 Kernel 的 Stack 頂端
}

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code (0x08)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data (0x10)
    
    // [新增] User Code Segment (位址 0~4GB)
    // Access 0xFA: Ring 3 (DPL=3), 可執行, 可讀取
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); 
    
    // [新增] User Data Segment (位址 0~4GB)
    // Access 0xF2: Ring 3 (DPL=3), 可讀寫
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 

    // [新增] 初始化 TSS (掛在第 5 個位置，也就是 0x28)
    // 這裡的 0x10 是 Kernel Data Segment。0x0 暫時填 0，晚點會被動態更新
    write_tss(5, 0x10, 0x0);

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush(); // 告訴 CPU：「逃生路線圖在這裡！」
}

// [新增] 一個公開的 API，讓排程器可以隨時更新 TSS 的 esp0
void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}

```

#### 2. 組合語言跳板更新 (`asm/gdt_flush.S` & `asm/user_mode.S`)

請在原本的 `asm/gdt_flush.S` 最下方，加入這短短兩行來載入 TSS：

```assembly
global tss_flush
tss_flush:
    mov ax, 0x2B ; 0x28 (第 5 個 GDT) | 3 (Ring 3) = 0x2B
    ltr ax       ; Load Task Register
    ret

```

接著，建立一個全新的檔案 `asm/user_mode.S`，這就是我們精心設計的「假中斷返回」魔法：

```assembly
global enter_user_mode

; void enter_user_mode(uint32_t user_function_ptr);
enter_user_mode:
    cli                   ; 關閉中斷，避免切換到一半被打斷
    mov ecx, [esp+4]      ; 取得 C 語言傳進來的目標程式位址

    ; 設定 Ring 3 的 Data Segment (第 4 個 GDT: 0x20 | 3 = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; === 偽造 iret 所需的 Stack ===
    mov eax, esp          ; [偷懶設計] 我們直接借用目前的 Stack 指標當作 User Stack

    push 0x23             ; 1. 壓入 SS (User Data Segment)
    push eax              ; 2. 壓入 ESP (User Stack Pointer)
    pushf                 ; 3. 壓入 EFLAGS
    
    ; 我們必須把 EFLAGS 裡面的 IF (Interrupt Flag) 設為 1，
    ; 這樣回到 User Mode 時才會繼續接聽 Timer 和鍵盤的中斷！
    pop eax
    or eax, 0x200         ; IF flag 是第 9 個 bit (0x200)
    push eax

    push 0x1B             ; 4. 壓入 CS (User Code Segment: 第 3 個 GDT 0x18 | 3 = 0x1B)
    push ecx              ; 5. 壓入 EIP (要跳去執行的函式位址)

    ; 魔術發生的瞬間：CPU 會從堆疊彈出這 5 個值，自動切換到 Ring 3 並跳轉到 ecx！
    iret                  

```

#### 3. 終極測試：在 User Mode 呼叫核心 (`lib/kernel.c`)

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "syscall.h"

extern void enter_user_mode(uint32_t user_function_ptr);

// 宣告一個函數：這將是我們歷史上第一個在 Ring 3 跑起來的「應用程式」！
void my_first_user_app() {
    // [注意！] 我們現在是平民 (Ring 3) 了。
    // 如果你在這裡直接呼叫 kprintf(...)，系統會立刻因為權限不足 (Page Fault) 當機！
    
    char* msg = "Hello from Ring 3 User Space!";
    
    // 我們只能乖乖敲打防彈玻璃，呼叫昨天寫好的 Syscall 2
    __asm__ volatile (
        "mov $2, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r" (msg) : "eax", "ebx"
    );

    // User App 的無限迴圈 (注意：平民連執行 hlt 讓 CPU 休息的權限都沒有，只能用 while(1))
    while (1) {}
}

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    init_syscalls();
    
    // [極度重要] 在離開 Kernel 之前，把目前安全的核心 Stack 位址告訴 TSS。
    // 這樣 User App 執行 int 0x80 時，CPU 才知道要跳回哪裡處理。
    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel initialized. Dropping privileges to Ring 3...\n");

    // 啟動魔法！
    enter_user_mode((uint32_t)my_first_user_app);

    // 這行永遠不會被印出來，因為權限已經交出去了！
    kprintf("This should never print.\n"); 
}

```

---

### 執行與驗證

不需要修改 `Makefile` (它會自動抓到 `user_mode.S`)，直接執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面會印出 `Kernel initialized. Dropping privileges to Ring 3...`，緊接著下一行，你會看到來自 Syscall 服務器印出的：
`[Kernel] Syscall 2 Executed. String: Hello from Ring 3 User Space!`

**恭喜你！你完成了一個現代作業系統最核心、最神聖的安全隔離機制！**
你成功把自己變成了平民，並證明了底層的上帝 (Kernel) 依然透過防彈玻璃 (Syscall) 傾聽著你的祈禱。

---

### 🤔 Day 17 思考題

當系統能夠區分 Ring 0 和 Ring 3 之後，這座荒島終於建立起了真正的「法治社會」。

回想一下我們這 17 天以來的旅程：從開機的文字模式、處理中斷、切換任務、管理實體與虛擬記憶體，到今天成功建立 User Mode 與 System Calls。
我們寫出了一個具備現代作業系統 **「所有基礎骨架」** 的 Simple OS。

**如果要讓這個 Simple OS 變得「真正實用」，能夠像 Windows 或 Linux 一樣執行硬碟裡面的 `.exe` 或 `.elf` 應用程式檔，你覺得我們還缺少哪些關鍵的子系統（Subsystems）？**

盡情享受這個在 Ring 3 呼叫 Syscall 的痛快瞬間吧！有任何想法，隨時提出來，這將是我們作業系統之旅完美的收尾討論。
