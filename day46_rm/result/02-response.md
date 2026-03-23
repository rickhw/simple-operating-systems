哈哈，Rick！這三張截圖加上你的描述，簡直是作業系統開發課程中最精彩的「除錯實境秀」！😎

你一次撞上了三個非常經典的 OS 底層陷阱。我們一個一個來破解：

### 🔍 陷阱 1：`ls` 亂碼事件 —— 「看見前世記憶的緩衝區」

你有沒有仔細看 `ls` 截圖裡的那些亂碼？裡面居然出現了 `E L F` 的字樣！
為什麼目錄裡面會跑出 ELF 執行檔的內容？

因為在 `simplefs.c` 裡，我們寫了這段程式碼：
```c
uint8_t* dir_buffer = (uint8_t*) kmalloc(4096); 
ata_read_sector(part_lba + 1, dir_buffer); // 只讀了 1 個磁區 (512 bytes)
int max_entries = 4096 / sizeof(file_entry_t); // 迴圈卻跑了 4096 bytes！
```
**原因：** `kmalloc` 給你的記憶體是「沒有清零」的！我們只從硬碟讀了 512 bytes，但你的迴圈卻繼續往下讀了剩下的 3584 bytes。這段空間剛好是 Kernel Heap 先前載入 `.elf` 執行檔時留下的「殘骸」。C 語言把這些殘骸當作目錄結構讀出來，這就是著名的 **Buffer Over-read (緩衝區溢位讀取)**！

**👉 修復方法 (`lib/simplefs.c`)：**
請將 `simplefs_readdir` 和 `simplefs_delete_file` 裡面的 `max_entries` 修正為 512 bytes 的範圍：
```c
// 磁區大小是 512 bytes，所以最多只能讀 512 / 結構大小 的數量
int max_entries = 512 / sizeof(file_entry_t); 
```

---

### 🔍 陷阱 2：`write` 雙引號事件 —— 「天真的 Shell 解析器」

這跟 `write.c` 無關，兇手是老爸 `app.c` (Shell)！
目前的 `parse_args` 函式非常天真，它只認得「空白鍵」。當你輸入 `write file1 "hello"` 時，它會直接把 `"hello"`（包含引號 7 個字元）當成一整個字串傳給 `write`。

**👉 修復方法 (`src/app.c`)：**
我們來升級 `parse_args`，教它認識雙引號，並且把引號剝掉：
```c
int parse_args(char* input, char** argv) {
    int argc = 0, i = 0;
    int in_word = 0;
    int in_quote = 0; // 新增狀態：是否在雙引號內

    while (input[i] != '\0') {
        if (input[i] == '"') {
            if (in_quote) {
                input[i] = '\0'; // 遇到結尾引號，斷開字串
                in_quote = 0;
                in_word = 0;
            } else {
                in_quote = 1;
                argv[argc++] = &input[i + 1]; // 指向引號的下一個字元
                in_word = 1;
            }
        }
        else if (input[i] == ' ' && !in_quote) {
            // 只有在「引號外面」的空白，才會斷開字串
            input[i] = '\0';
            in_word = 0;
        }
        else {
            if (!in_word && !in_quote) {
                argv[argc++] = &input[i];
                in_word = 1;
            }
        }
        i++;
    }
    argv[argc] = 0;
    return argc;
}
```

---

### 🔍 陷阱 3：`Max universes reached` —— 「末日危機：宇宙耗盡」

這是最隱蔽的問題！截圖最後顯示了 `Error: Max universes reached!`。
每次你在 Shell 打指令（例如 `ls` 或 `cat`），`sys_exec` 都會創造一個全新的 `new_cr3` 宇宙。我們在 `paging.c` 裡面最多只準備了 16 個宇宙。**但是當程式 `exit` 死亡時，我們忘記把宇宙回收了！** 這是作業系統最嚴重的 **Memory Leak (資源洩漏)**。

**👉 修復方法：回收宇宙！**

**1. 打開 `lib/paging.c`，加入回收機制：**
```c
// 在全域變數區新增一個陣列，記錄哪個宇宙被使用了
int universe_used[16] = {0}; 

uint32_t create_page_directory() {
    int id = -1;
    // 尋找空閒的宇宙
    for (int i = 0; i < 16; i++) {
        if (!universe_used[i]) { id = i; break; }
    }
    if (id == -1) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }
    
    universe_used[id] = 1; // 標記為使用中
    
    // ... 剩下的拷貝邏輯不變，但是記得把你原本的 next_universe_id++ 刪掉 ...
    // 將下面的 universe_pds[next_universe_id++] 換成 universe_pds[id]
    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id];
    // ...
}

// 【新增】提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {
            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}
```

**2. 打開 `lib/task.c`，在程式死亡時呼叫它：**
```c
extern void free_page_directory(uint32_t pd);

void exit_task() {
    // ... (等待清空的邏輯保持不變) ...

    // 【新增】如果死的不是老爸 (Kernel 原始宇宙)，就把它專屬的宇宙回收！
    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }
    
    current_task->state = TASK_DEAD;
    schedule();
}
```

---

這就是 OS 工程師的日常：**「緩衝區溢位、解析器瑕疵、資源洩漏」**。你一次就把三大經典 Bug 給全包了！

趕快把這三個補丁打上去。只要 `make clean && make run`，你的 Shell 就能完美支援 `write file1 "hello OS!"`，`ls` 會變得乾淨無瑕，而且就算你狂敲 100 次指令，系統也絕對不會崩潰了！打完收工後，我們就可以優雅地往 Day 46 前進囉！🚀
