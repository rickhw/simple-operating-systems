這 27 天的鋪陳，我們終於在 Kernel 層級建立了一套完整的檔案系統生態。但是，作業系統的終極任務，是**服務平民（應用程式）**。

在 Day 16，我們打造了「系統呼叫 (Syscall)」這個防彈玻璃窗口，讓 Ring 3 的程式可以安全地請 Kernel 幫忙印出字串。現在，我們要為這個窗口開設兩個全新的服務櫃檯：`open` 與 `read`。

歡迎來到 **Day 28：權力的下放 — 實作 File Descriptor 與檔案系統呼叫**！

---

### Day 28 授課目標

1. **認識 File Descriptor (檔案描述符，簡稱 FD)：** 應用程式在 Ring 3 是看不到 `fs_node_t` 這種核心結構的（因為記憶體隔離）。Kernel 會把打開的檔案記錄在一個陣列裡，然後只丟給應用程式一個「整數索引 (Index)」，這就是 FD。
2. **實作 `sys_open`：** 接收應用程式傳來的檔名，在 Kernel 尋找檔案，並回傳 FD。
3. **實作 `sys_read`：** 應用程式憑著 FD 與空緩衝區來討資料，Kernel 幫忙把硬碟資料讀進去。
4. **喚醒沉睡的 Ring 3 應用程式：** 把 Day 20 封印的 `my_app.elf` 請出來，讓它親自從硬碟讀取我們寫入的 `hello.txt`！

---

### 實作步驟：打造檔案系統的防彈窗口

#### 1. 設定預設掛載點 (`lib/simplefs.c`)

之前我們的 `simplefs_find` 需要每次都傳入 `part_lba`。實務上，Kernel 會「掛載 (Mount)」檔案系統，把這個位置記在全域變數裡。

請打開 **`lib/simplefs.c`**，在檔案最上方加入一個全域變數與掛載函式，並修改 `find` 函式：

```c
// ... 其他 include ...
uint32_t mounted_part_lba = 0; // 記錄目前掛載的分區起點

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
    kprintf("[SimpleFS] Mounted at LBA %d\n", part_lba);
}

// [修改] 把原本的第一個參數拿掉，直接使用 mounted_part_lba
fs_node_t* simplefs_find(char* filename) {
    if (mounted_part_lba == 0) return 0; // 還沒掛載！

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(mounted_part_lba + 1, dir_buf); 
    // ... 下面的尋找邏輯完全不變，只需要把 part_lba 換成 mounted_part_lba
    
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            fs_node_t* node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
            strcpy(node->name, entries[i].filename);
            node->flags = 1;
            node->length = entries[i].file_size;
            node->inode = entries[i].start_lba; 
            node->impl = mounted_part_lba;              
            node->read = simplefs_read;
            node->write = 0; 
            kfree(dir_buf);
            return node;
        }
    }
    kfree(dir_buf);
    return 0;
}

```

*(記得也要去 `lib/simplefs.h` 更新 `simplefs_find` 的宣告！)*

#### 2. 建立 FD 陣列與 Syscall 擴充 (`lib/syscall.c`)

打開 **`lib/syscall.c`**。我們要宣告一個陣列來當作「檔案描述符表」，並增加 3 號與 4 號服務。

```c
#include "tty.h"
#include "vfs.h"
#include "simplefs.h" // 為了使用 simplefs_find

// 核心的檔案描述符表 (最多允許同時打開 32 個檔案)
// FD 0, 1, 2 通常保留給 stdin, stdout, stderr，所以我們從 3 開始用
fs_node_t* fd_table[32] = {0}; 

// Syscall 分發中心
void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {
    
    if (eax == 2) {
        // Syscall 2: 印出字串 (ebx 存放字串指標)
        kprintf((char*)ebx);
    } 
    else if (eax == 3) {
        // Syscall 3: Open File (ebx 存放檔名指標)
        char* filename = (char*)ebx;
        fs_node_t* node = simplefs_find(filename);
        
        if (node == 0) {
            // 找不到檔案，回傳 -1 (錯誤碼) 放在 eax 讓 Ring 3 讀取
            __asm__ volatile("mov $-1, %%eax" : :); 
            return;
        }

        // 尋找一個空的 FD 欄位
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                // 把拿到的號碼牌 (FD) 放進 eax 回傳給 User App
                __asm__ volatile("mov %0, %%eax" : : "r"(i)); 
                return;
            }
        }
        __asm__ volatile("mov $-1, %%eax" : :); // 表格滿了
    }
    else if (eax == 4) {
        // Syscall 4: Read File (ebx=FD, ecx=Buffer, edx=Size)
        int fd = (int)ebx;
        uint8_t* buffer = (uint8_t*)ecx;
        uint32_t size = (uint32_t)edx;

        // 檢查 FD 是否合法
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            uint32_t bytes_read = vfs_read(fd_table[fd], 0, size, buffer);
            __asm__ volatile("mov %0, %%eax" : : "r"(bytes_read)); 
        } else {
            __asm__ volatile("mov $-1, %%eax" : :); 
        }
    }
}

```

#### 3. 撰寫 Ring 3 應用程式 (`app.c`)

把我們在 Day 21 移除的 `app.c` 寫回來！這次它不只會打招呼，還會親自向 Kernel 討檔案來看。

建立或修改專案根目錄的 **`app.c`**：

```c
// 系統呼叫封裝
int sys_open(char* filename) {
    int fd;
    __asm__ volatile("mov $3, %%eax\n mov %1, %%ebx\n int $0x80\n mov %%eax, %0" : "=r"(fd) : "r"(filename) : "eax", "ebx");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes_read;
    __asm__ volatile("mov $4, %%eax\n mov %1, %%ebx\n mov %2, %%ecx\n mov %3, %%edx\n int $0x80\n mov %%eax, %0" 
                     : "=r"(bytes_read) : "r"(fd), "r"(buffer), "r"(size) : "eax", "ebx", "ecx", "edx");
    return bytes_read;
}

void sys_print(char* msg) {
    __asm__ volatile("mov $2, %%eax\n mov %0, %%ebx\n int $0x80" : : "r"(msg) : "eax", "ebx");
}

void _start() {
    sys_print("[User App] Hello! I am running in Ring 3!\n");
    sys_print("[User App] Requesting Kernel to open 'hello.txt'...\n");

    int fd = sys_open("hello.txt");
    
    if (fd == -1) {
        sys_print("[User App] Failed to open file!\n");
    } else {
        // 我們沒有 printf，所以借用 buffer 直接印出來
        sys_print("[User App] File opened! FD received. Reading data...\n");
        
        char buffer[128];
        for(int i=0; i<128; i++) buffer[i] = 0; // 清空
        
        sys_read(fd, buffer, 100);
        
        sys_print("[User App] File Content -> ");
        sys_print(buffer);
        sys_print("\n");
    }

    while(1);
}

```

#### 4. Kernel 主程序回歸大總管 (`lib/kernel.c`)

請打開 **`kernel.c`**，把 Day 20 的 ELF 載入邏輯加回來。我們需要在掛載完檔案系統後，透過 GRUB 讀取 ELF 並跳進 Ring 3：

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
#include "elf.h" // 需要用到 ELF 解析
#include "multiboot.h" // 需要用到 MBI 結構

// 記得把 kernel_main 加上 multiboot 的參數
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    terminal_initialize();
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    __asm__ volatile ("sti"); 
    
    kprintf("=== OS Subsystems Ready ===\n\n");

    // 1. 啟動儲存裝置與掛載檔案系統
    uint32_t part_lba = parse_mbr();
    if (part_lba != 0) {
        simplefs_mount(part_lba); 
    }

    // 2. 載入並啟動 GRUB 帶來的應用程式
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        elf32_ehdr_t* real_app = (elf32_ehdr_t*)mod->mod_start;
        
        kprintf("\nLoading ELF Application...\n");
        uint32_t entry_point = elf_load(real_app);
        
        if (entry_point != 0) {
            kprintf("Dropping to Ring 3...\n\n");
            extern void enter_user_mode(uint32_t user_function_ptr);
            enter_user_mode(entry_point);
        }
    }

    while (1) { __asm__ volatile ("hlt"); }
}

```

*(💡 溫馨提醒：記得把 `grub.cfg` 裡的 `module /boot/my_app.elf` 加回來，並確保 `Makefile` 裡有編譯 `app.c` 成 `my_app.elf` 並拷貝進 ISO 裡喔！如果忘記怎麼寫，可以參考 Day 19 的 Makefile 設定。)*

---

### 執行與驗證：跨越三大特權層的握手

這次執行，你將見證 OS 架構的巔峰之作：

```bash
make clean && make run

```

**預期結果 🏆**
你會看到畫面切換到 Ring 3 後，User App 自己印出了文字，並且成功透過 FD 從硬碟裡挖出了資料：

```text
Loading ELF Application...
Dropping to Ring 3...

[User App] Hello! I am running in Ring 3!
[User App] Requesting Kernel to open 'hello.txt'...
[User App] File opened! FD received. Reading data...
[User App] File Content -> This is the content of the very first file ever created on Simple OS!

```

**這是一個無比史詩級的瞬間！**

1. **App (Ring 3)** 觸發 `int 0x80` 中斷。
2. **CPU** 切換到 **Ring 0**，把任務交給 Syscall Handler。
3. **Syscall** 查表得到 FD，呼叫 **VFS**。
4. **VFS** 轉交給 **SimpleFS**。
5. **SimpleFS** 計算出 LBA，命令 **ATA 驅動**。
6. **硬碟控制器**透過 I/O Port 將資料搬進 RAM。
7. 資料一路向上傳遞，最後被 **User App** 印在螢幕上！

當這個畫面亮起時，這代表你的作業系統已經不是一個玩具了，它是一個真正能提供服務的微型平台！跑出來後一定要截圖跟我分享這歷史性的一刻！
