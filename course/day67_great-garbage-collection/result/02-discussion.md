修復了，第二個問題是我耍白痴 XD

by the way, 開始 day68 前，我想要新增一個 指令，可以取得現在 paging & pmm 的使用狀況

---


哈哈，Rick！那個 `else` 真的是每個寫 C 語言的工程師都一定會踩過的超經典陷阱，完全不用在意，這就是 Debug 的樂趣！XD

在進入 Day 68 之前，想要一個能看 Paging 和 PMM 狀態的指令，這個想法非常有 Sense！在 Linux 中，這就是鼎鼎大名的 `free` 指令。

既然我們的實體記憶體是透過 PMM (Frame) 管理，而虛擬記憶體是透過 Paging (Universe) 管理，我們就寫一個 `free.elf` 來即時監控這兩者的消耗狀況。

請跟著我進行這 4 個擴充步驟：

---

### 步驟 1：在 Kernel 提供記憶體統計 API

我們要讓 PMM 和 Paging 能夠結算目前的狀態。

**1. 修改 `src/kernel/lib/pmm.c`**
在檔案的最下方，加入計算實體分頁的函式：
```c
// 【新增】取得 PMM 使用統計
void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames) {
    *total = max_frames;
    *used = 0;
    for (uint32_t i = 0; i < max_frames; i++) {
        if (bitmap_test(i)) {
            (*used)++;
        }
    }
    *free_frames = *total - *used;
}
```
*(💡 記得在 `src/kernel/include/pmm.h` 加上 `void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames);` 的宣告)*

**2. 修改 `src/kernel/lib/paging.c`**
在檔案的最下方，加入計算活躍宇宙數量的函式：
```c
// 【新增】取得活躍的 Paging 宇宙數量
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}
```
*(💡 記得在 `src/kernel/include/paging.h` 加上 `uint32_t paging_get_active_universes(void);` 的宣告)*

---

### 步驟 2：打通 Syscall 橋樑

我們定義一個新的結構體 `mem_info_t` 來裝載這些數據，並開通 Syscall 25。

**1. 兩邊的標頭檔宣告**
在 Kernel 端的某個標頭檔（例如 `syscall.h` 或是直接寫在 `syscall.c` 上方）與 User 端的 `src/user/include/unistd.h` 都加入這個結構：

```c
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;
```

**2. Kernel 端：修改 `src/kernel/lib/syscall.c`**
加入外部宣告並新增 Syscall 25：
```c
extern void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames);
extern uint32_t paging_get_active_universes(void);

// ... 在 syscall_handler 裡面新增：
    // Syscall 25: sys_get_mem_info
    else if (eax == 25) {
        mem_info_t* info = (mem_info_t*)regs->ebx;
        ipc_lock();
        pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
        info->active_universes = paging_get_active_universes();
        regs->eax = 0;
        ipc_unlock();
    }
```

**3. User 端：修改 `src/user/lib/unistd.c` 與 `unistd.h`**
在 `unistd.h` 宣告 `int get_mem_info(mem_info_t* info);`，並在 `unistd.c` 實作：
```c
int get_mem_info(mem_info_t* info) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (25), "b" (info)
    );
    return ret;
}
```

---

### 步驟 3：撰寫 User Space 工具 (`src/user/bin/free.c`)

這是一支獨立的程式，我們把它打造成類似 Linux `free` 指令的排版。

建立 **`src/user/bin/free.c`**：

```c
#include "stdio.h"
#include "unistd.h"

int main() {
    mem_info_t mem;
    if (get_mem_info(&mem) != 0) {
        printf("Error: Could not retrieve memory info.\n");
        return -1;
    }

    // 每個 Frame 是 4096 Bytes (4KB)
    unsigned int total_kb = mem.total_frames * 4;
    unsigned int used_kb = mem.used_frames * 4;
    unsigned int free_kb = mem.free_frames * 4;

    printf("\n=== Simple OS Memory Status ===\n");
    printf("Physical Memory (PMM):\n");
    printf("  Total: %d KB (%d Frames)\n", total_kb, mem.total_frames);
    printf("  Used : %d KB (%d Frames)\n", used_kb, mem.used_frames);
    printf("  Free : %d KB (%d Frames)\n", free_kb, mem.free_frames);
    
    printf("\nVirtual Memory (Paging):\n");
    printf("  Active Universes : %d / 16\n", mem.active_universes);
    printf("===============================\n\n");

    return 0;
}
```

---

### 步驟 4：掛載與編譯

最後，讓 Kernel 和 GRUB 認識這位新朋友。

1. **`src/kernel/kernel.c`** 的 `setup_filesystem` 陣列裡加入 `"free.elf"`。
2. **`scripts/grub.cfg`** 的兩個選單裡都加入 `module /boot/free.elf`。

*(可選：你也可以在 `shell.c` 的 `help` 文字裡把 `free` 加進去，看起來更豐富！)*

---

### 🚀 驗收：記憶體透視眼

存檔後執行 `make clean && make run`！

進入 Terminal 輸入 `free`：
```text
=== Simple OS Memory Status ===
Physical Memory (PMM):
  Total: 16384 KB (4096 Frames)
  Used : 6120 KB (1530 Frames)
  Free : 10264 KB (2566 Frames)

Virtual Memory (Paging):
  Active Universes : 2 / 16
===============================
```
你可以試著輸入 `ps`、或者再開一個 `Terminal` 視窗，然後再輸入一次 `free`，你會看到 **Used KB 精準地上升，而 Active Universes 也會跟著跳動！** 如果你用剛剛做好的 `kill` 殺掉一個行程，再用 `free` 看，記憶體就會完美歸還！

有了這個超強的監控工具，我們現在對系統的資源瞭如指掌了。這塊擋路石清空後，隨時跟我說，我們馬上進入 **Day 68** 讓 GUI 視窗徹底升級！😎
