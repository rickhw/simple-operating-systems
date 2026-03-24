哈哈，Rick！看著這個滿滿的開機載入畫面，真的是太壯觀了！從 `shell.elf` 一路載入到 `mkdir.elf`，你的 Simple OS 已經從一個簡陋的玩具，變成擁有完整生態系的作業系統了！🚀

但也就是因為你的生態系變得太過龐大，我們終於撞上了資源的天花板：
**`PANIC: Kernel Heap Out of Memory! (Req: 164 bytes)`**

### 🔍 案發現場解密：被塞爆的 128KB 平原

在最一開始實作 `kmalloc` 的時候，我們在 `lib/kheap.c` 裡只跟系統「批發」了 32 個實體分頁（$32 \times 4096 = 128\text{ KB}$）當作 Kernel Heap。
當時我們覺得 128 KB 對於 OS 來說是一塊超大的平原。

**但是現在情況不同了！**
開機時，Kernel 的 `setup_filesystem` 會一口氣把 GRUB 傳來的 **9 個 ELF 執行檔** 全部安裝進硬碟。每一次建立檔案、每一次 `simplefs_find` 尋找檔案，都需要用到 `kmalloc` 來分配緩衝區和大小約為 164 bytes 的 `fs_node_t` 結構。

加上我們的 `kfree` 目前還是「極簡版」（只標記為 `is_free = 1`，沒有把相鄰的空地合併起來，導致記憶體碎片化嚴重）。因此，在這波瘋狂的開機載入潮中，128 KB 的 Kernel Heap 終於被徹底榨乾，導致要載入 `shell.elf` 時，連 164 bytes 的節點空間都生不出來了！

### 🛠️ 終極修復：擴建 Kernel Heap！

解決這個問題最快、最霸氣的方法，就是直接向 PMM 申請更多的實體記憶體！反正我們的虛擬記憶體空間高達 4GB，這點記憶體對我們來說根本不算什麼。

請打開 **`lib/kheap.c`**，我們把 Kernel Heap 從 128 KB 一口氣擴大到 **2 MB (512 個分頁)**！
順便幫你加上型別轉型，消除編譯器 `integer from pointer without a cast` 的警告：

```c
void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // 【升級】從 32 個分頁擴建到 512 個分頁 (512 * 4KB = 2 MB)！
    for (int i = 0; i < 512; i++) {
        // 加上 (uint32_t) 強制轉型，消除編譯警告
        uint32_t phys_addr = (uint32_t) pmm_alloc_page(); 
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // 在這塊巨大的平原最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (512 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身
    first_block->is_free = 1;
    first_block->next = 0;
}
```

---

### 💡 同場加映：隱藏的致命 Bug (編譯器警告的逆襲)

作為有鷹眼的架構師，我還注意到你的編譯 Log 裡面有一行非常危險的警告：
```text
lib/simplefs.c:207:74: warning: iteration 24 invokes undefined behavior
207 | for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
```

這代表在你的 `lib/simplefs.h` 裡面，你的 `file_entry_t` 結構中的名字可能還叫 `char filename[24]`（舊版的長度），但是你的迴圈卻硬要跑到 32！這會導致嚴重的記憶體溢位。

請務必檢查你的 **`lib/simplefs.h`**（或者你定義此結構的標頭檔），確保它已經精準更新為我們 Day 47 設計的 64 bytes 完美對齊版本：

```c
#define FS_FILE 0
#define FS_DIR  1

typedef struct {
    char name[32];         // 檔名必須剛好是 32 bytes！
    uint32_t size;
    uint32_t lba;
    uint32_t type;
    uint32_t reserved[5];  // 補滿 64 bytes
} file_entry_t;
```
*(如果結構名稱改為 `name`，記得要把 `simplefs.c` 裡面的 `entries[i].filename` 都替換成 `entries[i].name` 喔！)*

---

補上這兩個修正後，存檔並執行：
```bash
make clean && make run
```

這次，你的系統將擁有無比寬廣的記憶體平原，順利吞下所有的執行檔，並跳出熟悉的 `Welcome to Simple OS Shell!`。

成功後，記得試試看我們上一回合剛寫好的 `mkdir myfolder` 指令，然後用 `ls` 看看它是不是帥氣地以 `[DIR]` 的型態顯示出來！😎
