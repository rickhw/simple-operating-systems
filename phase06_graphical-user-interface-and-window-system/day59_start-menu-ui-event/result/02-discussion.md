哈哈，Rick！你又一次精準地踩到了作業系統架構中最深、但也最迷人的一個大坑！😂

這絕對不是你的錯，這是所有從「命令列」跨入「圖形介面」的作業系統都會遇到的終極難題：**「視窗 (Window) 不等於 行程 (Process)！」**

### 🔍 案發現場解密：為什麼新的 Terminal 是個「空殼」？

當你點擊 Start Menu 的 "1. Terminal" 時，我們在 `gui.c` 裡面做了什麼事？
```c
int term_win = create_window(..., "Simple OS Terminal");
terminal_bind_window(term_win);
```
這兩行程式碼，只做了一件事：**在畫面上畫了一個新的漂亮框框，並且把我們全域的 `text_buffer` 顯示輸出（螢幕）切換到這個新框框裡。**

**但是！我們「沒有」啟動一個新的 Shell 程式！**


在真正的作業系統中，要打開一個能打字的 Terminal，必須經過這三個超複雜的步驟（目前我們的系統還沒實作）：
1. **多重行程啟動 (Process Spawning)**：點擊選單時，GUI 必須呼叫 `sys_fork` 與 `sys_exec`，真正在記憶體裡生出一個全新的 `shell.elf` 行程。
2. **虛擬終端機 (PTY Subsystem)**：我們現在全系統只有「一個」鍵盤 Ring Buffer。如果開了兩個 Shell，它們會互相打架搶鍵盤輸入！所以必須為每個視窗建立專屬的虛擬輸入/輸出管線 (stdin/stdout)。
3. **輸入路由 (Input Routing)**：鍵盤驅動必須檢查現在的 `focused_window_id` 是誰，然後把使用者按下的鍵，精準地「派送 (Route)」給那個視窗專屬的 Shell。

因為我們的 Simple OS 還是單一全域鍵盤緩衝區，所以你剛剛開出來的，只是一個**「看得到舊 Shell 畫面，但靈魂沒有跟過來的幽靈視窗」**！XD

---

### 🛠️ 最佳實務修復：讓 Start Menu 變成「工作切換器」

既然我們目前的架構是「單一桌面 Shell 實體」，最優雅且符合邏輯的作法，就是讓 Start Menu 的 "1. Terminal" 變成 **「把現有的終端機呼叫到最上層 (Focus)」**，而不是盲目地開一個空殼視窗。

請打開 **`lib/gui.c`**，找到 `gui_check_ui_click` 裡處理 `1. Terminal` 的地方，將它替換成這樣：

```c
            if (y >= menu_y + 10 && y <= menu_y + 30) {
                // 點擊 "1. Terminal"
                // 【核心修正】尋找現有的 Terminal 視窗，把它拉到最上層！
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "Simple OS Terminal") == 0) {
                        set_focused_window(i); // 找到了！拉到最上層並給予焦點
                        found = 1;
                        break;
                    }
                }
                
                // 如果終端機之前被手賤按 [X] 關掉了，我們就在原地幫他把框框畫回來
                // (註：因為 Shell process 其實還活在背景，所以框框畫回來，綁定回去就又看得到了！)
                if (!found) {
                    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");
                    terminal_bind_window(term_win);
                }
            }
```

同樣地，你也可以把 `"2. Sys Status"` 的邏輯改成這樣：

```c
            else if (y >= menu_y + 40 && y <= menu_y + 60) {
                // 點擊 "2. Sys Status"
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "System Status") == 0) {
                        set_focused_window(i);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    create_window(450, 100, 300, 200, "System Status");
                }
            }
```

---

### 🚀 重新編譯與體驗

修改完後，執行 `make clean && make run`。

現在，試著做這套超酷的操作：
1. 點擊 Terminal，讓它取得焦點。
2. 把 `System Status` 拖曳去蓋住 `Terminal`。
3. 點擊左下角 `Start` -> 點擊 `1. Terminal`。

你會發現 Terminal 視窗會**瞬間彈到最上層**，並且依然可以順暢地打字執行指令！如果我不說，別人還以為這是一個已經實作了完整 Window Manager Routing 的成熟作業系統呢！

到這裡為止，我們的 **Phase 6: GUI 圖形化介面** 已經達成了極其輝煌的成就！我們從寫單一像素，一路刻出了雙重緩衝、字型渲染、滑鼠中斷、Z-Order 視窗管理，還有非同步渲染迴圈！

準備好進入下一個階段，或是想在這塊桌面上再加點什麼好玩的小工具嗎？😎
