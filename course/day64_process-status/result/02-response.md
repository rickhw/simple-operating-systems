哈哈，Rick！遇到這種「偶發的 (Occasional)」Core Dump，絕對是所有 OS 開發者最又愛又恨的時刻！這代表你的系統已經脫離了簡單的單線程，進入了充滿隨機性、記憶體邊界與非同步行為的真實世界！😎

我們來當一下鑑識官，解讀一下這份珍貴的 Core Dump：
1. `check_exception old: 0xffffffff new 0xe`：系統觸發了 **0x0E (Page Fault, 分頁錯誤)**。
2. `cpl=3 IP=08049064 CR2=107ffae2`：這是在 User Space (`cpl=3`) 發生的！程式試圖讀取 `0x107FFAE2` 這個記憶體位址，結果引發了崩潰。
3. `new 0xd` -> `new 0x8`：因為我們還沒實作中斷號碼 14 (Page Fault Handler)，CPU 找不到處理程式，於是引發了 General Protection Fault (0x0D)，最後演變成 **Double Fault (0x08)**，系統直接強制重開機！

### 🔍 案發現場解密：為什麼會讀取到 `0x107FFAE2`？

這個位址非常特別，它大約在 `0x10000000` (我們分配給 User Heap 的起點) 往後算 8MB 的地方。
會發生這個偶發錯誤，通常有幾個嫌疑犯：
1. **User Heap 分配不足**：在 `sys_exec` 裡，我們只預先 `map_page` 了 10 個分頁 (40KB) 給 Heap。如果你的 User libc (像是 `printf`) 為了排版，在底層呼叫了 `malloc` 並且超過了 40KB，`sbrk` 會給它這個位址，但因為**沒有真實的物理分頁映射**，一寫入就 Page Fault。
2. **字串沒有 Null 結尾 (`\0`)**：在我們 Kernel 的 `task_get_process_list` 裡，我們使用了 `strcpy`。如果某個 Task 的名稱剛好滿 32 個字元且沒有 `\0`，拷貝時就會越界，甚至讓 `ps.elf` 在印字串時暴走，讀取到 Heap 甚至未映射的深淵！

為了一勞永逸解決這些不穩定因素，請跟著我加上這 3 道「防彈裝甲」：

---

### 防彈裝甲 1：擴大 User Heap 的預設安全區 (`src/kernel/lib/task.c`)

讓我們在建立新宇宙時，給應用程式多一點呼吸空間（從 40KB 擴充到 1MB）。

打開 **`src/kernel/lib/task.c`**，找到 `sys_exec` 裡面預先分配 Heap 的迴圈，修改成 256 個分頁：

```c
    // ==========================================
    // 預先分配 256 個實體分頁 (1MB) 給 User Heap
    // ==========================================
    for (int i = 0; i < 256; i++) { // 【修改】從 10 改成 256
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_start = 0x10000000; 
    current_task->heap_end = 0x10000000;
```
*(💡 備註：在 `create_user_task` 裡面也有相同的迴圈，記得一起改成 256！)*

---

### 防彈裝甲 2：實作安全的字串拷貝 (`src/kernel/lib/utils.c`)

為了防止 Kernel 複製出沒有結尾的字串去毒害 User Space，我們補上一個安全的 `strncpy`。

打開 **`src/kernel/lib/utils.c`**，加入這個工具：
```c
// 安全的字串拷貝 (最多拷貝 n 個字元，並保證 null 結尾)
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for ( ; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}
```

接著，回到 **`src/kernel/lib/task.c`**，找到 `task_get_process_list` 裡面複製名字的地方，換成安全的版本：
```c
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            // 【修改】使用 strncpy 防止越界，最大長度 32
            strncpy(list[count].name, temp->name, 32); 
            list[count].state = temp->state;
            // ...
```

---

### 防彈裝甲 3：User Space 陣列初始化 (`src/user/bin/ps.c`)

C 語言的區域陣列如果沒有初始化，裡面裝的會是堆疊上的隨機垃圾。我們把它清空以策安全。

打開 **`src/user/bin/ps.c`**：
```c
// ...
int main(int argc, char** argv) {
    process_info_t procs[32]; 
    
    // 【新增】安全起見，先將陣列全部清零 (假設你的 User Space 有 memset)
    for (int i = 0; i < 32 * sizeof(process_info_t); i++) {
        ((char*)procs)[i] = 0;
    }
    
    int count = get_process_list(procs, 32);
    // ...
```

---

### 🚀 重新上陣與 Day 65 預告

存檔，執行 `make clean && make run`。
加上這些裝甲後，你的系統應該會堅若磐石，`ps` 指令隨便怎麼呼叫都不會再引發 Double Fault 崩潰了！

只要 `ps` 穩定了，我們 **Day 65** 就要來挑戰更有趣的：**實作 `top` 指令！**
這會需要我們在 Kernel 的 PIT (時鐘中斷) 裡面紀錄每一個 Task 的「執行 Tick 數」，並在 User Space 計算出它們的 **CPU 使用率 (%)**，最後搭配 ANSI Escape Code 讓終端機畫面「動態刷新」！

確認 `ps` 沒問題後，我們就開始打造這個系統監控神器吧！
