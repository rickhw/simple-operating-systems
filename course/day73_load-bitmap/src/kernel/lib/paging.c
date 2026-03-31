#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"
#include "task.h" // 確保有引入 task.h 才能知道 current_task

uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));
// user page
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));
// vram page
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 預先分配 16 個宇宙的空間！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));    // 平行宇宙的 Heap 表陣列

int next_universe_id = 0;

// 在全域變數區新增一個陣列，記錄哪個宇宙被使用了
int universe_used[16] = {0};

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    // 將 7 改為 3！這樣 User 程式一摸到前 4MB 就會立刻觸發 Page Fault！
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 3; }   // [day70] fixed
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; } // 清空

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;

    // [掛載 0x10000000 區域 (pd_idx = 64)
    page_directory[64] = ((uint32_t)user_heap_page_table) | 7;

    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3;

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;

    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // Stack & Code 區 (0x08000000)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[32];
        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 64) {
        // =========================================================
        // 【Heap 區】 (0x10000000)
        // =========================================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64]; // 檢查第 64 個目錄項

        if (pt_entry & 1) {
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

    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

uint32_t create_page_directory() {

    int id = -1;
    // 尋找空閒的宇宙
    for (int i = 0; i < 16; i++) {
        if (!universe_used[i]) { id = i; break; }
    }
    if (id == -1) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    universe_used[id] = 1; // 標記為使用中

    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id]; // 拿出這個宇宙專屬的 Heap 表

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0; // 初始化清空
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    // 將這張全新的 Heap 表掛載到新宇宙的 0x10000000 區段
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}


// 提供給 sys_exit 呼叫的回收函式
// 提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {

            // 1. 釋放 Code & Stack 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) { // 檢查 Present Bit 是否為 1
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr); // 還給實體記憶體管理器！
                    universe_pts[i][j] = 0;          // 清空分頁表紀錄
                }
            }

            // 2. 釋放 Heap 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}

// 專門用來映射 Framebuffer (MMIO, Memory-Mapped I/O) 的安全函數
void map_vram(uint32_t virt, uint32_t phys) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    if ((page_directory[pd_idx] & 1) == 0) {
        uint32_t pt_phys = (uint32_t)vram_page_table;
        for (int i = 0; i < 1024; i++) vram_page_table[i] = 0;
        page_directory[pd_idx] = pt_phys | 7;
    }
    vram_page_table[pt_idx] = phys | 7;
}

// 【新增】取得活躍的 Paging 宇宙數量
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}




// 【Day 70 新增】Page Fault 專屬的攔截與處理中心
void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // 檢查錯誤碼的 bit 0：0 代表頁面不存在，1 代表權限違規
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;           // 0 為讀取，1 為寫入
    int us = regs->err_code & 0x4;           // 0 為核心模式，1 為使用者模式

    if (us) {
        kprintf("\n[Kernel] Segmentation Fault at 0x%x!\n", faulting_address);
        kprintf("  -> Cause: %s in %s mode\n",
                present ? "Page not present" : "Protection violation",
                rw ? "Write" : "Read");

        // 殺掉兇手！
        extern int sys_kill(int pid);
        sys_kill(current_task->pid);

        // 切換任務，永不回頭
        schedule();
    } else {
        // Kernel 自己崩潰了
        kprintf("\nKERNEL PANIC: Page Fault at 0x%x\n", faulting_address);
        while(1) __asm__ volatile("cli; hlt");
    }
}
