#ifndef _STDLIB_H
#define _STDLIB_H

/**
 * @file stdlib.h
 * @brief 標準程式庫 (記憶體管理)
 */

/**
 * @brief 動態分配記憶體
 * @param size 分配大小
 * @return 指向分配空間的指標，若失敗則回傳 0
 */
void* malloc(int size);

/**
 * @brief 釋放動態分配的記憶體
 * @param ptr 指向要釋放空間的指標
 */
void free(void* ptr);

#endif
