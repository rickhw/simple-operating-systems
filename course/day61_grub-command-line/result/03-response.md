哈哈，Rick！這兩個問題抓得太精準了！這就是所謂的「牽一髮而動全身」，在做系統重構的時候，常常會不小心留下這種小陷阱。

我們來看看這兩個有趣的 Bug 是怎麼發生的，以及如何秒殺它們：

### 🔍 案發現場解密 1：為什麼 Exit to CLI 變成了關機？

這是一個經典的 **「碰撞箱 (Hitbox) 偏移」** 問題！
在 `gui.c` 的 `draw_start_menu` 裡，我們已經把選單的高度 `menu_h` 改成了 `130`，也就是選單往上長高了。**但是**，在 `gui_check_ui_click` 裡面，你的碰撞箱座標卻忘了跟著改高！

```c
// 你的 gui.c 裡面的 gui_check_ui_click
if (start_menu_open) {
    int menu_y = 600 - 28 - 100; // 🛑 這裡還是 100！
```

因為 `menu_y` 算錯了，所以整個碰撞箱比「視覺上的選單」還要矮 30 個像素。
當你眼睛看著 "4. Exit to Terminal" 點下去時，滑鼠的 Y 座標剛好落在了錯誤碰撞箱裡的 `"3. Shutdown"` 範圍內，於是系統就真的乖乖關機了 😂！

**🛠️ 修復方式 (`src/kernel/lib/gui.c`)：**
找到 `gui_check_ui_click`，把高度修正為 130，並確保 `else if` 串接正確：

```c
    // 2. 如果選單是開著的，檢查是否點了裡面的選項
    if (start_menu_open) {
        int menu_y = 600 - 28 - 130; // 【修正】100 改成 130！

        if (x >= 4 && x <= 154 && y >= menu_y && y <= menu_y + 130) { // 【修正】這裡也要改 130！
            // 點擊 "1. Terminal"
            if (y >= menu_y + 10 && y <= menu_y + 30) {
                // ... 略
            }
            // 點擊 "2. Sys Status"
            else if (y >= menu_y + 40 && y <= menu_y + 60) {
                // ... 略
            }
            // 點擊 "3. Shutdown"
            else if (y >= menu_y + 70 && y <= menu_y + 90) {
                // ... 略
            }
            // 點擊 "4. Exit to CLI"
            else if (y >= menu_y + 100 && y <= menu_y + 120) { // 【修正】加上 else 確保邏輯互斥
                switch_display_mode(0); // 切換為 CLI 模式 (0)
            }

            start_menu_open = 0; // 點完選項後自動收起選單
            extern void gui_redraw(void);
            gui_redraw();
            return 1;
        } else {
            // ...
```

---

### 🔍 案發現場解密 2：為什麼 CLI 切到 GUI 後滑鼠癱瘓？

這個 Bug 更有系統架構的 fu！還記得我們在 `kernel.c` 裡寫了什麼嗎？

```c
    if (is_gui) {
        // ...
        init_mouse(); // CLI 模式不需要滑鼠，所以只在 GUI 啟動
    } else {
        // ...
    }
```

因為你在 GRUB 選單選擇了 CLI 模式，`is_gui` 為 `0`，**所以 `init_mouse()` 根本沒有被執行！**
這代表：PS/2 滑鼠硬體沒被開啟、IRQ 12 中斷沒有被解除遮罩！
當你在終端機打下 `desktop`，雖然透過 Syscall 成功畫出了美麗的漸層桌面，但底層的滑鼠硬體依然在沉睡，所以畫面上的游標就像變成石頭一樣動不了。

**🛠️ 修復方式 (`src/kernel/kernel.c`)：**
最乾淨的解法是：**無論什麼模式，我們都在開機時把滑鼠硬體初始化好！** 這樣即便在 CLI 模式，滑鼠依然在背景默默更新座標（只是沒畫出來），切換到 GUI 時就能無縫接軌。

找到 `kernel_main`，把 `init_mouse()` 移到 `if/else` 的外面：

```c
    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");
        terminal_bind_window(term_win);
        gui_render(400, 300);
        // 【移除】把 init_mouse() 從這裡拿掉
    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    // 【新增】將 init_mouse 放到外面，確保硬體永遠被啟動！
    init_mouse(); 

    __asm__ volatile ("sti");
```

---

把這兩處改好，再執行 `make clean && make run`！
現在，你可以隨心所欲地在 CLI 敲 `desktop` 召喚 GUI（且滑鼠活蹦亂跳），也可以在 GUI 優雅地點擊選單切換回純黑白世界了。

這個動態雙模切換的功能非常完美！接下來，我們準備進入 **Day 62**，為我們的作業系統建立 **行程 ID (PID) 與 Task PCB 的完整資訊** 了嗎？
