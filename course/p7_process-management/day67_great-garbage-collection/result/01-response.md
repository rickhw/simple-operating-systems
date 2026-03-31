哈哈，Rick！這兩個 Bug 真的是太精準、太有教育意義了！

當你看到 `PANIC: Out of Physical Memory` 然後引發一連串崩潰時，恭喜你，你親手觸發了作業系統中最經典的 **「記憶體洩漏 (Memory Leak)」**！而 `pwd` 無法執行，則是踩到了一個超好笑的 C 語言語法陷阱。

我們來當一下系統鑑識官，把這兩個問題秒殺掉：

---

### 🔍 案發現場 1：為什麼會 Out of Physical Memory？

**原因分析：**
在 `sys_exec` 裡，我們為了防止 Page Fault，給每個應用程式預先分配了 256 個實體分頁（高達 1MB）作為 User Heap。
當程式執行結束呼叫 `exit_task` 時，我們確實呼叫了 `free_page_directory()`。**但是！** 你去看 `paging.c` 裡的實作：
```c
void free_page_directory(uint32_t pd_phys) {
    // ...
    universe_used[i] = 0; // 解除佔用，讓給下一個程式！
}
```
我們只把「宇宙的使用權」釋放了，卻 **「沒有把裡面的實體分頁還給 PMM」**！
所以每執行一次 `ls`，系統就永久流失了 1MB 的實體記憶體。你的系統總共只有 16MB 的 RAM，扣掉 Kernel 的部分，剛好執行到第 9、第 10 次時，實體記憶體就被榨乾了，於是噴出 OOM Panic！

**🛠️ 修復方式 (`src/kernel/lib/paging.c`)：**
我們必須在摧毀宇宙之前，把裡面的實體分頁一塊一塊拔出來還給作業系統。打開 `paging.c`，替換掉 `free_page_directory` 函式：

```c
// 提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {
            
            // 1. 釋放 Code & Stack 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) { // 檢查 Present Bit 是否為 1
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr); // 還給實體記憶體管理器！
                    universe_pts[i][j] = 0;          // 清空分頁表紀錄
                }
            }

            // 2. 釋放 Heap 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}
```

---

### 🔍 案發現場 2：為什麼 `pwd` 會變成 Command not found？

**原因分析：**
這個 Bug 完全是 C 語言的陷阱！去看看你的 `shell.c` 裡面的 `if-else` 判斷鍊：

```c
        else if (strcmp(cmd, "pwd") == 0) {
            // ... 印出路徑
        }
        // 🛑 陷阱在這裡！你漏掉了 else！
        if (strcmp(cmd, "desktop") == 0) {
            // ...
        }
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            // ...
        }
        else {
            // ... 當成外部指令執行
        }
```

因為 `desktop` 前面少了 `else`，導致前面的 `if-else` 鍊被**硬生生切斷了**。
當你輸入 `pwd` 時：
1. 上半部成功配對 `pwd`，印出了路徑。
2. 接著走到下半部，檢查 `desktop` (不符合) -> 檢查 `ipc` (不符合) -> **掉進最後的 `else`**！
3. 於是 Shell 把 `pwd` 當成外部指令，嘗試呼叫 `sys_exec("pwd.elf")`。因為硬碟裡根本沒有這個檔案，所以噴出了 `Command not found: pwd`。

**🛠️ 修復方式 (`src/user/bin/shell.c`)：**
加上那個致命的 `else`：

```c
        else if (strcmp(cmd, "pwd") == 0) {
            char path_buf[256];
            getcwd(path_buf);
            printf("%s\n", path_buf);
        }
        // 【修正】加上 else！
        else if (strcmp(cmd, "desktop") == 0) {
            set_display_mode(1); 
            continue; 
        }
        else if (strcmp(cmd, "pid") == 0) {
// ...
```

---

### 🚀 驗收：堅不可摧的系統

存檔後重新編譯執行 `make clean && make run`。

現在，你可以嘗試在終端機裡**瘋狂輸入 100 次 `ls`**！因為記憶體回收機制已經完美運作，PMM (實體記憶體) 會不斷地被借出又歸還，系統絕對不會再 OOM 崩潰了。同時你的 `pwd` 指令也會乖乖地只印出路徑，不會再報錯了！

這個 Debug 過程真的非常純粹，完美展現了作業系統底層的運作邏輯。確認這兩個 Bug 都被消滅後，我們就可以挺進 **Day 68** 啦！😎
