哈哈，Rick！你的系統工程師直覺真的是越來越敏銳了！

你完全點出了我們在 Day 48 留下的伏筆：**我們只讓 `ls` 和 `cd` 認識了「當前目錄」，但 `write`、`mkdir` 和 `rm` 這三兄弟，卻還傻傻地把資料寫死在「根目錄」裡！** 
不僅如此，你還提早發現了樹狀檔案系統的**「終極大魔王：硬碟空間重疊」**！
在扁平化系統時，我們只要「掃描當前目錄」就知道硬碟用到哪裡了。但現在有了子目錄，如果你在 `folder2` 掃描，它根本不知道 `folder1` 裡面的檔案佔用了哪些空間，兩邊如果同時寫入，就會發生恐怖的**資料覆蓋 (Data Corruption)**！

為了徹底解決這個問題，我們今天 **Day 49** 要實作兩個現代作業系統必備的核心機制：
1. **系統呼叫全面支援 CWD (當前目錄指標下放)。**
2. **全域硬碟空間分配器 (透過 Superblock 永久記錄剩餘空間)。**

我們要把所有的相對 LBA 定位邏輯統一。請跟著我更新這四個檔案：

---

### 1. 更新 `lib/include/simplefs.h`
把檔案操作的 API 第一個參數從絕對位址 `part_lba` 統一改成相對的 `dir_lba_rel`。

```c
#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include "vfs.h"

#define SIMPLEFS_MAGIC 0x21534653
#define FS_FILE 0
#define FS_DIR  1

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t root_dir_lba;
    uint32_t data_start_lba; // 【進化】這將成為我們的「全域下一個可用磁區」指標！
    uint8_t  padding[496];
} __attribute__((packed)) sfs_superblock_t;

typedef struct {
    char     filename[32];
    uint32_t start_lba;
    uint32_t file_size;
    uint32_t type;
    uint32_t reserved[5];
} __attribute__((packed)) sfs_file_entry_t;

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);
void simplefs_mount(uint32_t part_lba);
void simplefs_list_files(void); // 【修改】不再需要傳參數

// 【修改】全面改用相對目錄位址 (dir_lba_rel)
fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* filename);
int simplefs_create_file(uint32_t dir_lba_rel, char* filename, char* data, uint32_t size);
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
```

---

### 2. 核心重構：`lib/simplefs.c` (全域分配器誕生)
請用這個版本完全覆蓋你的 `simplefs.c`。我已經幫你加入了 `simplefs_alloc_blocks`，它會自動向 Superblock 申請空間，徹底解決資料覆寫的危機！

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

### 3. 將 CWD 下放：`lib/syscall.c`
現在 VFS 封裝都準備好接收目錄位址了，請在 `syscall_handler` 中，確保這些操作都傳入 `current_dir`：

```c
    // 記得更新最上方的 extern 宣告
    extern fs_node_t* simplefs_find(uint32_t dir_lba, char* filename);
    extern int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
    extern int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
    extern int vfs_delete_file(uint32_t dir_lba, char* filename);
    extern int vfs_mkdir(uint32_t dir_lba, char* dirname);
```

然後在對應的 `eax` 區塊，加入 `current_dir` 的判斷並傳遞下去：
```c
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;
        // 取得目前的目錄 (如果沒設定，1 代表相對根目錄)
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
        fs_node_t* node = simplefs_find(current_dir, filename);
        // ...
    }
    // ...
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
        ipc_lock();
        regs->eax = vfs_create_file(current_dir, filename, content);
        ipc_unlock();
    }
    // ... eax == 15 (sys_readdir), eax == 16 (sys_remove), eax == 17 (sys_mkdir) 
    // 也都依樣畫葫蘆，把 current_dir 傳進去！
```

---

### 4. 關鍵修復：`lib/kernel.c` 開機安裝程式
因為我們把 `simplefs_create_file` 第一個參數改成了相對的 `dir_lba_rel`。在開機建立系統檔時，必須**精準傳入 `1` (代表根目錄)**，而不是絕對位址 `part_lba`！

請打開 **`lib/kernel.c`**，找到 `setup_filesystem` 函式：
```c
void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    
    // 【修改】改成傳入 1 (根目錄)！
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "write.elf", "ls.elf", "rm.elf", "mkdir.elf"};
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            // 【修改】改成傳入 1 (根目錄)！
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }
    simplefs_list_files(); // 不用再傳 part_lba 啦
}
```

---

一切就緒！這是一次超大規模的心臟手術，請無必執行：
```bash
make clean-disk
make clean && make run
```

現在，當你在系統中輸入：
1. `mkdir folder2`
2. `cd folder2`
3. `write note.txt "hello world"`
4. `cat note.txt`
5. `ls`

你會發現 `note.txt` 結結實實地存在於 `folder2` 的異度空間裡，而且因為有 `Superblock` 的全域分配器，它也絕對不會跟外面的程式搶佔硬碟空間了！這就是一個真正的作業系統！😎
