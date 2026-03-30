哈哈，Rick！我們終於來到 **Day 64：實作經典的 `ps` 指令 (Process Status)** 了！🎉

昨天我們已經在 Kernel 與 User Space 之間架好了一座「查戶口」的橋樑 (`get_process_list`)。今天，我們要把這個資料視覺化，寫一支真正的 User Space 應用程式 `ps.elf`，讓它在你的 Terminal 裡印出漂亮的行程清單。

這就跟我們在 Linux 終端機下打 `ps aux` 是一樣的道理！請跟著我完成這 3 個步驟：

---

### 步驟 1：撰寫 `ps` 應用程式 (`src/user/bin/ps.c`)

在你的 User Space 新增這個檔案。我們要呼叫昨天做好的 Syscall，並把狀態碼轉成人類看得懂的文字。

建立 **`src/user/bin/ps.c`**：

```c
#include "stdio.h"
#include "unistd.h"

// 狀態碼轉換 (對應 Kernel 裡的 TASK_RUNNING, WAITING 等)
const char* state_to_string(unsigned int state) {
    switch (state) {
        case 0: return "RUNNING";
        case 1: return "DEAD   ";
        case 2: return "WAITING";
        case 3: return "ZOMBIE ";
        default: return "UNKNOWN";
    }
}

int main(int argc, char** argv) {
    process_info_t procs[32]; // 假設系統最多同時有 32 個行程
    
    // 呼叫 Syscall，取得目前所有的行程清單
    int count = get_process_list(procs, 32);

    if (count == 0) {
        printf("Error: Could not retrieve process list.\n");
        return -1;
    }

    printf("\n");
    // 簡單的表格標頭 (因為我們的 printf 還沒實作 %8d 這種對齊，所以先用空白稍微排版)
    printf("PID   PPID   STATE      MEMORY      CMD\n");
    printf("---   ----   -----      ------      ---\n");

    for (int i = 0; i < count; i++) {
        // 印出每一筆行程的資料
        printf("%d     %d      %s    %d B       %s\n", 
            procs[i].pid, 
            procs[i].ppid, 
            state_to_string(procs[i].state), 
            procs[i].memory_used, 
            procs[i].name
        );
    }
    printf("\nTotal processes: %d\n", count);

    return 0;
}
```

---

### 步驟 2：將 `ps.elf` 加入開機掛載清單 (`src/kernel/kernel.c`)

我們寫好的 `ps` 程式編譯成 `ps.elf` 後，必須讓 GRUB 載入它，並且讓 Kernel 把它拷貝到 SimpleFS 虛擬硬碟裡，這樣 Shell 才找得到它。

打開 **`src/kernel/kernel.c`**，找到 `setup_filesystem` 函式，把 `ps.elf` 加進陣列裡：

```c
void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    // ...
    // 【修改】在 filenames 陣列中加入 "ps.elf"
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf", "ps.elf"}; // <--- 新增 ps.elf
        
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            // ...
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }
}
```

---

### 步驟 3：更新 GRUB 設定檔 (`scripts/grub.cfg`)

最後，我們必須告訴 GRUB 在開機時，順便把剛編譯好的 `ps.elf` 載入到記憶體中，作為 Multiboot Module 傳給 Kernel。

打開 **`scripts/grub.cfg`**，在 **兩個 menuentry** (`Desktop GUI` 和 `Command Line`) 裡面，都補上 `ps.elf` 的 module 宣告：

```text
menuentry "Simple OS (Desktop GUI)" {
    set gfxpayload=800x600x32
    multiboot /boot/myos.bin mode=gui
    module /boot/shell.elf
    module /boot/echo.elf
    module /boot/cat.elf
    module /boot/ping.elf
    module /boot/pong.elf
    module /boot/touch.elf
    module /boot/ls.elf
    module /boot/rm.elf
    module /boot/mkdir.elf
    module /boot/ps.elf    # <--- 新增這一行
}

# (請記得在 "Command Line" 的 menuentry 裡也加上 module /boot/ps.elf)
```

*(💡 溫馨提醒：請確認你的 `Makefile` 有設定好會把 `src/user/bin/ps.c` 編譯成 `ps.elf` 並放進 ISO 檔的 `/boot/` 目錄下！如果你原本是用 wildcard `*.c` 編譯的，那它應該會自動處理好。)*

---

### 🚀 驗收：見證你的系統生態

存檔後，敲下：
```bash
make clean && make run
```

進入桌面後，打開 Terminal，直接輸入：
```bash
ps
```



你將會看到一張超級壯觀的表格！
裡面會列出 PID 0 的 `[kernel_idle]`、PID 1 的 `[kernel_main]`、PID 2 的 `shell.elf`，還有你剛剛輸入並正在執行的 `ps.elf` 本身！這證明了你的作業系統真的在進行多工排程，而且 User Space 已經能完美透視 Kernel 的秘密了。

試試看開兩個 Terminal，或者在背景跑個 `ping` 再下 `ps`，看看名單會有什麼變化！測試成功後跟我說，我們明天 **Day 65** 就要來挑戰動態刷新的 `top` 指令了！😎
