#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"

// 1. 宣告兩張分頁表 (必須對齊 4KB)
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096))); // 管 0MB ~ 4MB
uint32_t second_page_table[1024] __attribute__((aligned(4096)));// 我們用這張表來管 2GB 附近的高階記憶體
uint32_t third_page_table[1024] __attribute__((aligned(4096))); // 給 3GB (0xC0000000) 用
uint32_t user_page_table[1024] __attribute__((aligned(4096)));

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
        // [修改] 將權限從 3 改成 7 (Present | Read/Write | User)
        first_page_table[i] = (i * 0x1000) | 7;
    }

    // [新增] 初始化第二張表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        second_page_table[i] = 0;
    }

    // 初始化第三張表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        third_page_table[i] = 0;
    }

    // [新增] 初始化 User 表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        user_page_table[i] = 0;
    }

    // 將兩張表掛載到目錄上
    // 這樣 0x00000000 到 0x003FFFFF 的虛擬位址就會被翻譯到這裡
    // [修改] 目錄的第 0 項也必須加上 User 權限 (7)
    page_directory[0] = ((uint32_t)first_page_table) | 7;

    // [Day20][新增] 掛載到目錄的第 32 項 (管理 128MB ~ 132MB 的虛擬空間)
    page_directory[32] = ((uint32_t)user_page_table) | 7; // 7 代表允許 Ring 3 存取

    // 0x80000000 除以 4MB (0x400000) = 512，所以 2GB 的位址是由目錄的第 512 項來管！
    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3; // [新增] 3GB 目錄項

    // 4. 呼叫組合語言，把目錄位址交給 CPU，並開啟 Paging
    load_page_directory(page_directory);
    enable_paging();
}


// === [今天的主角：動態映射函式] ===
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // 1. 取得目錄索引 (最高 10 bits)：右移 22 位元
    uint32_t pd_idx = virt >> 22;
    // 2. 取得分頁表索引 (中間 10 bits)：右移 12 位元後，用 0x3FF (1023) 遮罩
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    // 取得該目錄項指到的分頁表位址
    uint32_t* page_table;
    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {          // [新增] 支援 128MB 區域
        page_table = user_page_table;
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table; // [新增] 支援 3GB 映射
    } else {
        // 在一個完整的 OS 中，這裡應該要 pmm_alloc_page() 一個新的實體框，
        // 然後用遞迴映射 (Recursive Paging) 的黑魔法來初始化它。我們今天先跳過這塊深水區。
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    // 3. 把實體位址 (對齊 4KB) 加上權限標籤，寫入分頁表！
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;

    // 4. [極度重要] 刷新 TLB (Translation Lookaside Buffer)
    // CPU 為了加速，會把舊的地址對應記在快取裡。我們改了字典，必須強迫 CPU 重讀！
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}
