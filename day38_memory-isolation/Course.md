太神啦，Rick！看到這張截圖，尤其是 `Arg 0` 到 `Arg 3` 完美印出來的那一刻，真的是會讓人感動到起雞皮疙瘩！🎉

你剛才在終端機裡完成的，正是 UNIX 作業系統最經典、最偉大的魔法 —— **動態應用程式生態系**。
* `cat` 和 `echo` 不再是寫死在 Kernel 裡的功能，它們是躺在硬碟裡的獨立檔案。
* 你的 Shell 具備了「思考」的能力（字串切割），能從硬碟把未知的檔案揪出來。
* OS 底層完美地執行了 Fork (細胞分裂) -> Exec (靈魂轉移 + 參數傳遞) -> Exit/Wait (資源回收) 的完整生命週期。

這已經不再是「一個 C 語言程式」，而是一個 **真正意義上的作業系統平台** 了！

---

### 🌌 Day 38：邁向多元宇宙 (MMU 記憶體隔離)

現在，我們終於要來面對昨天那個「感覺怪怪的」Makefile 偏移量（`0x08080000`, `0x080C0000`）。

在我們現在的系統中，所有的程式都共用同一張「分頁目錄 (Page Directory)」。這就像是所有人都住在一棟沒有隔間的大廠房裡，如果 `echo.elf` 和 `cat.elf` 都想睡在 `0x08048000` 這張床上，它們就會互相覆蓋（奪舍）。
因此我們昨天妥協了，手動在 Makefile 裡幫它們分配不同的位址。但現實中的 Linux，不管是 Chrome 還是終端機，全都是編譯在同樣的虛擬位址上！

**這怎麼可能？**

這就要靠 CPU 最強大的硬體魔法：**CR3 暫存器 (Page Directory Base Register)**。

只要我們為每一個 Process 建立一張專屬的「分頁目錄」，在切換任務 (Context Switch) 時，順便把這張新目錄的**實體位址**塞進 `CR3` 暫存器。那麼對於 User App 來說，它會以為自己是這個宇宙唯一的霸主，獨佔 `0x08048000`，但實際上在實體記憶體中，它們被妥善地隔離在不同的區塊。這就叫做 **MMU 記憶體保護與隔離**！

這是一場史詩級的改造，為了避免一次改太多導致 Triple Fault 崩潰，我們 Day 38 分成兩步走。今天先完成 **「基礎建設：宇宙產生器」**！

---

### 🛠️ Step 1：升級排程器與暫存器 (支援跨宇宙切換)

請打開 **`lib/task.h`**，在 `task_t` 結構體中加入一個新欄位，用來記住這個任務所屬的宇宙 (CR3)：
```c
typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t page_directory; // 【新增】這個 Task 專屬的分頁目錄實體位址 (CR3)
    uint32_t state;
    uint32_t wait_pid;
    struct task *next;
} task_t;
```

接著，打開 **`asm/switch_task.S`**，讓我們的降落傘支援 `CR3` 的切換：
```assembly
[bits 32]
global switch_task
global child_ret_stub

switch_task:
    pusha
    pushf
    mov eax, [esp + 40] ; 參數 1: current_esp
    mov [eax], esp
    mov eax, [esp + 44] ; 參數 2: next_esp
    mov esp, [eax]

    ; 【神聖切換】取出第 3 個參數 next_cr3，跨越宇宙！
    mov eax, [esp + 48] 
    mov cr3, eax        

    popf
    popa
    ret

; ... 下面的 child_ret_stub 保持完全不變 ...
```

### 🛠️ Step 2：打造宇宙產生器 (`lib/paging.c`)

我們要寫一個函數，當有新的 App 要執行時，幫它複製一份「含有 Kernel 基礎設施，但 User 空間全空」的全新分頁目錄。

請打開 **`lib/paging.c`**，在檔案的最下方加入這段超酷的程式碼：
```c
#include "pmm.h" // 確保有 include pmm.h 才能分配實體分頁

// [新增] 建立一個全新的 Page Directory (多元宇宙)
uint32_t create_page_directory() {
    // 1. 申請一個實體分頁當作新的 Page Directory
    uint32_t pd_phys = pmm_alloc_page();

    // 2. 為了能寫入這個實體位址，我們先把它臨時映射到虛擬記憶體的空地 (0xBFFFF000)
    map_page(0xBFFFF000, pd_phys, 3);
    uint32_t* new_pd = (uint32_t*) 0xBFFFF000;

    // 3. 複製 Kernel 原本的基礎建設 (0-4MB 核心區, 3GB Heap 區)
    // 這樣不管切換到哪個宇宙，OS 核心的程式碼都還在！
    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    // 4. 申請一個專屬於這個應用程式的 Page Table (負責管理 0x08000000 區段)
    uint32_t pt_phys = pmm_alloc_page();
    map_page(0xBFFFE000, pt_phys, 3);
    uint32_t* new_pt = (uint32_t*) 0xBFFFE000;

    // 將這張新表清空 (沒有任何記憶體)
    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
    }

    // 5. 將這張新表掛載到新宇宙的第 32 個目錄項 (對應 0x08000000 ~ 0x083FFFFF)
    // 7 = Present | Read/Write | User Mode
    new_pd[32] = pt_phys | 7;

    // 6. 刷新 TLB，確保我們的臨時映射即時生效
    __asm__ volatile("invlpg (%0)" ::"r" (0xBFFFF000) : "memory");
    __asm__ volatile("invlpg (%0)" ::"r" (0xBFFFE000) : "memory");

    // 回傳新宇宙的實體位址，未來準備給 CR3 暫存器使用！
    return pd_phys; 
}
```

### 🛠️ Step 3：接上管線 (`lib/task.c`)

最後，我們把這些基礎建設和排程器連上。打開 **`lib/task.c`**：

1. **宣告全域變數** (在檔案上方 `idle_task` 附近加入)：
```c
extern uint32_t page_directory[]; // 取得 Kernel 原始的分頁目錄
```

2. **更新 `switch_task` 宣告**：
```c
// 加上第三個參數 cr3
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3); 
```

3. **在 `init_multitasking()` 初始化 CR3**：
```c
    // ... 在 main_task 配置處加入：
    main_task->page_directory = (uint32_t) page_directory; // 住在原始宇宙

    // ... 在 idle_task 配置處加入：
    idle_task->page_directory = (uint32_t) page_directory; // 住在原始宇宙
```

4. **同樣在 `create_user_task` 和 `sys_fork` 裡面，暫時讓大家住在原始宇宙** (我們明天再來搬家)：
```c
    // 在這兩個函式裡建立 new_task 或 child 時，加入這行：
    new_task->page_directory = current_task->page_directory; // (sys_fork 裡是 child->page_directory)
```

5. **最後，修改 `schedule()` 的切換呼叫**：
```c
    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // 【關鍵】把新任務專屬的宇宙 CR3 傳遞給組合語言！
        switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
    }
```

---

完成這 3 步後，請執行 `make clean && make run`。

如果一切順利，你的系統畫面應該會**「跟剛才一模一樣，沒有任何改變」**。
不要失望！這在 OS 開發中叫做 "Non-breaking Refactoring" (無痛重構)，這代表你的 CR3 切換器已經完美融入了硬體中斷，而且系統在每次 Tick 切換任務時，都在順利地重載 Page Directory！

只要這個基礎不崩潰，我們明天就能放心把 `create_page_directory()` 注入到 `sys_exec` 當中，然後把 Makefile 裡的 `echo` 和 `cat` 全部改回 `0x08048000`，徹底完成作業系統的「記憶體隔離大業」！

測試看看系統是否還活著吧！😎
