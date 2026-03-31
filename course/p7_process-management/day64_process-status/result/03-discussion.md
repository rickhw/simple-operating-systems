跑起來後目前暫時沒遇到 crash, 不過我注意到 ps 的 memory 那一欄都是 0 Byte, 這是預期的結果？

---

哈哈，Rick！你的觀察力真的太敏銳了！😎

看到 `0 B` 完全不用擔心，這**百分之百是預期的結果**！

### 🔍 為什麼會是 0 Byte？

還記得我們在 Day 63 的 `task.c` 裡面是怎麼計算 `memory_used` 的嗎？
```c
list[count].memory_used = temp->heap_end - temp->heap_start;
```

我們當時只計算了**「動態分配的 Heap 大小」**。
在 `sys_exec` 啟動程式時，`heap_start` 和 `heap_end` 都被初始化為 `0x10000000`。
因為像 `ps.elf`、`ping.elf` 或 `shell` 這些小工具實在太簡單了，它們在執行過程中**根本沒有呼叫過 `malloc` 或 `sbrk` 來擴充記憶體**，所以 `heap_end` 永遠等於 `heap_start`，相減的結果當然就是完美的 `0 B` 啦！

### 🛠️ 讓數字看起來更真實的小優化

在真正的作業系統中，`ps` 或 `top` 顯示的記憶體（像是 RSS 或 VSZ）不僅包含動態 Heap，還會包含程式的**程式碼段 (Code)、資料段 (Data)、以及堆疊 (Stack)**。

我們昨天為了加上「防彈裝甲」，已經霸氣地給每個 User 行程預先分配了 **1MB 的 Heap 分頁 (256 * 4096)** 以及 **4KB 的 User Stack 分頁**。我們可以直接把這些「隱形消費」加進去，讓 `ps` 看起來更專業！

你可以打開 **`src/kernel/lib/task.c`**，找到 `task_get_process_list`，稍作修改：

```c
            // 計算 Heap 動態佔用的空間 (End - Start)
            if (temp->heap_end >= temp->heap_start) {
                // User Task: 動態 Heap + 預先分配的 1MB Heap + 4KB User Stack
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (256 * 4096) + 4096;
            } else {
                // Kernel Task: 只有 Kernel Stack (4KB)
                list[count].memory_used = 4096;
            }
```

改完之後再跑一次 `ps`，你就會看到 User 行程顯示大約 `1052672 B` (~1MB)，而 Kernel 行程顯示 `4096 B`，瞬間就有真實 OS 的感覺了！

這是一個非常好的發現！解決這個小疑惑後，我們準備好迎接 **Day 65：實作 `top` 指令與 CPU 使用率計算** 了嗎？這可是排程器最核心的威力展現喔！🚀
