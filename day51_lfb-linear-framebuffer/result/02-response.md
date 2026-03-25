哈哈，Rick！看到這個 Core Dump，我反而要大聲恭喜你！🎉

這絕對不是你的邏輯寫錯了，這是一個**超級經典的「C 語言型別陷阱」與「記憶體映射 (MMIO)」**的史詩級碰撞！我們來當一下系統法醫，解剖這個 Core Dump：

### 🔍 案發現場解密：被「負數」欺騙的 CPU 

仔細看你的錯誤日誌：
* `v=0e` 代表 **Page Fault (缺頁錯誤)**。
* `CR2=fd04e390` 代表引發錯誤的記憶體位址是 `0xFD04E390`。

我們來算一下數學：
* 你的畫布起點 `fb_addr` 是 `0xFD000000`。
* 我們的迴圈試圖畫第一個點 (x=100, y=100)。它的偏移量是 `(100 * 3200) + (100 * 4) = 320400`。
* `320400` 的十六進位正好是 **`0x4E390`**！
* `0xFD000000 + 0x4E390 = 0xFD04E390`。

**這證明了兩件事：**
1. GRUB 真的聽懂了 `grub.cfg` 的設定，完美回傳了 800x600x32 的畫布給你！
2. 你的 `put_pixel` 座標與指標運算**完全正確**！

**那為什麼還會 Page Fault 當機呢？**
這代表我們呼叫的 `map_page` **默默地失敗了**，它根本沒有把 `0xFD000000` 寫進 Page Directory (分頁目錄) 裡！



**為什麼 `map_page` 會失敗？通常有兩個致命原因：**
1. **Signed Integer 陷阱 (最有可能)：** `0xFD000000` 這個數字非常大。如果底層的 `map_page` 實作時，參數不小心用到了 `int` (有號整數) 而不是 `uint32_t`，那 `0xFD000000` 就會被當作負數（`-50331648`）。當它進行 `>> 22` 算出 Page Directory Index 時，會產生「算術位移」，算出一個**負數索引 (`-12`)**！結果它把資料寫到了陣列外面的隨機記憶體，導致 `0xFD000000` 根本沒被映射。
2. **實體記憶體保護機制：** 很多 OS 的 `map_page` 會有保護機制 `if (phys >= max_ram) return;`。但顯示卡的 Framebuffer 屬於 MMIO (記憶體映射 I/O)，它不在正常的 RAM 範圍內，所以會被這個保護機制無情拒絕。

---

### 🛠️ 終極修復方案：打造專屬的無敵映射函數

為了解決這個問題，我們不要去動原本的 `map_page`，我們直接在 `kernel.c` 裡面打造一個**專屬於顯示卡、絕對安全的無敵映射引擎**！

請打開 **`lib/kernel.c`**，進行以下 2 個修改：

**1. 在檔案最上方加入專屬的 `map_vram` 函數：**

```c
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
```

**2. 將 `kernel_main` 裡面的 `map_page` 換成 `map_vram`：**

```c
    // 【繪圖基礎建設】映射 Framebuffer 實體記憶體
    if (mbd->flags & (1 << 12)) { 
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;

        uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            // 【關鍵修改】改用我們自己寫的無敵映射函數！
            map_vram((uint32_t)fb_addr + i, (uint32_t)fb_addr + i);
        }

        // ==========================================
        // 重新載入 CR3，強制刷新 TLB 快取！
        // ==========================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }
```

---

存檔！這一次，敲下：
```bash
make clean && make run
```

這個專屬的映射函數保證了我們用最正確的 `uint32_t` 算出 Index `1012`，並且直接寫入 Page Directory。
當 QEMU 重啟後，你一定會看到那塊令人感動的藍色正方形與紅色方塊！成功點亮世界後記得跟我說！🚀
