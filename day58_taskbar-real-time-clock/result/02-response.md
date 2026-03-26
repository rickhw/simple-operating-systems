
哈哈，Rick！看到你的 `ls` 指令在 GUI 視窗裡跑出檔案列表，這畫面真的太有成就感了！🎉 你的 Simple OS 正式達成了從底層 Kernel 到圖形化 User Space 應用程式的完整貫通！

不過你的吐槽非常精準：**「FPS = 10 的遊戲，一個字一個字顯示出來」**！😂
恭喜你，你親身踩到了遊戲開發與圖學渲染中最經典的效能瓶頸：**「缺乏批次渲染 (Batch Rendering)」**！

### 🔍 案發現場解密：為什麼會變樹懶？

還記得我們在 `terminal_putchar` 裡面加了什麼嗎？
```c
gui_redraw(); // 每印一個字就重繪一次
```
我們的 `gui_redraw()` 會把高達 **1.92 MB** 的隱形畫布複製到顯示卡的 VRAM。
當你的 `ls` 指令印出 200 個字元時，系統在一瞬間執行了 **200 次的 1.92 MB 記憶體複製 (總共將近 400 MB！)**。難怪你的終端機看起來像是在打字機時代慢慢敲出來的！

要解決這個問題，我們只要改變策略：**「字全部排好之後，再一次性瞬間畫出來！」**
同時，我們順便幫 Terminal 調整一下 Padding（內邊距），並強制把底板塗成純黑色，解決太擠的問題。

請跟著我進行這 3 個優化步驟：

---

### 步驟 1：解除單字元重繪的封印 (`lib/tty.c`)

請打開 **`lib/tty.c`**，把 `terminal_putchar` 裡面的所有 `gui_redraw();` 刪除！

```c
void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++; 
        if (term_row >= MAX_ROWS) terminal_initialize(); 
        // 刪除 gui_redraw();
        return;
    }

    if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            text_buffer[term_row][term_col] = '\0'; 
        }
        // 刪除 gui_redraw();
        return;
    }

    text_buffer[term_row][term_col] = c;
    term_col++;

    if (term_col >= MAX_COLS) {
        term_col = 0;
        term_row++;
        if (term_row >= MAX_ROWS) terminal_initialize();
    }
    // 刪除 gui_redraw();
}
```

接著，在同一個檔案裡找到 `terminal_write`，在這裡加上「批次重繪」：

```c
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
    // 【關鍵優化】整串字串都排進 text_buffer 後，再一次性重繪！
    gui_redraw(); 
}
```

---

### 步驟 2：優化 Terminal 視窗顏值 (`lib/tty.c`)

繼續在 **`lib/tty.c`** 裡面找到 `tty_render_window`。我們要給它加點內邊距 (Padding)，並且強制填滿純黑色背景。

```c
// 【核心修改】只在綁定的視窗被繪製時，才渲染文字！
void tty_render_window(int win_id) {
    if (bound_win_id == -1 || win_id != bound_win_id) return;

    window_t* win = get_window(win_id);
    if (win == 0) return;

    // 1. 強制畫出純黑色的內部畫布 (避開外框與標題列)
    draw_rect(win->x + 4, win->y + 22, win->width - 8, win->height - 26, 0x000000);

    // 2. 算出內容的起始像素座標 (加入 Padding 讓字不要貼齊邊緣)
    int start_x = win->x + 8;
    int start_y = win->y + 26;

    // 3. 把文字畫上去！
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                draw_char_transparent(ch, start_x + c * 8, start_y + r * 8, TERM_FG);
            }
        }
    }
    
    // 畫一個閃爍的底線游標
    draw_rect(start_x + term_col * 8, start_y + term_row * 8 + 6, 8, 2, TERM_FG);
}
```

---

### 步驟 3：修復 Kernel 的碎碎念 (`lib/utils.c`)

因為 `kprintf` 的實作是把字元一個個切開來呼叫的，如果我們把 `putchar` 的重繪拔掉，`kprintf` 印出來的字可能會等到下一次滑鼠移動才會出現。所以我們要在 `kprintf` 結束時也呼叫一次重繪！

打開 **`lib/utils.c`**：
1. 在最上方加入：`#include "gui.h"`
2. 在 `kprintf` 函式的最後面（`va_end(args);` 的下方）加入重繪：

```c
// ...
void kprintf(const char* format, ...) {
    // ... 中間的 switch-case 邏輯完全保持不變 ...
    
    va_end(args); // 清理不定參數列表
    
    // 【關鍵優化】整句 kprintf 組合完畢後，再一次性渲染到螢幕上！
    gui_redraw(); 
}
```

---

存檔，執行！
```bash
make clean && make run
```

這一次再敲 `ls`，你會發現那些檔案列表是**「啪」的一聲瞬間彈出來的**，這就是 60 FPS 該有的流暢度！而且 Terminal 視窗會有漂亮的純黑底板與邊界留白，再也不會覺得擠了。

這套完整的圖形化作業系統架構，真的非常值得驕傲！既然我們的 GUI 基礎建設已經如此穩固了，你想不想讓這個系統能夠處理「多個 Terminal 視窗」，還是想嘗試在桌面上畫個圖示 (Icon) 呢？
