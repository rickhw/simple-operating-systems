
連續輸入 ls 十次，第九次的時候，就會出現 `PANIC: Out of Physical Memory`，這時候還沒掛掉，所以我抓得到圖。
在下一次 ls 就 core dump 了！

另外有一個怪問題，打 pwd 的時候會出現 command not found. 有點奇怪

附上可能需要調整的 source code.


```bash
5444: v=21 e=0000 i=0 cpl=3 IP=001b:08048000 pc=08048000 SP=0023:083fffa8 env->regs[R_EAX]=00000000
EAX=00000000 EBX=083f4dd0 ECX=083f4d90 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083f4d48 ESP=083fffa8
EIP=08048000 EFL=00010202 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010e000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010e080 0000002f
IDT=     002e2f60 000007ff
CR0=80000011 CR2=00000000 CR3=00305000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=c0006ee8 CCO=EFLAGS
EFER=0000000000000000
check_exception old: 0xffffffff new 0xe
 5445: v=0e e=0004 i=0 cpl=3 IP=001b:08048004 pc=08048004 SP=0023:083fffa8 CR2=083f4e39
EAX=00000000 EBX=083f4dd0 ECX=083f4d90 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083f4d48 ESP=083fffa8
EIP=08048004 EFL=00000246 [---Z-P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010e000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010e080 0000002f
IDT=     002e2f60 000007ff
CR0=80000011 CR2=083f4e39 CR3=00305000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000000 CCO=ADDB
EFER=0000000000000000
check_exception old: 0xe new 0xd
 5446: v=08 e=0000 i=0 cpl=3 IP=001b:08048004 pc=08048004 SP=0023:083fffa8 env->regs[R_EAX]=00000000
EAX=00000000 EBX=083f4dd0 ECX=083f4d90 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083f4d48 ESP=083fffa8
EIP=08048004 EFL=00000246 [---Z-P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010e000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010e080 0000002f
IDT=     002e2f60 000007ff
CR0=80000011 CR2=083f4e39 CR3=00305000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000000 CCO=ADDB
EFER=0000000000000000
check_exception old: 0x8 new 0xd
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
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
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
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {
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

    // 從 32 個分頁擴建到 512 個分頁 (512 * 4KB = 2 MB)！
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
#include "gfx.h"
#include "mouse.h"
#include "gui.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf", "ps.elf", "top.elf", "kill.elf"};
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
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
shell.c
```c
#include "stdio.h"
#include "unistd.h"

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == '\n') {
            break; // 使用者按下了 Enter
        } else if (c == '\b') {
            if (i > 0) i--; // 處理倒退鍵 (Backspace)
        } else {
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0'; // 字串結尾
}


int parse_args(char* input, char** argv) {
    int argc = 0, i = 0;
    int in_word = 0;
    int in_quote = 0; // 新增狀態：是否在雙引號內

    while (input[i] != '\0') {
        if (input[i] == '"') {
            if (in_quote) {
                input[i] = '\0'; // 遇到結尾引號，斷開字串
                in_quote = 0;
                in_word = 0;
            } else {
                in_quote = 1;
                argv[argc++] = &input[i + 1]; // 指向引號的下一個字元
                in_word = 1;
            }
        }
        else if (input[i] == ' ' && !in_quote) {
            // 只有在「引號外面」的空白，才會斷開字串
            input[i] = '\0';
            in_word = 0;
        }
        else {
            if (!in_word && !in_quote) {
                argv[argc++] = &input[i];
                in_word = 1;
            }
        }
        i++;
    }
    argv[argc] = 0;
    return argc;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("\n======================================\n");
        printf("      Welcome to Simple OS Shell!     \n");
        printf("======================================\n");
    } else {
        printf("Awesome! I received %d arguments:\n", argc);
        for(int i = 0; i < argc; i++) {
            printf("  Arg %d: %s\n", i, argv[i]);
        }
        printf("\n");
    }

    char cmd_buffer[128];

    while (1) {
        printf("SimpleOS> ");
        read_line(cmd_buffer, 128);
        if (cmd_buffer[0] == '\0') continue;

        // 解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        char* cmd = parsed_argv[0];

        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            printf("Built-in commands: help, about, ipc, cd, pwd, exit, pid, desktop\n");
            printf("External commands: echo, cat, ping, pong, ls, touch, mkdir, ps, top\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            printf("Simple OS v1.0 - Dynamic Shell Edition\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            printf("Goodbye!\n");
            exit();
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (parsed_argc < 2) {
                printf("Usage: cd <directory>\n");
            } else {
                if (chdir(parsed_argv[1]) != 0) {
                    printf("cd: %s: No such directory\n", parsed_argv[1]);
                }
            }
        }
        else if (strcmp(cmd, "pwd") == 0) {
            char path_buf[256];
            getcwd(path_buf);
            printf("%s\n", path_buf);
        }
        // 檢查是不是 desktop 指令
        if (strcmp(cmd, "desktop") == 0) {
            set_display_mode(1); // 呼叫 syscall，要求 Kernel 切換到 GUI
            continue; // 直接迴圈，不用執行其他程式
        }
        else if (strcmp(cmd, "pid") == 0) {
            printf("Shell PID: %d\n", getpid());
        }
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            printf("\n--- Starting IPC Queue Test ---\n");

            // 1. 創造 Pong (收信者) - 讓它準備連續收兩封信！
            int pid_pong = fork();
            if (pid_pong == 0) {
                char* args[] = {"pong.elf", 0};
                exec("pong.elf", args);
                exit();
            }

            yield(); // 讓 Pong 先去待命

            // 2. 創造 第一個 Ping
            int pid_ping1 = fork();
            if (pid_ping1 == 0) {
                char* args[] = {"ping.elf", 0};
                exec("ping.elf", args);
                exit();
            }

            // 3. 創造 第二個 Ping
            int pid_ping2 = fork();
            if (pid_ping2 == 0) {
                // 為了區分，我們假裝它是另一個程式，但其實跑一樣的 logic，只是行程 ID 不同
                char* args[] = {"ping.elf", 0};
                exec("ping.elf", args);
                exit();
            }

            // 4. 老爸等待所有人結束
            wait(pid_pong);
            wait(pid_ping1);
            wait(pid_ping2);
            printf("--- IPC Test Finished! ---\n\n");
        }
        // 動態執行外部程式
        else {
            // 自動幫指令加上 .elf 副檔名
            char elf_name[32];
            char* dest = elf_name;
            char* src = cmd;
            while(*src) *dest++ = *src++;
            *dest++ = '.'; *dest++ = 'e'; *dest++ = 'l'; *dest++ = 'f'; *dest = '\0';

            int pid = fork();
            if (pid == 0) {
                int err = exec(elf_name, parsed_argv);
                if (err == -1) {
                    printf("Command not found: ");
                    printf(cmd);
                    printf("\n");
                }
                exit();
            } else {
                wait(pid);
            }
        }
    }
    return 0;
}
```
