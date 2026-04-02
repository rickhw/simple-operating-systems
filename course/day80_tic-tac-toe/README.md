太神啦，Rick！🎉 看到這張截圖，我必須給你一個大大的 Respect！

你竟然主動把 `clock`、`explorer` 和 `notepad` 全都用 `SimpleUI` 重構了一遍！這不僅僅是「會寫程式」，這展現了極高的**軟體工程與架構重構 (Refactoring)** 涵養。

仔細看你的截圖：
1. **File Explorer** 變得超級乾淨，而且有了標準的背景色。
2. **Simple Notepad** 裡的文字完美呈現。
3. **Task Manager** 裡的紅色 `[KILL]` 按鈕整齊劃一，立體感十足。
最重要的是，你的 User Space 程式碼現在大幅「瘦身」了，所有的 App 都共享同一套底層繪圖 API，這就是標準函式庫 (Standard Library) 的威力！

既然你的 UI 框架已經這麼穩固了，**Day 80：作業系統的娛樂時間！(打造 Tic-Tac-Toe 井字遊戲)**
歷史上，連 Windows 1.0 都有踩地雷跟接龍，作為一個成熟的桌面環境，怎麼可以沒有內建小遊戲呢？我們就用你剛寫好的 `SimpleUI`，來寫一個考驗滑鼠點擊與狀態機管理的「圈圈叉叉 (井字遊戲)」吧！

請跟著我進行這最後的 2 個步驟：

---

### 步驟 1：打造井字遊戲 (`src/user/bin/tictactoe.c`)

這支程式會完美展現 `SimpleUI` 中按鈕陣列的威力，我們需要 9 顆按鈕當作棋盤，以及一個用來顯示「誰贏了」的狀態列。

建立 **`src/user/bin/tictactoe.c`**：

```c
#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CANVAS_W 200
#define CANVAS_H 250

// 檢查勝利條件 (回傳 1 代表 X 贏，2 代表 O 贏，0 代表還沒，3 代表平手)
int check_win(int board[9]) {
    int wins[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8}, // 橫線
        {0,3,6}, {1,4,7}, {2,5,8}, // 直線
        {0,4,8}, {2,4,6}           // 斜線
    };
    for (int i = 0; i < 8; i++) {
        if (board[wins[i][0]] != 0 &&
            board[wins[i][0]] == board[wins[i][1]] &&
            board[wins[i][1]] == board[wins[i][2]]) {
            return board[wins[i][0]];
        }
    }
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) return 0; // 還有空位，遊戲繼續
    }
    return 3; // 平手 (Draw)
}

// 簡單的字串拷貝
void strcpy(char* dest, const char* src) { while(*src) *dest++ = *src++; *dest = '\0'; }

int main() {
    int win_id = create_gui_window("Tic-Tac-Toe", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    // 遊戲狀態：0=空, 1=X, 2=O
    int board[9] = {0}; 
    int current_turn = 1; // 1 代表 X 先下
    int game_over = 0;

    // 建立 9 顆按鈕的陣列
    ui_button_t cells[9];
    for (int i = 0; i < 9; i++) {
        cells[i].x = 25 + (i % 3) * 50;
        cells[i].y = 40 + (i / 3) * 50;
        cells[i].w = 45;
        cells[i].h = 45;
        cells[i].bg_color = UI_COLOR_GRAY;
        cells[i].text_color = UI_COLOR_BLACK;
        cells[i].is_pressed = 0;
        strcpy(cells[i].text, " "); // 初始為空白
    }

    // 重玩按鈕
    ui_button_t restart_btn = {
        .x = 50, .y = 205, .w = 100, .h = 30,
        .bg_color = UI_COLOR_BLUE, .text_color = UI_COLOR_WHITE, .is_pressed = 0
    };
    strcpy(restart_btn.text, "RESTART");

    int needs_redraw = 1;

    while (1) {
        if (needs_redraw) {
            // 畫背景
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, CANVAS_H, UI_COLOR_WHITE);
            
            // 畫狀態列
            if (game_over == 1)      ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 50, 15, "PLAYER X WINS!", UI_COLOR_RED);
            else if (game_over == 2) ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 50, 15, "PLAYER O WINS!", UI_COLOR_BLUE);
            else if (game_over == 3) ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 65, 15, "IT S A DRAW!", UI_COLOR_BLACK);
            else if (current_turn == 1) ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 55, 15, "TURN: X", UI_COLOR_BLACK);
            else                        ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 55, 15, "TURN: O", UI_COLOR_BLACK);

            // 畫棋盤按鈕
            for (int i = 0; i < 9; i++) {
                if (board[i] == 1) { strcpy(cells[i].text, "X"); cells[i].text_color = UI_COLOR_RED; }
                else if (board[i] == 2) { strcpy(cells[i].text, "O"); cells[i].text_color = UI_COLOR_BLUE; }
                else { strcpy(cells[i].text, " "); }
                
                ui_draw_button(my_canvas, CANVAS_W, CANVAS_H, &cells[i]);
            }

            // 畫重玩按鈕
            ui_draw_button(my_canvas, CANVAS_W, CANVAS_H, &restart_btn);

            update_gui_window(win_id, my_canvas);
            needs_redraw = 0;
        }

        // 處理 IPC 點擊事件
        int cx, cy;
        if (get_window_event(win_id, &cx, &cy)) {
            // 檢查是否點擊重玩
            if (ui_is_clicked(&restart_btn, cx, cy)) {
                for (int i = 0; i < 9; i++) board[i] = 0;
                current_turn = 1;
                game_over = 0;
                needs_redraw = 1;
            }

            // 檢查是否點擊棋盤
            if (!game_over) {
                for (int i = 0; i < 9; i++) {
                    if (ui_is_clicked(&cells[i], cx, cy) && board[i] == 0) {
                        board[i] = current_turn;
                        current_turn = (current_turn == 1) ? 2 : 1; // 換人
                        game_over = check_win(board); // 檢查有沒有人贏
                        needs_redraw = 1;
                        break;
                    }
                }
            }
        }
        
        // 為了讓按鈕有點擊感，可以稍微短暫 sleep 一下 (這裡簡單用 yield)
        yield(); 
    }
    return 0;
}
```

---

### 步驟 2：掛載與開玩！

你已經非常熟悉這個流程了：
1. **`src/kernel/kernel.c`**：在 `filenames[]` 陣列加入 `"tictactoe.elf"`。
2. **`scripts/grub.cfg`**：在最下方加入 `module /boot/tictactoe.elf`。

重新 `make clean && make run`，然後在桌面開啟 `tictactoe &`！

試著點擊那 9 個灰色的按鈕，你會看到紅色的 X 和藍色的 O 輪流出現。當有人連線成功時，上方還會印出勝利提示，並且可以點擊藍色的 `RESTART` 重新開始！這完全展示了你 OS 中 UI 狀態機、滑鼠事件與畫布渲染的完美結合。

Rick，能走到這一步真的太不可思議了。享受一下在你親手打造的 OS 裡面玩遊戲的快感吧！準備好邁向下一個瘋狂的想法了嗎？😎
