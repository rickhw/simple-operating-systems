哈哈，Rick！這個案件真的太精彩了，你現在遇到的現象，完美演繹了作業系統開發中最經典的**「硬碟空間限制」**與**「記憶體越界讀取 (Buffer Over-read)」**！

這不是 Kernel Heap 的錯，也不是 Syscall 的錯，兇手藏在一個非常基礎的數學題裡面。我們來當一次系統法醫：

### 🔍 案發現場解密：被塞爆的「512 Bytes 小套房」

還記得我們在 `simplefs_create_file` 裡面寫的這行程式碼嗎？
```c
int max_entries = 512 / sizeof(sfs_file_entry_t);
```
在以前，`sfs_file_entry_t` 是 32 bytes，所以一個 512 bytes 的磁區可以塞 16 個檔案。
但是在 Day 47，我們為了支援 `type` 屬性，把結構對齊放大到了 **64 bytes**！

$512 \div 64 = 8$

也就是說，你的**根目錄 (Root Directory) 現在最多、最多只能裝得下 8 個檔案！**
讓我們清點一下你在 `setup_filesystem` 裡面安裝的檔案：
1. `hello.txt`
2. `shell.elf`
3. `echo.elf`
4. `cat.elf`
5. `ping.elf`
6. `pong.elf`
7. `write.elf`
8. `ls.elf`  **<-- 第 8 個，目錄剛好客滿！**
9. `rm.elf`  **<-- 裝不下，被作業系統拒絕 (return -1)！**
10. `mkdir.elf` **<-- 裝不下！**

這就是為什麼 `write` 指令永遠寫不進去，因為目錄已經客滿了！

**那為什麼開機畫面的 `[SimpleFS] List Root Directory` 卻印出了所有的檔案？**
因為在 `simplefs_list_files` 裡面，你保留了一行寫死的程式碼：
```c
for (int i = 0; i < 16; i++) { // 你逼它讀 16 個項目！
```
你只從硬碟讀了 512 bytes，但你的迴圈卻強迫 C 語言往後讀了 $16 \times 64 = 1024$ bytes 的記憶體。多出來的那 512 bytes 剛好是 `kmalloc` 在記憶體裡鄰近的資料（可能是存放那幾條字串的暫存區），所以它「碰巧」印出了那些幽靈檔案的殘影！這就是標準的記憶體越界漏洞！

---

### 🛠️ 終極修復：擴建根目錄莊園！

既然 1 個磁區（512 Bytes）只能裝 8 個檔案太小了，我們就一口氣給根目錄分配 **8 個磁區 (4096 Bytes)**，這樣就可以裝下 $4096 \div 64 = 64$ 個檔案了！

請用以下這個**升級版**完全覆蓋你的 **`lib/simplefs.c`**：
（我利用了 `#define` 巨集來定義大小，並加入了 `read_root_dir` 和 `write_root_dir` 幫你自動處理跨磁區的目錄讀寫！）

```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

uint32_t mounted_part_lba = 0; 

// 【擴建配置】
#define ROOT_DIR_SECTORS 8  // 根目錄佔用 8 個磁區
#define ROOT_DIR_BYTES   (ROOT_DIR_SECTORS * 512) // 4096 Bytes
#define MAX_FILES        (ROOT_DIR_BYTES / sizeof(sfs_file_entry_t)) // 64 個檔案！

// --- 內部輔助函數：讀寫整個根目錄 ---
static void read_root_dir(uint32_t part_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_read_sector(part_lba + 1 + i, buffer + (i * 512));
    }
}
static void write_root_dir(uint32_t part_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(part_lba + 1 + i, buffer + (i * 512));
    }
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
    // 【修改】資料區從根目錄之後開始 (1 + 8 = 9)
    sb->data_start_lba = 1 + ROOT_DIR_SECTORS; 

    ata_write_sector(partition_start_lba, (uint8_t*)sb);

    uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    memset(empty_dir, 0, ROOT_DIR_BYTES);
    write_root_dir(partition_start_lba, empty_dir); // 寫入空白的巨型根目錄

    kfree(sb);
    kfree(empty_dir);
    kprintf("[SimpleFS] Format complete. Root Dir capacity: %d files.\n", MAX_FILES);
}

void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
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

fs_node_t* simplefs_find(char* filename) {
    if (mounted_part_lba == 0) return 0; 
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(mounted_part_lba, dir_buf);
    
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

int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int target_idx = -1;
    uint32_t next_data_lba = 1 + ROOT_DIR_SECTORS; // 避開根目錄

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            if (target_idx == -1) target_idx = i;
        } else {
            if (strcmp(entries[i].filename, filename) == 0) target_idx = i; // 覆寫
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) next_data_lba = file_end_lba;
        }
    }

    if (target_idx == -1) { kfree(dir_buf); return -1; } 

    uint32_t sectors_needed = (size + 511) / 512;
    if (sectors_needed > 0) {
        for (uint32_t i = 0; i < sectors_needed; i++) {
            uint8_t temp[512] = {0};
            uint32_t copy_size = 512;
            if ((i * 512) + 512 > size) copy_size = size - (i * 512);
            memcpy(temp, data + (i * 512), copy_size);
            ata_write_sector(part_lba + next_data_lba + i, temp);
        }
    }

    memset(entries[target_idx].filename, 0, 32); 
    strcpy(entries[target_idx].filename, filename);
    entries[target_idx].start_lba = next_data_lba;
    entries[target_idx].file_size = size;
    entries[target_idx].type = FS_FILE;

    write_root_dir(part_lba, dir_buf);
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

int vfs_create_file(char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;
    int len = strlen(content);
    return simplefs_create_file(mounted_part_lba, filename, content, len);
}

int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;
                kfree(dir_buf);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buf);
    return 0;
}

int vfs_readdir(int index, char* out_name, uint32_t* out_size) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size);
}

int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            entries[i].filename[0] = '\0';
            write_root_dir(part_lba, dir_buf);
            kfree(dir_buf);
            return 0; 
        }
    }
    kfree(dir_buf);
    return -1; 
}

int vfs_delete_file(char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(mounted_part_lba, filename);
}

int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') { 
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR; 
            entries[i].start_lba = part_lba + 20 + i; 

            write_root_dir(part_lba, dir_buf);
            kfree(dir_buf);
            return 0; 
        }
    }
    kfree(dir_buf);
    return -1; 
}

int vfs_mkdir(char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(mounted_part_lba, dirname);
}
```

這個補丁把根目錄的容量推升到了 64 個檔案，並徹底解決了記憶體越界讀取的問題。
記得 `make clean-disk`，然後再次 `make clean && make run`！見證你打造的 64-slot 堅固檔案系統吧！🚀
