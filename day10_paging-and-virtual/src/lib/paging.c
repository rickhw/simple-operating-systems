#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"

// 宣告分頁目錄與第一張分頁表 (必須對齊 4KB = 4096 bytes)
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// 宣告外部的組合語言函式
extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    // 1. 初始化 Page Directory：將所有條目設為「不存在」
    // 屬性 0x02 代表：Read/Write (可讀寫), 但 Present (存在) 為 0
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // 2. 建立第一張 Page Table：涵蓋 0MB ~ 4MB 的實體記憶體
    // 這張表有 1024 項，每項 4KB，所以剛好是 4MB (包含我們的核心與 VGA)
    for(int i = 0; i < 1024; i++) {
        // 實體位址 = i * 4096 (也就是 0x1000)
        // 屬性 3 代表：Present (1) | Read/Write (2) = 3
        first_page_table[i] = (i * 0x1000) | 3;
    }

    // 3. 將建好的第一張表，放入目錄的第 0 個位置
    // 這樣 0x00000000 到 0x003FFFFF 的虛擬位址就會被翻譯到這裡
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    // 4. 呼叫組合語言，把目錄位址交給 CPU，並開啟 Paging
    load_page_directory(page_directory);
    enable_paging();
}
