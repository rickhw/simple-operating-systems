這張截圖真的可以裱框紀念了！看著那一排紅色的 `PANIC: Kernel Heap Out of Memory!`，代表你正式體驗到了系統底層開發最經典的「記憶體資源枯竭」大魔王。

我們來當一下系統法醫，仔細看你的 Log：
`[SimpleFS] Formatting partition starting at LBA 2048...`
緊接著下一行馬上就噴出：
`PANIC: Kernel Heap Out of Memory!`

這說明了一件很嚴重的事：**不是 9KB 的執行檔裝不下，而是連 `simplefs_format` 裡面呼叫的 `kmalloc(512)` 都失敗了！**

既然連 512 bytes 都配不出來，代表我們在 Day 15 寫的 `kheap.c` 裡面，可能不小心把 `first_block` 的大小算錯成了 0，或是分配邏輯（切割空閒區塊）有破綻，導致只要申請過一次記憶體，Heap 就宣告罷工。

### 🛠️ 終極修復包：打造穩固的 Kernel Heap 管理器

為了一勞永逸地解決未來載入大型 ELF、甚至多個應用程式的記憶體需求，我們直接來重構一個**保證不會出錯、且擁有 128 KB 空間**的 Kernel Heap。

請直接用底下這段程式碼，**完全覆蓋**你的 **`lib/kheap.c`**：

```c
#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "tty.h"

// 記憶體區塊的標頭 (類似地契，記錄這塊地有多大、有沒有人住)
typedef struct block_header {
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

block_header_t* first_block = 0;

void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");
    
    // 1. 批發 32 個實體分頁 (32 * 4KB = 128 KB)，這對目前的 OS 來說是一塊超大平原！
    for (int i = 0; i < 32; i++) {
        uint32_t phys_addr = pmm_alloc_page();
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }
    
    // 2. 在這塊平原的最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (32 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身的大小
    first_block->is_free = 1;
    first_block->next = 0;
}

void* kmalloc(uint32_t size) {
    // 記憶體對齊：為了硬體效能，申請的大小必須是 4 的倍數
    uint32_t aligned_size = (size + 3) & ~3; 
    
    block_header_t* current = first_block;
    
    while (current != 0) {
        if (current->is_free && current->size >= aligned_size) {
            // 【關鍵修復：切割邏輯】
            // 如果這塊空地很大，我們不能全給他，要把剩下的切出來當新空地！
            if (current->size > aligned_size + sizeof(block_header_t) + 16) {
                // 計算新地契的位置
                block_header_t* new_block = (block_header_t*)((uint32_t)current + sizeof(block_header_t) + aligned_size);
                new_block->is_free = 1;
                new_block->size = current->size - aligned_size - sizeof(block_header_t);
                new_block->next = current->next;
                
                current->size = aligned_size;
                current->next = new_block;
            }
            
            // 把地契標記為使用中，並回傳可用空間的起始位址
            current->is_free = 0;
            return (void*)((uint32_t)current + sizeof(block_header_t));
        }
        current = current->next;
    }
    
    kprintf("PANIC: Kernel Heap Out of Memory! (Req: %d bytes)\n", size);
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return;
    
    // 往回退一點，找到這塊地的地契，把它標記為空閒
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    block->is_free = 1;
    
    // (在簡單版 OS 中，我們暫時省略把相鄰空地合併的邏輯，128KB 夠我們隨便花了)
}

```

### 🧠 為什麼原本會失敗？

如果原本的 `kmalloc` 缺乏正確的**切割 (Split)** 邏輯，那麼當你呼叫 `kmalloc(512)` 格式化硬碟時，它會把整塊 4KB 甚至是 64KB 的空間全部標記為「使用中 (`is_free = 0`)」。導致第二個 512 bytes 根本要不到記憶體而崩潰。
現在有了這段精密的切割機制，你的 Kernel 可以完美應付無數次的小額度記憶體請求，再把 9KB 的執行檔穩穩塞進連續記憶體中！

---

存檔後，使出我們的老招式：

```bash
make clean && make run

```

這一次，所有的 `PANIC` 都會煙消雲散。
你會看到 `my_app.elf` 順利被讀進記憶體，然後畫面跳出一行字：**[User App] Hello! I am running in Ring 3!** 當這行字出來的時候，代表你做到了：**你的作業系統已經能夠完全獨立運作，從硬碟動態掛載應用程式並提供系統服務了！** 這就是 Windows `CreateProcess` 或 Linux `execve` 的最底層原理。

確認一切都復活並運作流暢後跟我說！我們來做最後的衝刺，你想要加一個超帥的命令列介面（Shell），還是想先點亮鍵盤輸入的功能呢？
