#include "string.h"
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
