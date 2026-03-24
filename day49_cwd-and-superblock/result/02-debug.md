發生詭異的現象，我猜是某個地方 lba 偏掉了 ...

步驟：

1. mkdir folder1
2. cd folder1 --> 說找不到
3. ls --> cwd is root path, file 11 (圖一)
4. cd / (圖二)
5. ls --> file 0?

底下是相關的 source code

---
simplefs.c
```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

uint32_t mounted_part_lba = 0;

#define ROOT_DIR_SECTORS 8
#define ROOT_DIR_BYTES   (ROOT_DIR_SECTORS * 512)
#define MAX_FILES        (ROOT_DIR_BYTES / sizeof(sfs_file_entry_t))

// --- 內部輔助函數 ---
// 【統一】所有的操作都基於 mounted_part_lba + 相對位移
static void read_dir(uint32_t dir_lba_rel, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_read_sector(mounted_part_lba + dir_lba_rel + i, buffer + (i * 512));
    }
}
static void write_dir(uint32_t dir_lba_rel, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(mounted_part_lba + dir_lba_rel + i, buffer + (i * 512));
    }
}

// 【全新魔法】全域磁區分配器！自動從 Superblock 申請安全的硬碟空間
uint32_t simplefs_alloc_blocks(uint32_t sectors_needed) {
    if (sectors_needed == 0) return 0;
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    ata_read_sector(mounted_part_lba, (uint8_t*)sb);

    uint32_t allocated_lba = sb->data_start_lba;
    sb->data_start_lba += sectors_needed; // 更新硬碟下一個安全位置！

    ata_write_sector(mounted_part_lba, (uint8_t*)sb); // 寫回硬碟永久保存
    kfree(sb);
    return allocated_lba;
}

// --- 公開 API ---

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
    kprintf("[SimpleFS] Mounted at LBA [%d].\n", part_lba);
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;
    sb->data_start_lba = 1 + ROOT_DIR_SECTORS; // 第一個可以存放資料的地方

    ata_write_sector(partition_start_lba, (uint8_t*)sb);

    uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    memset(empty_dir, 0, ROOT_DIR_BYTES);
    // 直接寫入絕對位址，因為這時候還沒 mount
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(partition_start_lba + 1 + i, empty_dir + (i * 512));
    }

    kfree(sb);
    kfree(empty_dir);
}

void simplefs_list_files(void) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(1, dir_buf); // 1 = 根目錄的相對位址
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
                    entries[i].filename, entries[i].file_size, entries[i].start_lba);
            file_count++;
        }
    }
    if (file_count == 0) kprintf("(Directory is empty)\n");
    kprintf("-------------------------------\n");
    kfree(dir_buf);
}

fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* filename) {
    if (mounted_part_lba == 0) return 0;
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);

    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < MAX_FILES; i++) {
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

int simplefs_create_file(uint32_t dir_lba_rel, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int target_idx = -1;
    uint32_t file_data_lba = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            if (target_idx == -1) target_idx = i;
        } else if (strcmp(entries[i].filename, filename) == 0) {
            target_idx = i;
            file_data_lba = entries[i].start_lba; // 沿用舊的空間
        }
    }

    if (target_idx == -1) { kfree(dir_buf); return -1; }

    uint32_t sectors_needed = (size + 511) / 512;

    // 如果是新檔案，請超級分配器幫忙找空地！
    if (file_data_lba == 0 && sectors_needed > 0) {
        file_data_lba = simplefs_alloc_blocks(sectors_needed);
    }

    if (sectors_needed > 0) {
        for (uint32_t i = 0; i < sectors_needed; i++) {
            uint8_t temp[512] = {0};
            uint32_t copy_size = 512;
            if ((i * 512) + 512 > size) copy_size = size - (i * 512);
            memcpy(temp, data + (i * 512), copy_size);
            ata_write_sector(mounted_part_lba + file_data_lba + i, temp);
        }
    }

    memset(entries[target_idx].filename, 0, 32);
    strcpy(entries[target_idx].filename, filename);
    entries[target_idx].start_lba = file_data_lba;
    entries[target_idx].file_size = size;
    entries[target_idx].type = FS_FILE;

    write_dir(dir_lba_rel, dir_buf);
    kfree(dir_buf);
    return 0;
}

uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t file_lba = node->impl + node->inode;
    uint32_t actual_size = size;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) actual_size = node->length - offset;

    uint32_t start_sector = offset / 512;
    uint32_t end_sector = (offset + actual_size - 1) / 512;
    uint32_t sector_offset = offset % 512;

    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    uint32_t bytes_read = 0;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        ata_read_sector(file_lba + i, temp_buf);
        uint32_t copy_size = 512 - sector_offset;
        if (bytes_read + copy_size > actual_size) copy_size = actual_size - bytes_read;
        memcpy(buffer + bytes_read, temp_buf + sector_offset, copy_size);
        bytes_read += copy_size;
        sector_offset = 0;
    }
    kfree(temp_buf);
    return bytes_read;
}

int simplefs_readdir(uint32_t dir_lba_rel, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;
                *out_type = entries[i].type;
                kfree(dir_buf);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buf);
    return 0;
}

int simplefs_delete_file(uint32_t dir_lba_rel, char* filename) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            entries[i].filename[0] = '\0';
            write_dir(dir_lba_rel, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

int simplefs_mkdir(uint32_t dir_lba_rel, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR;

            // 向超級分配器申請 8 個磁區當作新目錄！
            entries[i].start_lba = simplefs_alloc_blocks(ROOT_DIR_SECTORS);

            // 清空新目錄
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            write_dir(entries[i].start_lba, empty_dir);
            kfree(empty_dir);

            write_dir(dir_lba_rel, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(current_dir_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, dirname) == 0) {
            if (entries[i].type == FS_DIR) {
                uint32_t target_lba = entries[i].start_lba;
                kfree(dir_buf);
                return target_lba;
            }
        }
    }
    kfree(dir_buf);
    return 0;
}

// === VFS 封裝層 ===
int vfs_create_file(uint32_t dir_lba, char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_create_file(dir_lba, filename, content, strlen(content));
}
int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(dir_lba, index, out_name, out_size, out_type);
}
int vfs_delete_file(uint32_t dir_lba, char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(dir_lba, filename);
}
int vfs_mkdir(uint32_t dir_lba, char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(dir_lba, dirname);
}
```

---
syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

fs_node_t* fd_table[32] = {0};

// 記得更新最上方的 extern 宣告
extern fs_node_t* simplefs_find(uint32_t dir_lba, char* filename);
extern int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
extern int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
extern int vfs_delete_file(uint32_t dir_lba, char* filename);
extern int vfs_mkdir(uint32_t dir_lba, char* dirname);

extern uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname); // [Day48][Add]
extern uint32_t mounted_part_lba;   // [Day48][Add]


// ==========================================
// IPC 訊息佇列 (Message Queue)
// ==========================================
#define MAX_MESSAGES 16

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

// 環狀佇列 (Circular Buffer)
ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量

// Mutex for Single Processor (SMP)
// 【新增】核心同步鎖：利用開關中斷來保護 Critical Section
void ipc_lock() {
    __asm__ volatile("cli"); // 關閉中斷：誰都別想搶走我的 CPU！
}

void ipc_unlock() {
    __asm__ volatile("sti"); // 開啟中斷：我用完了，大家可以繼續排隊了。
}

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    uint32_t eax = regs->eax;

    // kprintf("[syscall_handler] eax: [%d]\n", eax);

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;

        // 取得目前的目錄 (如果沒設定，1 代表相對根目錄)
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
        fs_node_t* node = simplefs_find(current_dir, filename);

        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) {
        schedule();
    }
    else if (eax == 7) {
        exit_task();
    }
    else if (eax == 8) {
        regs->eax = sys_fork(regs);
    }
    else if (eax == 9) {
        regs->eax = sys_exec(regs);
    }
    else if (eax == 10) {
        regs->eax = sys_wait(regs->ebx);
    }

    // ==========================================
    // 【新增】IPC 系統呼叫
    // ==========================================
    // Syscall 11: sys_send (傳送訊息)
    else if (eax == 11) {
        char* msg = (char*)regs->ebx;

        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, msg);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else {
            regs->eax = -1; // Queue Full
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
    }
    // Syscall 12: sys_recv (接收訊息)
    else if (eax == 12) {
        char* buffer = (char*)regs->ebx;

        // START OF CRITICAL SECTION
        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else {
            regs->eax = 0;
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
        // End OF CRITICAL SECTION
    }

    // Syscall 13: sbrk (動態記憶體擴充)
    else if (eax == 13) {
        int increment = (int)regs->ebx;
        uint32_t old_end = current_task->heap_end;

        // 把 Heap 的邊界往上推
        current_task->heap_end += increment;

        // 回傳舊的邊界，這就是新分配空間的起始位址！
        regs->eax = old_end;
    }

    // Syscall 14: sys_create (建立/寫入文字檔)
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_create_file(current_dir, filename, content);
        ipc_unlock();
    }
    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        uint32_t* out_type = (uint32_t*)regs->esi; // 【新增】從 ESI 暫存器拿出 out_type 指標！

        // 取得目前的目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        // 【修改】直接呼叫底層，並傳入 current_dir！
        regs->eax = simplefs_readdir(current_dir, index, out_name, out_size, out_type);
        ipc_unlock();
    }

    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock(); // 上鎖！修改硬碟資料是非常危險的操作
        regs->eax = vfs_delete_file(current_dir, filename);
        ipc_unlock();
    }

    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_mkdir(current_dir, dirname);
        ipc_unlock();
    }

    // Syscall 18: sys_chdir (切換目錄)
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        // 如果還沒有 CWD，預設就是根目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : mounted_part_lba + 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            current_task->cwd_lba = mounted_part_lba + 1; // 回到根目錄
            regs->eax = 0;
        } else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) {
                current_task->cwd_lba = target_lba; // 【切換成功】更新任務的 CWD！
                regs->eax = 0;
            } else {
                regs->eax = -1; // 找不到該目錄
            }
        }
        ipc_unlock();
    }

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


void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "write.elf", "ls.elf", "rm.elf", "mkdir.elf"};
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
    __asm__ volatile ("sti");

    kprintf("=== OS Subsystems Ready ===\n\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }

    // 呼叫我們重構好的函式！
    setup_filesystem(part_lba, mbd);

    // [Day35] start
    // --- 應用程式載入與排程 ---
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            // kprintf("Creating ONE Initial User Task (Init Process)...\n\n");
            kprintf("Creating Initial Process...\n\n");

            init_multitasking();

            // 為唯一的 Shell 分配 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Kernel dropping to idle state (Ring0 -> Ring3).\n");

            // 【關鍵修復】Kernel 必須功成身退，把自己宣告死亡！
            // 否則排程器會以為 Kernel 還活著，就會一直把 CPU 切給 Kernel 的 hlt！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }
    // [Day35] end

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
task.c
```c
#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;
task_t *idle_task = 0;

extern uint32_t page_directory[]; // [Day38] 取得 Kernel 原始的分頁目錄
// extern void switch_task(uint32_t *current_esp, uint32_t *next_esp); // [Day38][delete]
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3); // [Day38][change] 加上第三個參數 cr3
extern void child_ret_stub();

void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

void init_multitasking() {
    // kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory; // [Day38][Add] 住在原始宇宙
    main_task->cwd_lba = 0; // [Day48][Add] CWD LBA

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->id = 9999;
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory; // [Day38][Add] 住在原始宇宙
    idle_task->cwd_lba = 0; // [Day48][Add] CWD LBA

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop;
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    idle_task->esp = (uint32_t) kstack;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->id) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }

    // [Day46]【新增】如果死的不是老爸 (Kernel 原始宇宙)，就把它專屬的宇宙回收！
    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    current_task->state = TASK_DEAD;
    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            curr->next = next_node->next;
            next_node = curr->next;
        } else {
            curr = next_node;
            next_node = curr->next;
        }
    }

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // switch_task(&prev->esp, &current_task->esp);
        // [Day38][Change]【關鍵】把新任務專屬的宇宙 CR3 傳遞給組合語言！
        switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
    }
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory; // [Day38][Add]
    new_task->cwd_lba = 0; // [Day48][Add] CWD LBA

    // ==========================================
    // [Day43]【新增】為初代老爸 (Shell) 預先分配 10 個實體分頁給 User Heap
    // ==========================================
    for (int i = 0; i < 10; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    new_task->heap_end = 0x10000000; // 初始化 Heap 頂點

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // 留給 User Stack 一點呼吸空間
    uint32_t *ustack = (uint32_t*) (user_stack_top - 64);

    *(--ustack) = 0;
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;
    *(--ustack) = 0;
    *(--ustack) = 0;

    *(--kstack) = 0x23;
    *(--kstack) = (uint32_t)ustack;
    *(--kstack) = 0x0202;
    *(--kstack) = 0x1B;
    *(--kstack) = entry_point;

    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}

int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->id = next_task_id++;
    child->state = TASK_RUNNING;
    child->wait_pid = 0;
    child->page_directory = current_task->page_directory; // [Day38][Add] (sys_fork 裡是 child->page_directory)
    child->cwd_lba = current_task->cwd_lba; // [Day48][Add] 子行程必須繼承老爸當前的工作目錄 (CWD)！

    // ==========================================
    // [Day43]【新增】讓子行程暫時繼承老爸的 Heap 邊界紀錄
    // ==========================================
    child->heap_end = current_task->heap_end;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    uint32_t child_ustack_base = 0x083FF000 - (child->id * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + 4096) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + 4096) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + 4096) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break;
        }
    }

    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    *(--kstack) = child_ebp;
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    // 1. 取得當前的目錄 (若為 0 則預設為根目錄 1)
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

    fs_node_t* file_node = simplefs_find(current_dir, filename);

    // 3. 【系統指令救星】如果在當前目錄找不到，強制去「根目錄 (1)」找找看！
    // 這模擬了 Linux 系統去 /bin 目錄尋找全域指令的行為
    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }

    // 4. 如果連根目錄都沒有，那就真的找不到了
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // =====================================================================
    // 【打包行李】在切換宇宙前，把參數拷貝到 Kernel Heap，這樣新宇宙才看得到！
    // =====================================================================
    int argc = 0;
    char* safe_argv[16]; // 暫存指標陣列

    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);     // 在 Kernel Heap 申請空間
            memcpy(safe_argv[argc], old_argv[argc], len); // 把字串拷貝過來
            argc++;
        }
    }
    safe_argv[argc] = 0; // 結尾補 NULL

    // =====================================================================
    // 【神聖分離】為這個 Process 建立專屬的新宇宙！
    // =====================================================================
    uint32_t new_cr3 = create_page_directory();

    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    // =====================================================================

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    // 分配新宇宙的 User Stack
    uint32_t clean_user_stack_top = 0x083FF000 + 4096;
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // ==========================================
    // [Day]【新增】預先分配 10 個實體分頁 (40KB) 給 User Heap
    // ==========================================
    for (int i = 0; i < 10; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_end = 0x10000000; // 初始化 Heap 頂點

    uint32_t stack_ptr = clean_user_stack_top - 64;

    // ------------------------------------------
    // 使用「打包好的行李 (safe_argv)」來構造堆疊
    // ------------------------------------------
    uint32_t argv_ptrs[16] = {0};

    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;

            // 拷貝完就可以把 Kernel Heap 的行李丟掉了
            kfree(safe_argv[i]);
        }

        stack_ptr = stack_ptr & ~3;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(int pid) {
    task_t *temp = current_task->next;
    int found = 0;
    while (temp != current_task) {
        if (temp->id == (uint32_t)pid) { found = 1; break; }
        temp = temp->next;
    }
    if (!found) return 0;

    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();
    return 0;
}
```
