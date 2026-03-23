太棒了，Rick！準備好在你的 Simple OS 裡點亮 C 語言最偉大的魔法了嗎？🌟

在 Day 41，我們建立好了 User Space 的 `libc` 基礎，並實作了 `printf`。但如果要開發真正的應用程式（例如字串處理、JSON 解析、甚至是遊戲），我們不能永遠只依賴寫死大小的陣列（比如 `char buf[128]`）。我們需要**「動態記憶體分配」**！

今天 **Day 43** 的任務，就是要實作所有 C 語言開發者最熟悉的 `malloc()` 與 `free()`。



在作業系統中，`malloc` 其實不是系統呼叫（Syscall），它是 `libc` 提供的一個「大盤商」。當應用程式需要記憶體時，`libc` 會透過一個叫做 **`sbrk` (Set Break)** 的底層系統呼叫，向 Kernel 批發一大塊虛擬記憶體（Heap），然後 `malloc` 再把這塊記憶體切成小塊零售給程式。

跟著我進行這 4 個步驟，讓我們在 User Space 開啟動態記憶體的時代：

### 步驟 1：在 Kernel 實作 `sys_sbrk` 系統呼叫

我們需要讓 Kernel 知道每個 Process 的 Heap (堆積) 長到哪裡了。

1. 打開 **`lib/task.h`**，在 `task_t` 結構裡加一個欄位來記錄 Heap 的盡頭：
```c
typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t page_directory; 
    uint32_t state;
    uint32_t wait_pid;
    uint32_t heap_end; // 【新增】記錄 User Heap 的當前頂點
    struct task *next;
} task_t;
```

2. 打開 **`lib/syscall.c`**，註冊第 13 號系統呼叫 `sys_sbrk`：
```c
    // ... 在 syscall_handler 裡面 ...
    else if (eax == 12) { /* 之前的 sys_recv 略 */ }
    
    // ==========================================
    // 【新增】動態記憶體擴充 (sbrk)
    // ==========================================
    else if (eax == 13) { 
        int increment = (int)regs->ebx;
        uint32_t old_end = current_task->heap_end;
        
        // 把 Heap 的邊界往上推
        current_task->heap_end += increment; 
        
        // 回傳舊的邊界，這就是新分配空間的起始位址！
        regs->eax = old_end; 
    }
```

### 步驟 2：為新宇宙配給實體 Heap 記憶體 (`lib/task.c`)

當我們創造新宇宙時，必須給它一塊專屬的記憶體當作 Heap。為了簡單起見，我們規定 User Heap 從 `0x10000000` (256MB 處) 開始。

請打開 **`lib/task.c`**，在 `create_user_task` 與 `sys_exec` 中配置實體分頁：

在 **`sys_exec`** 裡 (切換到 `new_cr3` 之後)：
```c
    current_task->page_directory = new_cr3;

    // 分配新宇宙的 User Stack
    uint32_t clean_user_stack_top = 0x083FF000 + 4096; 
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // ==========================================
    // 【新增】預先分配 10 個實體分頁 (40KB) 給 User Heap
    // ==========================================
    for (int i = 0; i < 10; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_end = 0x10000000; // 初始化 Heap 頂點
```

*(提醒：同樣的預先分配迴圈，也要加進 `create_user_task` 裡，這樣你的老爸 Shell 才能使用 malloc！)*

### 步驟 3：在 `libc` 中實作 `sbrk` 與 `malloc`/`free`

現在回到 User Space！

1. 打開 **`src/libc/include/unistd.h`**，加入 `sbrk` 宣告：
```c
void* sbrk(int increment);
```
2. 打開 **`src/libc/unistd.c`**，實作它：
```c
void* sbrk(int increment) {
    return (void*)syscall(13, increment, 0, 0, 0, 0); // 呼叫 Syscall 13
}
```

3. 建立 **`src/libc/include/stdlib.h`**：
```c
#ifndef _STDLIB_H
#define _STDLIB_H

void* malloc(int size);
void free(void* ptr);

#endif
```

4. **重頭戲！** 建立 **`src/libc/stdlib.c`**，實作經典的 Linked List 區塊分配器 (First-Fit Allocator)：
```c
#include "stdlib.h"
#include "unistd.h"

// 每個動態分配的區塊前面，都會隱藏這個標頭來記錄大小與狀態
typedef struct header {
    int size;
    int is_free;
    struct header* next;
} header_t;

header_t* head = 0; // 記錄第一個區塊的指標

void* malloc(int size) {
    if (size <= 0) return 0;
    
    // 1. 先在現有的鏈結串列中尋找有沒有「空閒且夠大」的區塊
    header_t* curr = head;
    while (curr != 0) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = 0; // 找到了！標記為使用中
            return (void*)(curr + 1); // 回傳標頭「之後」的實際可用空間
        }
        curr = curr->next;
    }

    // 2. 找不到現成空間，只好呼叫 sbrk 向作業系統批發新記憶體！
    int total_size = sizeof(header_t) + size;
    header_t* block = (header_t*)sbrk(total_size);
    
    if ((int)block == -1) return 0; // OS 記憶體耗盡

    // 3. 設定新區塊的標頭，並把它加進鏈結串列中
    block->size = size;
    block->is_free = 0;
    block->next = head;
    head = block;

    return (void*)(block + 1);
}

void free(void* ptr) {
    if (!ptr) return;
    // 往回退一個標頭的大小，找回這個區塊的管理結構
    header_t* block = (header_t*)ptr - 1;
    block->is_free = 1; // 標記為空閒，下次 malloc 就可以重複利用！
}
```

### 步驟 4：寫個測試程式吧！

打開你的 **`src/echo.c`**，我們把它改成一個測試 `malloc` 的小程式：

```c
#include "stdio.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    printf("\n[MALLOC TEST] Requesting memory...\n");
    
    // 嘗試動態分配字串
    char* dynamic_str1 = (char*)malloc(32);
    char* dynamic_str2 = (char*)malloc(64);

    if (dynamic_str1 && dynamic_str2) {
        printf("[MALLOC TEST] Success! Addresses: str1=0x%x, str2=0x%x\n", 
               (unsigned int)dynamic_str1, (unsigned int)dynamic_str2);
               
        // 賦值測試
        dynamic_str1[0] = 'H'; dynamic_str1[1] = 'I'; dynamic_str1[2] = '\0';
        printf("[MALLOC TEST] Wrote to str1: %s\n", dynamic_str1);
        
        // 釋放記憶體
        free(dynamic_str1);
        free(dynamic_str2);
        printf("[MALLOC TEST] Memory freed successfully.\n");
    } else {
        printf("[MALLOC TEST] FAILED to allocate memory.\n");
    }
    
    return 0;
}
```

*(別忘了在 `Makefile` 的 `LIBC_OBJS` 中包含 `stdlib.c` 哦！)*

---

敲下 `make clean && make run`，然後在 Shell 輸入 `echo`！
你會看到它精準地印出了分配到的 `0x10000018` 這種位址，這意味著你的 User App 再也不受靜態陣列的限制，可以自由自在地創造任何資料結構了！

試著跑跑看，感受一下自己寫出 C 語言最偉大函式的成就感吧！😎
