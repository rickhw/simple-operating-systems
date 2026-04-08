/**
 * @file src/kernel/arch/x86/paging.c
 * @brief Paging and virtual memory management.
 *
 * This file implements the paging system for x86. It manages page directories,
 * page tables, and handles page faults. It also supports multiple "universes"
 * (page directories) for process isolation.
 */

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"
#include "task.h" // Ensure task.h is included for current_task

// Global kernel page directory and tables
uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));

// User-space page tables
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));

// Video RAM page table
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

/**
 * Pre-allocate 16 "universes" (page directories) for process isolation.
 */
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));

int next_universe_id = 0;
int universe_used[16] = {0};

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

/**
 * @brief Initialize paging system.
 */
void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    
    // Identity map the first 4MB for the kernel
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 3; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; }

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;

    // Mount 0x10000000 region (pd_idx = 64) for user heap
    page_directory[64] = ((uint32_t)user_heap_page_table) | 7;

    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3;

    load_page_directory(page_directory);
    enable_paging();
}

/**
 * @brief Map a virtual address to a physical address.
 * @param virt Virtual address.
 * @param phys Physical address.
 * @param flags Page flags (Present, Read/Write, User/Supervisor).
 */
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;

    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // Stack & Code region (0x08000000)
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
        // User Heap region (0x10000000)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64];

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

/**
 * @brief Create a new page directory for a new process.
 * @return Physical address of the new page directory.
 */
uint32_t create_page_directory() {
    int id = -1;
    // Find a free universe
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

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0;
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}

/**
 * @brief Free a page directory and its associated physical pages.
 * @param pd_phys Physical address of the page directory to free.
 */
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {
            // 1. Free Code & Stack physical pages
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_pts[i][j] = 0;
                }
            }

            // 2. Free Heap physical pages
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            universe_used[i] = 0;
            return;
        }
    }
}

/**
 * @brief Map Video RAM for Video Framebuffer.
 * @param virt Virtual address.
 * @param phys Physical address.
 */
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

/**
 * @brief Get the number of active universes.
 * @return Count of active universes.
 */
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}

/**
 * @brief Page fault handler.
 * @param regs The current register state.
 */
void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // Error code bits: 0: present, 1: write, 2: user
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;           // 0: read, 1: write
    int us = regs->err_code & 0x4;           // 0: kernel, 1: user

    if (us) {
        kprintf("\n[Kernel] Segmentation Fault at 0x%x!\n", faulting_address);
        kprintf("  -> Cause: %s in %s mode\n",
                present ? "Page not present" : "Protection violation",
                rw ? "Write" : "Read");

        // Kill the offending process
        sys_kill(current_task->pid);

        // Switch task
        schedule();
    } else {
        // Kernel panic
        kprintf("\nKERNEL PANIC: Page Fault at 0x%x\n", faulting_address);
        while(1) __asm__ volatile("cli; hlt");
    }
}
