修改 kernel.c 之後，跑起來之後有問題，底下是 core dump & source code.

我猜是否跟 kheap, pmm, paging 有關係？
另外也附上沒改過的 linker.ld，這裡面有一些記憶體設定的資訊，但我不曉得有沒關係。

---

Core Dump

```bash
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x09
check_exception old: 0xffffffff new 0xe
     0: v=0e e=0002 i=0 cpl=0 IP=0008:00100848 pc=00100848 SP=0010:0010afb8 CR2=00600000
EAX=00600000 EBX=00106084 ECX=00600000 EDX=00601000
ESI=00000000 EDI=00142000 EBP=000003f4 ESP=0010afb8
EIP=00100848 EFL=00200012 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010b000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010b080 0000002f
IDT=     0010b0e0 000007ff
CR0=80000011 CR2=00600000 CR3=00142000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=0000001c CCD=0010afa4 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xe new 0xd
     1: v=08 e=0000 i=0 cpl=0 IP=0008:00100848 pc=00100848 SP=0010:0010afb8 env->regs[R_EAX]=00600000
EAX=00600000 EBX=00106084 ECX=00600000 EDX=00601000
ESI=00000000 EDI=00142000 EBP=000003f4 ESP=0010afb8
EIP=00100848 EFL=00200012 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010b000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010b080 0000002f
IDT=     0010b0e0 000007ff
CR0=80000011 CR2=00600000 CR3=00142000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=0000001c CCD=0010afa4 CCO=ADDL
EFER=0000000000000000
check_exception old: 0x8 new 0xd
```

---

kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"

// 紀錄畫布資訊
uint8_t* fb_addr = 0;
uint32_t fb_pitch = 0;

// 【畫筆魔法】在螢幕上的 x, y 座標點上一個顏色！
void put_pixel(int x, int y, uint32_t color) {
    if (fb_addr == 0) return;

    // 計算記憶體偏移量：Y * 每一行的寬度 + X * 每個像素的寬度(4 Bytes)
    uint32_t offset = (y * fb_pitch) + (x * 4);

    // 32-bit 色彩格式通常是 BGRA (小端序)
    fb_addr[offset]     = color & 0xFF;         // Blue
    fb_addr[offset + 1] = (color >> 8) & 0xFF;  // Green
    fb_addr[offset + 2] = (color >> 16) & 0xFF; // Red
    // fb_addr[offset + 3] 是 Alpha (透明度)，通常不用管
}

// 放在 fb_addr 宣告的附近
extern uint32_t page_directory[];

// 【無敵畫布映射魔法】專門用來處理超高位址 MMIO 的映射，避開所有型別與容量陷阱！
void map_vram(uint32_t virt, uint32_t phys) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    // 如果這個 4MB 的目錄項還沒建立
    if ((page_directory[pd_idx] & 1) == 0) {
        // 向實體記憶體管理器要一頁來當作 Page Table
        uint32_t pt_phys = (uint32_t)pmm_alloc_page();
        uint32_t* pt_virt = (uint32_t*)pt_phys;

        // 清空新分頁表
        for (int i = 0; i < 1024; i++) pt_virt[i] = 0;

        // 寫入目錄 (Present | R/W | User)
        page_directory[pd_idx] = pt_phys | 7;
    }

    // 寫入分頁表 (Present | R/W | User)
    uint32_t* pt = (uint32_t*)(page_directory[pd_idx] & ~0xFFF);
    pt[pt_idx] = phys | 7;
}


void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf"};
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    // [Day51] add -- start
    // 【繪圖基礎建設】映射 Framebuffer 實體記憶體
    if (mbd->flags & (1 << 12)) { // 檢查 GRUB 是否成功回傳 Framebuffer
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;

        // 算出畫布總容量 (約 1.92 MB)，把它逐頁 Map 進 Kernel 宇宙裡！
        uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            // // 實體位址與虛擬位址 1:1 映射，權限設為 3 (Ring 0/3 皆可存取)
            // map_page((uint32_t)fb_addr + i, (uint32_t)fb_addr + i, 3);
            // 【關鍵修改】改用我們自己寫的無敵映射函數！
            map_vram((uint32_t)fb_addr + i, (uint32_t)fb_addr + i);
        }

        // ==========================================
        // 【關鍵修復】重新載入 CR3，強制刷新 TLB 快取！
        // ==========================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }
    // [Day51] add -- end

    __asm__ volatile ("sti");

    // [Day51] add -- start
    // ==============================================================
    // 🎨 畫圖時間！畫一個 200x200 的藍色正方形！
    // ==============================================================
    for (int y = 100; y < 300; y++) {
        for (int x = 100; x < 300; x++) {
            put_pixel(x, y, 0x0000FF); // 0x0000FF = 藍色
        }
    }

    // 畫一顆紅色的太陽在右上角
    for (int y = 50; y < 150; y++) {
        for (int x = 600; x < 700; x++) {
            put_pixel(x, y, 0xFF0000); // 0xFF0000 = 紅色
        }
    }
    // [Day51] add -- end

    while (1) { __asm__ volatile ("hlt"); }
}
```

---

paging.c
```c
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
// [Day43] 新增初代宇宙的 Heap 表
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 【神級捷徑】預先分配 16 個宇宙的空間！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
// [Day43] 新增平行宇宙的 Heap 表陣列
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));
int next_universe_id = 0;

// 在全域變數區新增一個陣列，記錄哪個宇宙被使用了
int universe_used[16] = {0};

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; } // 清空

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;

    // [Day43] 掛載 0x10000000 區域 (pd_idx = 64)
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
        // [Day43] 【Heap 區】 (0x10000000)
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
    // if (next_universe_id >= 16) {
    //     kprintf("Error: Max universes reached!\n");
    //     while(1) __asm__ volatile("hlt");
    // }
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

    // int id = next_universe_id++;
    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id]; // [Day43] 拿出這個宇宙專屬的 Heap 表

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0; // [Day43] 初始化清空
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    // [Day43] 將這張全新的 Heap 表掛載到新宇宙的 0x10000000 區段
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}


// [Day46]【新增】提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {
            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}
```

---
kheap.c
```c
#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "tty.h"

block_header_t* first_block = 0;

// --- 公開 API ---

void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // 【升級】從 32 個分頁擴建到 512 個分頁 (512 * 4KB = 2 MB)！
    for (int i = 0; i < 512; i++) {
        // 加上 (uint32_t) 強制轉型，消除編譯警告
        uint32_t phys_addr = (uint32_t) pmm_alloc_page();
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // 在這塊巨大的平原最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (512 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身
    first_block->is_free = 1;
    first_block->next = 0;
}

void* kmalloc(uint32_t size) {
    // 記憶體對齊：為了硬體效能，申請的大小必須是 4 的倍數
    uint32_t aligned_size = (size + 3) & ~3;

    block_header_t* current = first_block;

    while (current != 0) {
        if (current->is_free && current->size >= aligned_size) {
            // 【關鍵修復：切割邏輯】
            // 如果這塊空地很大，我們不能全給他，要把剩下的切出來當新空地！
            if (current->size > aligned_size + sizeof(block_header_t) + 16) {
                // 計算新地契的位置
                block_header_t* new_block = (block_header_t*)((uint32_t)current + sizeof(block_header_t) + aligned_size);
                new_block->is_free = 1;
                new_block->size = current->size - aligned_size - sizeof(block_header_t);
                new_block->next = current->next;

                current->size = aligned_size;
                current->next = new_block;
            }

            // 把地契標記為使用中，並回傳可用空間的起始位址
            current->is_free = 0;
            return (void*)((uint32_t)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    kprintf("PANIC: Kernel Heap Out of Memory! (Req: %d bytes)\n", size);
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return;

    // 往回退一點，找到這塊地的地契，把它標記為空閒
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    block->is_free = 1;

    // (在簡單版 OS 中，我們暫時省略把相鄰空地合併的邏輯，128KB 夠我們隨便花了)
}
```

---
kheap.h
```c
#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

// 記憶體區塊的標頭 (類似地契，記錄這塊地有多大、有沒有人住)
typedef struct block_header {
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

// 初始化 Kernel Heap
void init_kheap(void);

// 動態配置指定大小的記憶體 (類似 malloc)
void* kmalloc(size_t size);

// 釋放記憶體 (類似 free)
void kfree(void* ptr);

#endif
```

---
paging.h
```c
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void init_paging(void);

// 將特定的實體位址 (phys) 映射到虛擬位址 (virt)
// flags 是權限標籤 (例如：3 代表 Present + Read/Write)
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

// [Day38] 建立一個全新的 Page Directory (多元宇宙)
// uint32_t create_page_directory();

#endif
```

---
pmm.h
```c
// PMM: Physical Memory Management
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

#define PMM_FRAME_SIZE 4096 // 每塊地的大小是 4KB

// 初始化 PMM，傳入系統總記憶體大小 (以 KB 為單位)
void init_pmm(uint32_t mem_size_kb);

// 索取一塊 4KB 的實體記憶體，回傳其記憶體位址
void* pmm_alloc_page(void);

// 釋放一塊 4KB 的實體記憶體
void pmm_free_page(void* ptr);

#endif
```

---
pmm.c
```c
// PMM: Physical Memory Management
#include "pmm.h"
#include "utils.h"
#include "tty.h"

// 假設我們最多支援 4GB RAM (總共 1,048,576 個 Frames)
// 1048576 bits / 32 bits (一個 uint32_t) = 32768 個陣列元素
#define BITMAP_SIZE 32768
uint32_t memory_bitmap[BITMAP_SIZE];

uint32_t max_frames = 0; // 系統實際擁有的可用 Frame 數量

// --- 內部輔助函式：操作特定的 Bit ---

// 將第 frame_idx 個 bit 設為 1 (代表已使用)
static void bitmap_set(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] |= (1 << bit_offset);
}

// 將第 frame_idx 個 bit 設為 0 (代表釋放)
static void bitmap_clear(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] &= ~(1 << bit_offset);
}

// 檢查第 frame_idx 個 bit 是否為 1
static bool bitmap_test(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    return (memory_bitmap[array_idx] & (1 << bit_offset)) != 0;
}

// 尋找第一個為 0 的 bit (第一塊空地)
static int bitmap_find_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        // 如果這個整數不是 0xFFFFFFFF (代表裡面至少有一個 bit 是 0)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame_idx = i * 32 + j;
                if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量

                if (!bitmap_test(frame_idx)) {
                    return frame_idx;
                }
            }
        }
    }
    return -1; // 記憶體全滿 (Out of Memory)
}

// --- 公開 API ---

void init_pmm(uint32_t mem_size_kb) {
    // 總 KB 數 / 4 = 總共有多少個 4KB 的 Frames
    max_frames = mem_size_kb / 4;

    // 一開始先把所有的記憶體都標記為「已使用」(填滿 1)
    // 這是為了安全，避免我們不小心分配到不存在的硬體空間
    memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));

    // 然後，我們只把「真實存在」的記憶體標記為「可用」(設為 0)
    for (uint32_t i = 0; i < max_frames; i++) {
        bitmap_clear(i);
    }

    // [極度重要] 保留系統前 4MB (0 ~ 1024 個 Frames) 不被分配！
    // 因為這 4MB 已經被我們的 Kernel 程式碼、VGA 記憶體、IDT/GDT 給佔用了
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
}

void* pmm_alloc_page(void) {
    int free_frame = bitmap_find_first_free();
    if (free_frame == -1) {
        kprintf("PANIC: Out of Physical Memory!\n");
        return NULL; // OOM
    }

    // 標記為已使用
    bitmap_set(free_frame);

    // 計算出實際的物理位址 (Frame 索引 * 4096)
    uint32_t phys_addr = free_frame * PMM_FRAME_SIZE;
    return (void*)phys_addr;
}

void pmm_free_page(void* ptr) {
    uint32_t phys_addr = (uint32_t)ptr;
    uint32_t frame_idx = phys_addr / PMM_FRAME_SIZE;

    bitmap_clear(frame_idx);
}

```


---
linker.ld
```linker
ENTRY(_start)

SECTIONS {
    /* 核心載入起始位址：1MB */
    . = 1M;

    /* 將 multiboot 標頭放在最前面，這非常重要，否則 Bootloader 找不到 */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* 唯讀資料區段 */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* 可讀寫資料區段 */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* 未初始化資料區段 */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
```
