哈哈，Rick！我們終於來到大魔王關卡的最後一個房間了！🔥

你貼上的這份 Core Dump 簡直就是一份完美的驗屍報告。請看它留下的最後一句遺言：
```text
CR0=80000011 CR2=bffff000 CR3=0010f000
check_exception old: 0xffffffff new 0xe
```
又是 **Page Fault (0x0E)**，而且 `CR2` 剛好等於 **`0xBFFFF000`**！

這不就是我們在 `create_page_directory` 裡面用來「臨時映射」新分頁目錄的位址嗎？

### 🔍 案發現場解密：瞎眼的「造物主」

我們來還原一下案發經過：
1. 當你執行 `echo hello` 時，`sys_exec` 呼叫了 `create_page_directory()` 準備創造新宇宙。
2. 在 `create_page_directory` 裡，我們呼叫了 `map_page(0xBFFFF000, pd_phys, 3);`。
3. **但是！** 你回去看 `map_page` 的實作，它只有支援 `pd_idx` 為 0 (核心), 32 (App), 512, 768 這幾個固定的區域。
4. 當 `map_page` 看到 `0xBFFFF000`（pd_idx 是 767）時，它根本不知道這是哪裡，於是它印出一行錯誤就直接 **`return;`** 罷工了！
5. 接下來我們天真地執行 `uint32_t* new_pd = (uint32_t*) 0xBFFFF000;` 並試圖寫入時，因為這塊記憶體根本沒被對應，CPU 當場引發 Page Fault 暴斃！

不僅如此，就算我們修好了這個臨時映射，我們還有一個更恐怖的隱患：**我們的 `map_page` 是一個瞎子！**
在原本的寫法裡：
```c
if (pd_idx == 32) page_table = user_page_table;
```
它硬生生地寫死了永遠去修改 Kernel 的 `user_page_table`！這代表即使你在 `sys_exec` 切換了 `CR3`（新宇宙），接著 `elf_load` 或配置堆疊時呼叫 `map_page`，它還是傻傻地跑去修改老爸的宇宙！最後新宇宙裡什麼都沒有，一跳過去執行就必定死機。

### 🗡️ 終極修復：打造「宇宙感知」的地圖系統

為了繞過繁瑣的遞迴映射 (Recursive Paging) 黑魔法，在我們 32-bit 的輕量級 OS 中，有一個極度聰明的超級捷徑：

**「只要我們把所有的宇宙（Page Directory 與 Page Table）都宣告在 Kernel 的 BSS 區段（0-4MB），因為這個區段是實體與虛擬 1:1 映射的，所以它們的實體位址就等於虛擬位址！我們隨時都可以用 C 語言的指標直接修改它們，完全不需要臨時映射！」**

請打開 **`lib/paging.c`**，徹底覆蓋成這個版本。
這個版本不僅拿掉了所有容易出錯的臨時映射，還讓 `map_page` 學會了**從目前的 `CR3` 自動找出正確的宇宙來修改**！

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
```

這個改動徹底拔掉了臨時映射引發的未爆彈，並且讓 `map_page` 能自動跟隨 `CR3` 切換狀態。

存檔後，最後一次敲下 `make clean && make run`！
這一次，當你輸入 `echo hello`，系統將為它建立專屬的平行宇宙。在裡面，它獨佔著神聖的 `0x08048000` 執行完畢，然後完美退出，把螢幕和鍵盤交還給位於另一個宇宙的老爸 Shell。

見證這場 MMU 的魔法吧！🌟
