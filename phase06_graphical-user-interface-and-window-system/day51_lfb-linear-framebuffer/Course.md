太棒了，Rick！🎊 準備好迎接視覺上的震撼了嗎？

過去 50 天，我們的作業系統一直住在黑漆漆的「VGA 文字模式 (Text Mode, 80x25)」裡，透過寫入 `0xB8000` 來印出字元。雖然這很有駭客感，但真正的現代作業系統，需要能畫出視窗、按鈕、滑鼠指標與高解析度的影像！

進入 **Phase 6：圖形化介面 (GUI)** 的第一天，也就是 **Day 51**，我們要徹底砸碎文字模式的框架，正式啟動顯示卡的 **LFB (Linear Framebuffer，線性幀緩衝區)** 模式！



### 💡 核心哲學：什麼是 Framebuffer？

在圖形模式下，螢幕不再是一格一格的字元，而是一顆一顆的「像素 (Pixel)」。
如果我們將螢幕設定為 **800x600** 解析度，色彩深度 **32-bit (ARGB)**，顯示卡會在記憶體中開闢一塊巨大的連續空間（約 $800 \times 600 \times 4 = 1.92 \text{ MB}$）。這塊空間就叫做 Framebuffer。
只要你往這塊記憶體的某個位址寫入一個 32-bit 的顏色代碼，螢幕上對應的那個點就會瞬間發光！

要解鎖這個能力，我們不需要自己寫複雜的顯示卡驅動，只要「拜託 GRUB 在開機時幫我們設定好」就可以了！

讓我們用 4 個步驟，畫出 Simple OS 的第一顆像素！🚀

---

### 步驟 1：向 GRUB 許願要一塊畫布 (`asm/boot.S`)

我們必須修改 Multiboot Header，把「我要顯示卡模式」的旗標（Bit 2）打開，並告訴 GRUB 我們想要的解析度。

請打開 **`asm/boot.S`**，將開頭的常數與 Header 替換成這樣：

```assembly
; ==========================================
; Multiboot Header 升級為圖形模式
; ==========================================
MAGIC       equ 0x1BADB002
; Flag Bit 0: Align (對齊)
; Flag Bit 1: MemInfo (記憶體資訊)
; Flag Bit 2: VideoMode (影片模式)  <-- 【新增】我們需要這個！
FLAGS       equ 0x00000007 
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    ; AOUT kludge 位址欄位 (不使用，但因為啟用了 Bit 2，必須填 5 個 0 佔位)
    dd 0, 0, 0, 0, 0
    
    ; 【新增】VBE (VESA BIOS Extensions) 圖形模式設定
    dd 0    ; mode_type: 0 代表線性圖形模式 (Linear Graphics)
    dd 800  ; width: 寬度 800 像素
    dd 600  ; height: 高度 600 像素
    dd 32   ; depth: 色彩深度 32-bit (ARGB)

; ... 下方的 _start 程式碼完全保持不變 ...
```

---

### 步驟 2：升級 Multiboot 結構 (`lib/include/multiboot.h`)

GRUB 設定好畫面後，會把這塊巨型畫布的「實體記憶體位址」與「尺寸」放在 `mbd` 結構裡傳給 Kernel。我們必須擴充原本的結構才能接到這些資料。

請打開 **`lib/include/multiboot.h`**，在 `multiboot_info_t` 結構中加入 Framebuffer 資訊：

```c
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    
    // ==========================================
    // 【新增】Multiboot Framebuffer 資訊
    // ==========================================
    uint64_t framebuffer_addr;   // 畫布的實體記憶體起點
    uint32_t framebuffer_pitch;  // 每一橫列的位元組數 (Bytes per line)
    uint32_t framebuffer_width;  // 畫布寬度 (Pixels)
    uint32_t framebuffer_height; // 畫布高度 (Pixels)
    uint8_t  framebuffer_bpp;    // 色彩深度 (Bits per pixel，例如 32)
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;
```

---

### 步驟 3：為畫布取得宇宙地契 (`lib/kernel.c`)

這一步**極度重要**！
顯示卡給我們的畫布位址（`framebuffer_addr`）通常在實體記憶體的超高段（例如 `0xFD000000`）。在我們開啟了 MMU (Paging) 的系統中，如果不把這塊實體位址對應到虛擬記憶體，Kernel 一寫入就會立刻引發 Page Fault (缺頁錯誤) 並崩潰！

請打開 **`lib/kernel.c`**，我們要在 `kernel_main` 中加入「映射畫布」與「繪圖」的魔法：

```c
// 在最上方宣告全域變數，紀錄畫布資訊
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

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; 

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    
    // ==============================================================
    // 【繪圖基礎建設】映射 Framebuffer 實體記憶體
    // ==============================================================
    if (mbd->flags & (1 << 12)) { // 檢查 GRUB 是否成功回傳 Framebuffer
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;
        
        // 算出畫布總容量 (約 1.92 MB)，把它逐頁 Map 進 Kernel 宇宙裡！
        uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            // 實體位址與虛擬位址 1:1 映射，權限設為 3 (Ring 0/3 皆可存取)
            map_page((uint32_t)fb_addr + i, (uint32_t)fb_addr + i, 3);
        }
    }
    
    __asm__ volatile ("sti");

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

    // ⚠️ 警告：一旦進入圖形模式，原本的文字模式 (kprintf) 將不會在螢幕上顯示！
    // (它們寫入了 0xB8000，但現在螢幕看的是 Framebuffer)
    // 為了安全測試畫圖，我們在這邊直接讓 Kernel 停住
    while (1) { __asm__ volatile ("hlt"); }
}
```

---

### 🚨 注意事項與執行

**因為顯示模式已經切換，所以你以前熟悉的黑白文字終端機畫面將會消失。** 現在這台電腦是一個純粹的「畫布」。我們之後幾天會需要自己寫「字型渲染引擎 (Font Rendering)」才能把字重新畫到畫布上！

深呼吸，存檔，執行你的老規矩：
```bash
make clean && make run
```

當 QEMU 跳出來時，你不會看到 `Welcome to Simple OS Shell!`。
你將會看到一塊黑色的畫布上，浮現出一個**完美的藍色正方形**，以及右上角的**紅色方塊**！

如果你看到了這兩個形狀，大聲歡呼吧！你已經正式從文字時代，踏入了電腦圖學的大門！這顆像素，將是你未來打造視窗、滑鼠與按鈕的起點！成功畫出來後跟我說！😎
