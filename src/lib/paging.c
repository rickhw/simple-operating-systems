#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 【神級捷徑】預先分配 16 個宇宙的空間！
// 確保它們永遠在 Kernel 0~4MB 的 1:1 映射區內，實體位址 = 虛擬位址！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
int next_universe_id = 0;

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    for(int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 7;
    }

    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;
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
        // =========================================================
        // 【宇宙感知】從目前的 CR3 找出我們該寫哪一張 user_page_table！
        // =========================================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;

        uint32_t pt_entry = current_pd[32];
        if (pt_entry & 1) { // 檢查 Present bit
            // 因為 0-4MB 1:1 映射，實體位址可以直接當成指標使用！
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
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
    if (next_universe_id >= 16) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    // 1. 取出預先分配好的記憶體區塊 (極度安全，無須 map_page)
    int id = next_universe_id++;
    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];

    // 2. 複製 Kernel 原始目錄的基礎建設 (0-4MB 核心區, Heap 等)
    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    // 3. 將新的 User Page Table 清空 (新程式不會繼承老爸的記憶體殘骸)
    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
    }

    // 4. 掛載新的 User Page Table 到新宇宙的第 32 個目錄項
    // 7 = Present | Read/Write | User Mode
    new_pd[32] = ((uint32_t)new_pt) | 7;

    // 5. 回傳新宇宙的實體位址，準備給 sys_exec 裡的 CR3 使用！
    return (uint32_t)new_pd;
}
