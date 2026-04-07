#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

/**
 * @file string.h
 * @brief 標準字串處理函式
 */

/**
 * @brief 計算字串長度
 * @param s 字串
 * @return 字串長度 (不含空字元)
 */
int strlen(const char* s);

/**
 * @brief 複製字串
 * @param dest 目標緩衝區
 * @param src 來源字串
 */
void strcpy(char* dest, const char* src);

/**
 * @brief 比較兩個字串
 * @param s1 字串 1
 * @param s2 字串 2
 * @return 0 表示相等，非 0 表示不相等 (其差值)
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief 解析指令參數 (處理空白與引號)
 * @param input 輸入字串 (會被修改，插入 '\0')
 * @param argv 輸出的參數指標陣列
 * @return 參數個數 (argc)
 */
int parse_args(char* input, char** argv);

#endif
