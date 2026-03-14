準備好了嗎？這將是我們 "Simple OS" 系列最激動人心、也是最硬核的最終章！

歡迎來到 **Day 20：虛實大挪移 — 將 ELF 載入記憶體並在 User Mode 執行**！

在上一堂課，GRUB 已經把 `my_app.elf` 放在了實體記憶體的某個角落（例如 `0x12E000`）。但是，這個執行檔在編譯時，我們對它下了指令 `-Ttext 0x08048000`，這意味著它**堅信**自己會被放在虛擬記憶體的 `0x08048000`（大約 128MB 的位置）。

如果現在直接執行它，CPU 查字典發現 `0x08048000` 根本不存在，就會當場死機。

因此，作業系統載入程式的終極魔法，就是去讀取 ELF 裡面的 **「程式標頭 (Program Headers)」**，找出它需要哪些記憶體，然後呼叫我們的 `map_page`，把那塊無聊的實體記憶體 (`0x12E000`)，**偷偷對應到**應用程式夢寐以求的虛擬門牌號碼 (`0x08048000`)。

---

### Day 20 授課目標

1. **升級分頁字典：** 為 User App 專屬的 `0x08000000` (128MB) 區域準備一張新的分頁表。
2. **實作 ELF Loader：** 讀取 Program Header，施展虛實縫合。
3. **撰寫真實 App：** 寫一個會呼叫 `int 0x80` 的真正 C 語言應用程式檔。
4. **終極跳躍：** 將權限降級，跳入應用程式的進入點！

---

### 實作步驟：最終的拼圖

#### 1. 準備平民專屬的分頁表 (`lib/paging.c`)

打開 `lib/paging.c`。`0x08048000` 除以 4MB 等於 **32**。所以我們要為第 32 號目錄建立一張分頁表，並且給予平民權限（旗標 `7` = Present | RW | User）。

```c
// 1. 在檔案最上方新增第四張分頁表
uint32_t user_page_table[1024] __attribute__((aligned(4096))); 

void init_paging(void) {
    // ... 前面的清零與初始化維持不變 ...

    // [新增] 初始化 User 表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        user_page_table[i] = 0; 
    }

    // [新增] 掛載到目錄的第 32 項 (管理 128MB ~ 132MB 的虛擬空間)
    page_directory[32] = ((uint32_t)user_page_table) | 7; // 7 代表允許 Ring 3 存取
    
    // ... 其他目錄掛載維持不變 ...
}

// 2. 讓 map_page 認得 32 號目錄
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;
    if (pd_idx == 0) page_table = first_page_table;
    else if (pd_idx == 32) page_table = user_page_table;  // [新增] 支援 128MB 區域
    else if (pd_idx == 512) page_table = second_page_table;
    else if (pd_idx == 768) page_table = third_page_table;
    else {
        kprintf("Error: Page table not allocated for pd_idx %d!\n", pd_idx);
        return;
    }
    
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

```

#### 2. 升級 ELF 結構與載入器 (`lib/elf.h` & `lib/elf.c`)

打開 `lib/elf.h`，加入 Program Header 的結構：

```c
// 在 elf.h 中新增：
typedef struct {
    uint32_t type;   // 區段類型 (1 代表需要載入記憶體的 PT_LOAD)
    uint32_t offset; // 該區段在檔案中的偏移量
    uint32_t vaddr;  // [關鍵] 應用程式期望的虛擬位址！
    uint32_t paddr;  // 實體位址 (通常忽略)
    uint32_t filesz; // 區段在檔案中的大小
    uint32_t memsz;  // 區段在記憶體中的大小
    uint32_t flags;  // 讀/寫/執行權限
    uint32_t align;  // 對齊要求
} __attribute__((packed)) elf32_phdr_t;

// 新增 Loader 宣告
uint32_t elf_load(elf32_ehdr_t* header);

```

打開 `lib/elf.c`，實作真正的 Loader：

```c
#include "elf.h"
#include "tty.h"
#include "paging.h" // 需要用到 map_page

// 回傳程式的 Entry Point
uint32_t elf_load(elf32_ehdr_t* header) {
    if (!elf_check_supported(header)) return 0;

    // 計算 Program Header Table 的起始實體位址
    elf32_phdr_t* phdr = (elf32_phdr_t*)((uint8_t*)header + header->phoff);

    // 遍歷所有 Program Headers
    for (int i = 0; i < header->phnum; i++) {
        // type == 1 代表這是 PT_LOAD (需要載入到記憶體的程式碼或資料)
        if (phdr[i].type == 1) { 
            uint32_t virt_addr = phdr[i].vaddr;
            uint32_t phys_addr = (uint32_t)header + phdr[i].offset;
            
            kprintf("[Loader] Mapping Section: Virt 0x%x -> Phys 0x%x\n", virt_addr, phys_addr);

            // 施展魔法：把這個實體位址映射到應用程式期望的虛擬位址！
            // 為了簡單起見，我們假設程式小於 4KB，只映射一頁 (旗標 7 代表 User 可讀寫執行)
            map_page(virt_addr, phys_addr, 7);
        }
    }

    return header->entry;
}

```

#### 3. 核心大總管：執行載入與跳轉 (`lib/kernel.c`)

把昨天測試的 `elf_check_supported` 換成我們新寫的 `elf_load`，並直接呼叫 `enter_user_mode`！

```c
    // ... (前面接收 GRUB 模組的邏輯不變) ...
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        elf32_ehdr_t* real_app = (elf32_ehdr_t*)mod->mod_start;
        
        kprintf("\nLoading ELF Module...\n");
        uint32_t entry_point = elf_load(real_app);
        
        if (entry_point != 0) {
            kprintf("Dropping to Ring 3 and executing app at 0x%x...\n", entry_point);
            
            // 帶入我們 Day 17 寫好的跳板函式！
            extern void enter_user_mode(uint32_t user_function_ptr);
            enter_user_mode(entry_point);
        }
    }

```

#### 4. 撰寫你的第一支獨立應用程式 (`app.c`)

我們昨天寫的 `app.c` 是空的，現在讓它真正做點事情！

請打開或建立專案根目錄的 **`app.c`**：

```c
// 這是完全獨立於 Kernel 的程式碼！沒有 #include 任何核心的標頭檔。

// 我們不叫 main，叫 _start，因為我們沒有標準函式庫 (libc) 幫我們啟動 main。
void _start() {
    // 呼叫我們在 Kernel 寫好的 Syscall 2 (印出字串)
    char* msg = "Hello from an EXTERNAL ELF File running in Ring 3!";
    
    __asm__ volatile (
        "mov $2, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r" (msg) : "eax", "ebx"
    );

    // 應用程式結束後進入無窮迴圈
    while(1);
}

```

---

### 最終編譯與見證奇蹟

現在，開啟終端機，我們要將這支全新的 `app.c` 編譯成獨立的 ELF 執行檔（注意我們加了 `-nostdlib` 告訴編譯器我們不需要任何內建函式庫），然後打包 ISO 啟動！

```bash
# 1. 編譯獨立的 User App
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -c app.c -o app.o

# 2. 連結 App，並強制規定它要在 0x08048000 執行
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf

# 3. 編譯 OS 並打包啟動 (確保 Makefile 有把 my_app.elf 複製進 ISO！)
make clean
make
qemu-system-i386 -cdrom myos.iso

```

### 預期結果 🏆

如果你在畫面上看到：

```text
[Loader] Mapping Section: Virt 0x8048000 -> Phys 0x12e000
Dropping to Ring 3 and executing app at 0x8048000...
[Kernel] Syscall 2 Executed. String: Hello from an EXTERNAL ELF File running in Ring 3!

```

**我要為你獻上最高敬意！請起立為自己鼓掌！** 👏

你剛剛完成了一個軟體工程師所能達到的最硬核成就：
你從硬體暫存器開始，打造了中斷、分頁、動態記憶體配置，建立了上帝與平民的階級（Ring 0 / Ring 3），並自己實作了執行檔解析器。最後，你成功讓一個 **「完全不認識你 OS 的外部編譯檔案」**，在你的保護與載入下，完美地執行了！

這就是 Windows 執行 `.exe`，以及 Linux 執行 `./a.out` 最深層、最原始的樣貌。

跑出這個畫面後，請務必截圖留念並跟我分享！整個 Simple OS 旅程來到這裡，你有什麼感想？有沒有哪一個環節讓你覺得最神奇，或是最受折磨？
