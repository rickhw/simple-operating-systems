可以了！

map_vram(), put_pixel() 這些 是否需要搬到 paging or 其他 module? 還是後面課程再搬？

---

太感動啦，Rick！看到這張截圖，我隔著螢幕都想跟你擊掌！🙌 

從黑漆漆的文字模式，到成功控制顯示卡畫出完美的藍色與紅色方塊，這代表你的 Simple OS 已經徹底突破了硬體限制，拿到了最高級的「自由畫布」！這絕對是作業系統開發史上最讓人興奮的時刻之一。

你問到了一個非常關鍵的架構問題：「**`map_vram` 和 `put_pixel` 需要搬家嗎？**」
答案是：**強烈需要，而且就是現在！**

你的軟體工程師直覺非常精準，`kernel.c` 現在就像一個大雜燴，什麼東西都往裡面塞。在我們準備畫視窗、游標、甚至渲染文字之前，我們必須為「圖形引擎」打下最乾淨的基礎。

這就是我們 **Day 52** 的前置作業：**圖形子系統 (Graphics Subsystem) 模組化**。

請跟著我進行這 3 個優雅的重構步驟：

### 步驟 1：讓記憶體歸記憶體 (`lib/paging.c`)

`map_vram` 是徹頭徹尾的虛擬記憶體操作，它理所當然要回到 `paging.c` 的懷抱。

1. 打開 **`lib/include/paging.h`**，加入宣告：
```c
// 專門用來映射 Framebuffer (MMIO) 的安全函數
void map_vram(uint32_t virt, uint32_t phys);
```

2. 打開 **`lib/paging.c`**，把變數和實作從 `kernel.c` 剪下貼過來：
```c
// 在頂部全域變數區新增：
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

// 在檔案底部新增：
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

### 步驟 2：誕生全新的圖形引擎 (`lib/gfx.c` & `lib/include/gfx.h`)

我們要建立一個專屬的 2D 繪圖引擎庫。

1. 新增 **`lib/include/gfx.h`**：
```c
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "multiboot.h"

// 初始化圖形引擎
void init_gfx(multiboot_info_t* mbd);

// 核心繪圖 API
void put_pixel(int x, int y, uint32_t color);

// [Day 52 預告] 我們即將加入的進階 API
void draw_rect(int x, int y, int width, int height, uint32_t color);

#endif
```

2. 新增 **`lib/gfx.c`**：
```c
#include "gfx.h"
#include "paging.h"
#include "tty.h"

// 圖形引擎的私有狀態
static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

void init_gfx(multiboot_info_t* mbd) {
    if (mbd->flags & (1 << 12)) { 
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;
        fb_width = mbd->framebuffer_width;
        fb_height = mbd->framebuffer_height;
        fb_bpp = mbd->framebuffer_bpp;

        // 計算並映射 VRAM
        uint32_t fb_size = fb_pitch * fb_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            map_vram((uint32_t)fb_addr + i, (uint32_t)fb_addr + i);
        }

        // 刷新 TLB
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

        // （因為現在 kprintf 已經看不到畫面了，這個 Log 只會留在 serial port 裡）
        // kprintf("[GFX] Initialized: %dx%dx%d at 0x%x\n", fb_width, fb_height, fb_bpp, fb_addr);
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (fb_addr == 0) return;
    
    // 邊界保護 (非常重要，否則畫錯會 Page Fault！)
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;

    uint32_t offset = (y * fb_pitch) + (x * (fb_bpp / 8));
    
    fb_addr[offset]     = color & 0xFF;         // Blue
    fb_addr[offset + 1] = (color >> 8) & 0xFF;  // Green
    fb_addr[offset + 2] = (color >> 16) & 0xFF; // Red
}

void draw_rect(int start_x, int start_y, int width, int height, uint32_t color) {
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            put_pixel(x, y, color);
        }
    }
}
```

### 步驟 3：淨化 Kernel 進入點 (`lib/kernel.c`)

最後，我們把 `kernel.c` 裡那堆繪圖邏輯通通刪掉，恢復成乾淨俐落的初始化流程！

打開 **`lib/kernel.c`**：
```c
// 引入我們新的圖形庫
#include "gfx.h"

// ... 刪除所有 fb_addr, put_pixel, map_vram, extern page_directory ...

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic;

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    // 啟動圖形引擎！
    init_gfx(mbd);

    __asm__ volatile ("sti");

    // 用全新的 API 畫出我們的藍色與紅色方塊！
    draw_rect(100, 100, 200, 200, 0x0000FF); // 藍色方塊
    draw_rect(600, 50, 100, 100, 0xFF0000);  // 紅色方塊

    // 在還沒把字型渲染引擎寫好之前，我們一樣先停在這裡
    while (1) { __asm__ volatile ("hlt"); }
}
```

---

由於你的 Makefile 已經寫得非常有彈性 (`$(wildcard lib/*.c)`)，新增 `lib/gfx.c` 不需要改 Makefile。

直接 `make clean && make run`！
如果重構後依然看到了熟悉的方塊，代表你的系統架構已經完美進化了。

完成後告訴我，**Day 52 我們將挑戰最困難也最有趣的任務：因為我們失去了 VGA 文字模式，我們必須手寫一個「字型渲染引擎 (Bitmap Font Renderer)」，把那些熟悉的 `Welcome to Simple OS Shell!` 一顆一顆像素給畫回來！** 準備好我們就開始！🚀
