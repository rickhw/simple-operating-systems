/**
 * @file src/kernel/arch/x86/paging.c
 * @brief Paging and virtual memory management.
 *
 * 此檔案負責 x86 架構下的分頁機制實作。
 * 包含 Page Directory 與 Page Table 的管理，以及 Page Fault 處理。
 * 同時透過「Universe」的概念，預先配置多組 Page Directory 來達成 Process 隔離。
 */

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"
#include "task.h" // 確保引入 current_task 以供 Page Fault Handler 使用

// ====================================================================
// 全域 Kernel 分頁表配置 (對齊 4KB 以符合硬體規範)
// ====================================================================
uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// User-space 與 VRAM 特殊用途分頁表
// ====================================================================
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 行程隔離 (Process Isolation) 專用資源：Universes
// 預先配置 16 組 Page Directories 供不同的 Process 使用
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));

int next_universe_id = 0;
int universe_used[16] = {0};

// 外部宣告 (定義於 paging_asm.S)
extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

/**
 * @brief 初始化分頁系統
 */
void init_paging(void) {
    // 預設將整個 Page Directory 設為 0x02 (Read/Write, Not Present)
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }

    // 1:1 等價映射 (Identity mapping) Kernel 的前 4MB 記憶體空間
    // 屬性 3 = Present (1) | Read/Write (2)
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 3; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; }

    // 設定 Page Directory Entries (PDEs)
    // 屬性 7 = Present (1) | Read/Write (2) | User (4)
    page_directory[0]   = ((uint32_t)first_page_table) | 7;
    page_directory[32]  = ((uint32_t)user_page_table)  | 7; // User Code & Stack 區域 (0x08000000)

    // 掛載 0x10000000 區域 (pd_idx = 64) 供 User Heap 使用
    page_directory[64]  = ((uint32_t)user_heap_page_table) | 7;

    // Kernel 額外的擴充表
    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3;

    // 將設定好的 Page Directory 載入 CPU 並啟用分頁機制
    load_page_directory(page_directory);
    enable_paging();
}

/**
 * @brief 映射虛擬位址至實體位址
 */
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // 拆解虛擬位址：
    // pd_idx: 前 10 bits 代表 Page Directory 的索引 (Index)
    // pt_idx: 中間 10 bits 代表 Page Table 的索引 (Index)
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;

    // 根據 PDE Index 決定要操作哪一個 Page Table
    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // 處理 User Stack & Code 區域 (對應虛擬位址 0x08000000 起始)
        // 為了確保寫入到正確的 Process，必須從目前的 CR3 取出真正的 Page Directory
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[32];

        if (pt_entry & 1) { // 檢查 Present bit
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 64) {
        // 處理 User Heap 區域 (對應虛擬位址 0x10000000 起始)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64];

        if (pt_entry & 1) { // 檢查 Present bit
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: heap page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table;
    } else {
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    // 寫入 Page Table Entry (PTE)，設定實體位址與標籤，並清除該虛擬位址的 TLB 快取
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

/**
 * @brief 建立一個全新的 Page Directory (Universe) 供新 Process 使用
 */
uint32_t create_page_directory() {
    int id = -1;
    // 尋找一個未使用的 Universe 槽位
    for (int i = 0; i < 16; i++) {
        if (!universe_used[i]) { id = i; break; }
    }

    if (id == -1) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    universe_used[id] = 1;

    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id];

    // 複製 Kernel 的基礎 PDE 配置
    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    // 清空屬於 User 空間的 Page Table
    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0;
    }

    // 將新清空的 User Page Tables 掛載至新的 Page Directory
    new_pd[32] = ((uint32_t)new_pt) | 7;
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}

/**
 * @brief 釋放 Page Directory，並歸還其實體分頁
 */
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {

            // 1. 釋放 Code & Stack 所佔用的實體記憶體
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) { // 如果該分頁存在
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr); // 歸還給 PMM
                    universe_pts[i][j] = 0;
                }
            }

            // 2. 釋放 Heap 所佔用的實體記憶體
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            // 標記該 Universe 為可重複使用
            universe_used[i] = 0;
            return;
        }
    }
}

/**
 * @brief 映射顯示用的 VRAM
 */
void map_vram(uint32_t virt, uint32_t phys) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    // 若對應的 Page Directory Entry 尚未建立，則動態綁定 vram_page_table
    if ((page_directory[pd_idx] & 1) == 0) {
        uint32_t pt_phys = (uint32_t)vram_page_table;
        for (int i = 0; i < 1024; i++) vram_page_table[i] = 0;
        page_directory[pd_idx] = pt_phys | 7; // 屬性 7: Present+RW+User
    }

    // 將 VRAM 的實體位址填入 PTE
    vram_page_table[pt_idx] = phys | 7;
}

/**
 * @brief 取得目前正在活動中的 Universe (Process 空間) 數量
 */
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}

/**
 * @brief Page Fault (分頁錯誤) 的中斷處理常式
 */
void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;

    // 觸發 Page Fault 時，CPU 會自動將引發錯誤的虛擬位址存入 CR2 暫存器
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // 解析 Error Code 欄位：
    // bit 0: Present (0=頁面不存在, 1=保護權限違反)
    // bit 1: Read/Write (0=讀取時發生, 1=寫入時發生)
    // bit 2: User/Kernel (0=Kernel 觸發, 1=User 觸發)
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;
    int us = regs->err_code & 0x4;

    if (us) {
        // User Mode 觸發的異常：印出錯誤資訊並終止該 Process
        kprintf("\n[Kernel] Segmentation Fault at 0x%x!\n", faulting_address);
        kprintf("  -> Cause: %s in %s mode\n",
                present ? "Page not present" : "Protection violation",
                rw ? "Write" : "Read");

        // 砍掉違規的行程 (Task)
        sys_kill(current_task->pid);

        // 切換到下一個行程
        schedule();
    } else {
        // Kernel Mode 觸發的異常：這代表核心設計有嚴重錯誤，必須立即停止 (Kernel Panic)
        kprintf("\nKERNEL PANIC: Page Fault at 0x%x\n", faulting_address);
        while(1) __asm__ volatile("cli; hlt");
    }
}
