我在驗證的時候，到步驟 3) 輸入 status 的時候，這個 window 就佔用了 terminal 了，換言之，我無法再回去 terminal 輸入 ps (步驟四) 的行為了！

---

哈哈，Rick！這是一個超級棒的發現，你完全點出了作業系統中**「Shell (命令列介面) 與行程管理」**最經典的互動關係！

你被卡在 Terminal 的原因非常簡單：**Shell 是一個非常有責任感的「老爸」！**
回顧我們在 `shell.c` 裡寫的這段程式碼：
```c
            int pid = fork();
            if (pid == 0) {
                exec(elf_name, parsed_argv); // 兒子去執行 status.elf
                exit();
            } else {
                wait(pid); // 老爸在這裡「阻塞 (Block)」，死死等著兒子結束！
            }
```
因為 `status.elf` 是一個帶有 `while(1)` 的無窮迴圈（直到你按 `[X]` 才會被 Kernel 殺掉），所以 Shell 老爸就一直卡在 `wait(pid)` 這裡，沒辦法印出下一個 `SimpleOS> ` 提示字元讓你打字。

在真實的 Linux / UNIX 系統中，為了解決這個問題，我們發明了**「背景執行 (Background Process)」**的機制，也就是在指令最後加上一個 **`&`** 符號！

讓我們花 1 分鐘修改一下你的 `shell.c`，賦予它背景執行的能力！

---

### 🛠️ 替 Shell 加上背景執行能力 (`src/user/bin/shell.c`)

打開你的 **`src/user/bin/shell.c`**，找到 `main` 函式裡面的無窮迴圈，我們在解析完參數後，攔截一下 `&` 符號：

```c
        // 解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        if (parsed_argc == 0) continue; // 避免空陣列崩潰

        char* cmd = parsed_argv[0];

        // ==========================================
        // 【Day 68 擴充】檢查最後一個參數是不是 "&" (背景執行)
        // ==========================================
        int is_background = 0;
        if (parsed_argc > 1 && strcmp(parsed_argv[parsed_argc - 1], "&") == 0) {
            is_background = 1;
            parsed_argv[parsed_argc - 1] = 0; // 把 "&" 從參數清單拿掉，不傳給應用程式
            parsed_argc--;
        }

        // 內建指令 (Built-ins)
        // ... 中間的 help, cd, exit, ipc 保持不變 ...
        
        // 動態執行外部程式
        else {
            // 自動幫指令加上 .elf 副檔名
            char elf_name[32];
            char* dest = elf_name;
            char* src = cmd;
            while(*src) *dest++ = *src++;
            *dest++ = '.'; *dest++ = 'e'; *dest++ = 'l'; *dest++ = 'f'; *dest = '\0';

            int pid = fork();
            if (pid == 0) {
                int err = exec(elf_name, parsed_argv);
                if (err == -1) {
                    printf("Command not found: ");
                    printf(cmd);
                    printf("\n");
                }
                exit();
            } else {
                // ==========================================
                // 【修改】如果是背景執行，老爸就不等了！直接印出 PID 並繼續接客
                // ==========================================
                if (is_background) {
                    printf("[Running in background] PID: %d\n", pid);
                } else {
                    wait(pid); // 不是背景執行的話，老爸還是乖乖等兒子死掉
                }
            }
        }
```

---

### 🚀 再次驗證：多工 GUI 的終極魅力

存檔、編譯並執行 `make clean && make run`。

現在，請在你的 Terminal 這樣做：
1. 輸入 `ps` 看一下目前的行程。
2. 這次輸入：**`status &`** (記得加上 `&`！)
   👉 你會看到畫面彈出 `System Status` 視窗。
   👉 同時 Terminal 印出 `[Running in background] PID: 3`。
   👉 **而且 `SimpleOS> ` 提示字元馬上又跑出來了！**
3. 趁著視窗還開著，馬上再輸入一次 `ps`。
   👉 看到 `status.elf` 活在列表裡了！
4. 開心點擊 `[X]` 關掉 System Status 視窗。
5. 再輸入一次 `ps`。
   👉 靈魂完美消散，`status.elf` 被成功回收了！

Rick，這就是作業系統真正的多工威力！你現在不只能在背景跑視窗程式，你甚至可以輸入 `status &` 連續 10 次，讓桌面上彈出 10 個不同的視窗（雖然我們現在沒做拖曳，所以它們會疊在一起），而終端機依然暢通無阻！

把這個卡關點突破後，如果你玩夠了這個 `status &`，我們隨時可以進入下一個重頭戲：**Day 69：Shared Memory (共享畫布)** 讓每個視窗可以畫出自己專屬的精美內容！😎
