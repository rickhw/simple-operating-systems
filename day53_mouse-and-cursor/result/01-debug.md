跑起來了，有看到一個滑鼠在畫面中間，不過無法移動 (如圖)

幫我看看 idt.c / interrupts.S 是否漏掉的，或者 IRQ 有錯誤 ?


---

mouse.c
```c
#include <stdint.h>
#include "io.h"
#include "mouse.h"
#include "gfx.h"
#include "tty.h"

// 宣告你現有的 IO 函式 (假設你有 inb 和 outb，如果沒有請在 utils.c 補上)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 滑鼠狀態
static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];
static int mouse_x = 400; // 預設在螢幕正中間
static int mouse_y = 300;

// 等待鍵盤控制器就緒
static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

// 寫入指令給滑鼠
static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); // 告訴控制器，下一個 byte 要送給滑鼠
    mouse_wait(1);
    outb(0x60, write);
    mouse_wait(0);
    inb(0x60); // 讀取 ACK 回應
}

// 初始化 PS/2 滑鼠
void init_mouse(void) {
    kprintf("[Mouse] Initializing PS/2 Mouse...\n");

    // 1. 啟用附屬裝置 (Mouse)
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 2. 啟用中斷 (IRQ 12)
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // 3. 設定滑鼠預設值並開始回報資料
    mouse_write(0xF6);
    mouse_write(0xF4);

    // 畫出初始游標
    draw_cursor(mouse_x, mouse_y);
}

// 【滑鼠中斷處理程式】當滑鼠移動或點擊時，IRQ 12 會呼叫這裡！
void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { // 檢查是否有資料可讀
        int8_t mouse_in = inb(0x60);

        // 滑鼠每次傳送 3 個 bytes (封包)
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { // 檢查封包合法性 (bit 3 必須是 1)
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
                break;
            case 1:
                mouse_byte[1] = mouse_in;
                mouse_cycle++;
                break;
            case 2:
                mouse_byte[2] = mouse_in;
                mouse_cycle = 0;

                // 取出 X 和 Y 的位移量
                int dx = mouse_byte[1];
                int dy = mouse_byte[2];

                // 更新座標 (Y 軸在電腦螢幕是往下遞增，所以要減 dy)
                mouse_x += dx;
                mouse_y -= dy;

                // 邊界碰撞測試 (避免游標跑出螢幕)
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790; // 保留 10 像素給游標本身
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                // 重畫游標！
                draw_cursor(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); // 繼續讀取直到清空
    }
}
```


---
gfx.c
```c
#include "gfx.h"
#include "paging.h"
#include "tty.h"
#include "font8x8.h"

// 圖形引擎的私有狀態
static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

// [Day53] add -- start
// 滑鼠游標系統 (Mouse Cursor System)
static int cursor_x = 400;
static int cursor_y = 300;
static uint32_t cursor_bg[100]; // 儲存游標底下的 10x10 背景像素

// 10x10 游標點陣圖 (1: 白色邊框, 2: 黑色填滿, 0: 透明)
static const uint8_t cursor_bmp[10][10] = {
    {1,1,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,1,0,0},
    {1,2,2,1,1,1,1,1,1,0},
    {1,1,1,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,0,0,0}
};
// [Day53] add -- end

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


// [Day52] add -- start
// 用像素點陣畫出一個 ASCII 字元
void draw_char(char c, int x, int y, uint32_t fg_color, uint32_t bg_color) {
    // 過濾掉我們字型庫沒有的控制字元或怪異字元
    if (c < 32 || c > 126) return;

    // 取得這個字母對應的 8 Bytes 像素點陣圖 (ASCII 32 對應陣列第 0 個)
    const uint8_t* glyph = font8x8[c - 32];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // 利用位元遮罩 (Bitmask) 檢查這個像素是 1 還是 0
            // 字型點陣通常是從左到右，最高位元 (0x80) 在最左邊
            if (glyph[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, fg_color); // 畫筆畫上字型顏色
            } else {
                put_pixel(x + col, y + row, bg_color); // 塗上背景色 (蓋掉原本的畫面)
            }
        }
    }
}
// [Day52] add -- end


// [Day53] add -- start
// 【新增】取得螢幕上特定座標的像素顏色
uint32_t get_pixel(int x, int y) {
    if (fb_addr == 0 || x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return 0;
    uint32_t offset = (y * fb_pitch) + (x * (fb_bpp / 8));
    // 組合 BGRA 成為 32-bit 顏色
    uint32_t color = fb_addr[offset] | (fb_addr[offset + 1] << 8) | (fb_addr[offset + 2] << 16);
    return color;
}

// 【新增】畫出滑鼠游標 (並處理背景還原)
void draw_cursor(int new_x, int new_y) {
    // 1. 先把「舊位置」的背景還原
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            put_pixel(cursor_x + j, cursor_y + i, cursor_bg[i * 10 + j]);
        }
    }

    // 2. 更新最新座標
    cursor_x = new_x;
    cursor_y = new_y;

    // 3. 備份「新位置」的背景，然後畫上游標
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            // 備份背景
            cursor_bg[i * 10 + j] = get_pixel(cursor_x + j, cursor_y + i);

            // 畫游標
            if (cursor_bmp[i][j] == 1) put_pixel(cursor_x + j, cursor_y + i, 0xFFFFFF); // 白邊
            else if (cursor_bmp[i][j] == 2) put_pixel(cursor_x + j, cursor_y + i, 0x000000); // 黑底
        }
    }
}
// [Day53] add -- end
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

    // 啟動圖形引擎！
    init_gfx(mbd);
    init_mouse();

    __asm__ volatile ("sti");

    // [Day51] add -- start
    // 用全新的 API 畫出我們的藍色與紅色方塊！
    // draw_rect(100, 100, 200, 200, 0x0000FF); // 藍色方塊
    // draw_rect(600, 50, 100, 100, 0xFF0000);  // 紅色方塊
    // [Day51] add -- end

    kprintf("=== OS Subsystems Ready ===\n\n");

    while (1) { __asm__ volatile ("hlt"); }
}
```

---

idt.c
```c
// IDT: Interrupt Descriptor Table
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
// PIC: [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0();     // 第 0  號中斷：的 Assembly 進入點
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr44();    // 第 44 號中斷：Mouse Handler
extern void isr128();   // 第 128 號中斷：system calls

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
void pic_remap() {
    // 儲存原本的遮罩 (Masks)
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; (void)a2; // 消除 unused variable 警告

    // 開始初始化序列 (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定 Master PIC 的偏移量為 0x20 (十進位 32)
    outb(0x21, 0x20);
    // ICW2: 設定 Slave PIC 的偏移量為 0x28 (十進位 40)
    outb(0xA1, 0x28);

    // ICW3: 告訴 Master PIC 有一個 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: 告訴 Slave PIC 它的串聯身份
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 我們目前只開啟 IRQ1 (鍵盤)，關閉其他所有硬體中斷 (避免 Timer 狂噴中斷干擾我們)
    // 0xFD 的二進位是 1111 1101 (第 1 個 bit 是 0，代表開啟鍵盤)
    // 0xFC 的二進位是 1111 1100 (第 0 和第 1 個 bit 都是 0，代表開啟 Timer 與 Keyboard)
    outb(0x21, 0xFC); // [修改] 從 0xFD 變成 0xFC
    outb(0xA1, 0xFF);
}


// --- 公開 API ---

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // [新增] 重新映射 PIC
    pic_remap();

    // [新增] 掛載第 32 號中斷 (IRQ0 Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);

    // [新增] 掛載第 33 號中斷 (IRQ1 鍵盤)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // [Day53] 掛載第 44 號中斷 (IRQ1 Mouse)
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);

    // [新增] 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}
```

---

interrupts.S
```asm
; [目的] 中斷處理與 IDT 載入
; [流程 - 以 System Call 為例]
; 使用者執行 int 0x128
;    |
;    V
; [ isr128 ]
;    |--> [ pusha ] (備份所有通用暫存器)
;    |--> [ call syscall_handler ] (執行 C 處理邏輯)
;    |--> [ popa ] (恢復暫存器)
;    |--> [ iret ] (返回使用者模式)

global idt_flush

global isr0
extern isr0_handler

global isr32
extern timer_handler

global isr33
extern keyboard_handler

global isr44
extern mouse_handler

global isr128
extern syscall_handler

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

isr0:
    cli
    call isr0_handler
    sti
    iret

isr32:
    pusha
    call timer_handler
    popa
    iret

isr33:
    pusha
    call keyboard_handler
    popa
    iret

; [Day53] Mouse handler
isr44:
    pusha
    call mouse_handler
    popa
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    ; 魔法：syscall_handler 已經把回傳值寫進堆疊的 EAX 位置了
    popa            ; 恢復所有暫存器
    iret

```
