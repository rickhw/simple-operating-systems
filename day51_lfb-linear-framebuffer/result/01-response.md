哈哈，Rick！歡迎來到 Phase 6 的視覺新世界！🎉

看到這個 Core Dump，我反而覺得這是一個「成功的失敗」！
為什麼呢？讓我們來當一下系統法醫，解剖這個 QEMU 的錯誤日誌：

### 🔍 案發現場解密：被大腦快取欺騙的 CPU

仔細看你的錯誤日誌：
`v=0e e=0002 ... CR2=fd04e390`
`v=0e` 代表 **Page Fault (缺頁錯誤)**！而引發錯誤的位址是 `CR2 = 0xFD04E390`。

這個位址非常特別：
* 你的顯示卡 Framebuffer 被 GRUB 安排在實體記憶體的超高空：`0xFD000000`。
* 我們的程式試圖畫第一個點 (x=100, y=100)。
* 偏移量剛好是 $100 \times 3200 + 100 \times 4 = 320400$ (十六進位正好是 `0x4E390`)！
* `0xFD000000 + 0x4E390 = 0xFD04E390`。

這證明了你的指標運算完全正確，GRUB 也真的把畫布交給你了！
**那為什麼會 Page Fault？我們明明用 `map_page` 把整塊畫布都 Map 進去了啊？**

這就是 MMU (記憶體管理單元) 的經典陷阱：**TLB (轉譯後備緩衝區) 快取未刷新！**
當我們用 `map_page` 新增了將近 2MB 的虛擬記憶體映射時，我們修改了 Page Directory。但是，CPU 的大腦裡還存著舊的快取，它認為 `0xFD000000` 這段空間是「不存在」的。當你立刻去寫入它時，CPU 連查都不查最新的 Page Directory，就直接丟出 Page Fault 當機了。

---

### 🛠️ 修復步驟 1：強制刷新 CPU 的大腦 (`lib/kernel.c`)

要解決這個問題，我們只要在 Map 完記憶體後，重新載入一次 `CR3` 暫存器（存放 Page Directory 位址的地方），這會強迫 CPU 忘掉舊的快取，重新認識這個新世界！

請打開 **`lib/kernel.c`**，在 mapping 迴圈的下方加入這兩行神聖的組合語言：

```c
    // 【繪圖基礎建設】映射 Framebuffer 實體記憶體
    if (mbd->flags & (1 << 12)) { 
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;

        uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            map_page((uint32_t)fb_addr + i, (uint32_t)fb_addr + i, 3);
        }
        
        // ==========================================
        // 【關鍵修復】重新載入 CR3，強制刷新 TLB 快取！
        // ==========================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }
```

---

### 🛠️ 步驟 2：確保 GRUB 聽話 (`grub.cfg`)

你問到 `grub.cfg` 需要改嗎？**強烈建議要改！**

雖然我們在 `boot.S` 的 Multiboot Header 裡面許願要 `800x600x32`，但是有些版本的 GRUB 2 會忽略這個請求，除非你在 `grub.cfg` 裡面明確下達指令。如果 GRUB 給了我們錯誤的解析度，我們的 `put_pixel` 就會畫錯位置甚至再度越界當機。

請打開你的 **`grub.cfg`**，在載入 kernel 之前加入 `set gfxpayload`：

```text
menuentry "Simple OS" {
    # 【新增】強制 GRUB 切換到 800x600 32-bit 圖形模式！
    set gfxpayload=800x600x32
    
    multiboot /boot/myos.bin
    module /boot/shell.elf
    module /boot/echo.elf
    # ... 其他 modules 保持不變 ...
}
```

---

存檔，然後打下：
```bash
make clean && make run
```

這一次，CPU 會乖乖地認得這塊超巨大的畫布記憶體。你的 QEMU 視窗將會瞬間拉大到 800x600，並且在黑色的夜空中，綻放出屬於你的藍色與紅色像素方塊！

這可是我們寫 OS 以來，第一次脫離字元、真正控制每一顆發光二極體的瞬間喔！成功亮起來後拍張照給我看看！🚀
