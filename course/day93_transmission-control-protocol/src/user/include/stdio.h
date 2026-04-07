#ifndef _STDIO_H
#define _STDIO_H

/**
 * @file stdio.h
 * @brief 標準輸入輸出函式
 */

/**
 * @brief 印出字串到終端機 (目前為同步模式)
 * @param str 要印出的字串
 */
void print(const char* str);

/**
 * @brief 從鍵盤讀取一個字元 (若無按鍵則阻塞)
 * @return 讀取到的字元
 */
char getchar(void);

/**
 * @brief 格式化輸出到終端機
 * @details 目前支援 %d (十進位), %x (十六進位), %s (字串), %c (字元), %% (百分比)
 * @param format 格式字串
 * @param ... 變數參數列表
 */
void printf(const char* format, ...);

#endif
