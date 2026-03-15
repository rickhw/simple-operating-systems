
這張 `PANIC: Kernel Heap Out of Memory!` 的截圖實在太寫實了！恭喜你，你遇到了一個所有作業系統開發者都絕對會踩到的坑——**核心記憶體不足**。

我們來當一下系統法醫，看看為什麼會崩潰：
你看畫面上的 `my_app.elf (Size: 9268 bytes)`，這個檔案大約是 **9 KB**。
但在我們 Day 15 實作 `init_kheap()`（核心堆積初始化）的時候，通常為了測試，我們可能只向 PMM 申請了 **1 個實體分頁（4096 bytes = 4 KB）**，並把它映射到虛擬位址 `0xC0000000` 當作 Heap 來用。

現在，Kernel 試圖呼叫 `kmalloc(9268)` 來裝載這個 ELF 檔。因為 9 KB 大於 4 KB，Heap 管理器發現空間不夠，就盡責地拋出了 `PANIC`！

### 🛠️ 終極修復：擴建 Kernel Heap 的土地

解決方法非常簡單，我們只需要在核心啟動時，多批發幾塊「實體記憶體」給 Kernel Heap 即可。

請打開你的 **`lib/kheap.c`**，找到 `init_kheap()` 這個函式。
原本的寫法可能只申請了一頁，請把它改成一個**迴圈**，一次申請 16 頁（64 KB）或更多的空間：

```c
void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // [修改這裡] 使用迴圈，向 PMM 連續申請 16 個分頁 (共 64 KB)
    for (int i = 0; i < 16; i++) {
        uint32_t phys_addr = pmm_alloc_page();
        
        // 將實體頁框對應到 0xC0000000 之後的連續虛擬位址 (權限 3 代表 Kernel RW)
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // --- 下面是你原本的 Heap Header 初始化邏輯 ---
    // 注意：如果你原本有變數記錄 Heap 的總大小，記得把它改成 16 * 4096
    // 例如：
    // block_header_t* first_block = (block_header_t*) 0xC0000000;
    // first_block->size = (16 * 4096) - sizeof(block_header_t); 
    // first_block->is_free = 1;
    // first_block->next = 0;
}

```

*(註：請根據你在 Day 15 實際寫的 `kheap.c` 變數名稱稍微調整，核心概念就是把申請的 `4096` 放大成 `16 * 4096` 或 `32 * 4096`)*

---

### 重新出發

修改完並存檔後，再次使出你的魔法：

```bash
make clean && make run

```

這次你的 Kernel 就擁有 64 KB 的動態記憶體了，要裝下 9 KB 的 `my_app.elf` 絕對是游刃有餘！

如果順利跑出我們在 Day 28 看到的 User App 輸出畫面，那就代表你真的成功完成了**「從實體硬碟讀取執行檔，並動態分配記憶體執行」**的壯舉！這絕對是作業系統開發的最高潮之一！

成功之後跟我說，你想在 Day 30 挑戰寫一個簡單的 Shell（命令列介面）來自由敲指令執行程式，還是想了解鍵盤輸入的整合呢？
