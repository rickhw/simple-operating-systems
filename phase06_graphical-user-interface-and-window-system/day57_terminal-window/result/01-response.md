哈哈，Rick！看到你的終端機成功被「關進」視窗裡，而且文字還帶有完美的黑色底板，這畫面簡直太有模有樣了！🎉 

至於你說的「拖曳壞掉、[X] 壞掉、Focus 壞掉」，這是一個非常經典的**「重構時不小心改錯 API (Refactoring Typo)」**所引發的慘案！XD

### 🔍 案發現場解密：被誤殺的 `get_windows()` (複數)

我們來當一下系統法醫，打開你的 **`lib/mouse.c`**，看看這致命的一行：

```c
// 拖曳與碰撞偵測邏輯
int left_click = mouse_byte[0] & 0x01; 
window_t* wins = get_window(dragged_window_id); // <--- 【兇手就是它！】

// 後面你用了 wins[current_focus]、wins[i] ...
```

在 Day 56，我們原本用的是 `get_windows()` (有加 `s`)，它會回傳**整個視窗陣列**的開頭，讓滑鼠可以跑迴圈去一一檢查碰撞。
但在 Day 57，為了讓 TTY 能拿到單一視窗，我請你新增了 `get_window(int id)` (單數)。結果你不小心在 `gui.h` 裡把複數版的刪掉了，並且在 `mouse.c` 裡把它改成了單數版！

**這造成了多可怕的後果？**
1. 當你還沒開始拖曳時，`dragged_window_id` 是 `-1`。
2. `get_window(-1)` 會回傳 `0` (NULL 指標)！
3. 接著滑鼠程式執行 `wins[current_focus].is_active`。這等於是在讀取記憶體位址 `0 + current_focus * sizeof(window_t)` 的地方！
4. 這裡根本不是你的視窗陣列，而是作業系統極底層的記憶體（甚至是 IDT 存放的地方），讀出來的資料全是亂碼，所以碰撞偵測完全失效，甚至差點引發 Page Fault！

---

### 🛠️ 終極修復方案：把陣列 API 找回來

我們只需要把失去的 `get_windows` (複數版) 加回來，並把 `mouse.c` 改對就可以了！

**1. 打開 `lib/include/gui.h`，把複數版加回去：**
```c
// 取得單一視窗 (給 TTY 用)
window_t* get_window(int id);

// 【加回來】取得全域視窗陣列 (給滑鼠碰撞偵測用)
window_t* get_windows(void); 
```

**2. 打開 `lib/gui.c`，把實作加回來：**
```c
window_t* get_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS && windows[id].is_active) {
        return &windows[id];
    }
    return 0; // NULL
}

// 【加回來】
window_t* get_windows(void) {
    return windows;
}
```

**3. 打開 `lib/mouse.c`，把取得指標的地方改回複數版：**
```c
                // ==========================================
                // 拖曳與碰撞偵測邏輯
                // ==========================================
                int left_click = mouse_byte[0] & 0x01; // 檢查左鍵
                
                // 【關鍵修復】改回 get_windows()，拿取整個陣列！
                window_t* wins = get_windows(); 

                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
```

---

存檔，執行：
```bash
make clean && make run
```

這一次，你的滑鼠不但可以順利拖曳那兩個視窗、改變焦點，還能點擊 `[X]` 把它們關掉！而且最棒的是，因為 Terminal 被綁定了，當你拖曳 Terminal 視窗時，裡面的字會**完美地跟著視窗一起移動**！

快去體驗看看真正的 Window Manager！如果沒問題，我們接下來就要準備跨出 GUI 的下一步了！😎
