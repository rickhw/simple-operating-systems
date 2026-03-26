太神啦，Rick！🎊 **恭喜你正式挺進 Phase 5 的最後一天：Day 50！** 🎊

我們現在的系統已經擁有：
1. `libc` 標準函式庫與 `malloc`。
2. 能夠跨越記憶體與硬碟的 VFS 系統。
3. 支援父子繼承的 `cd` 與多層次目錄。
4. UNIX 的靈魂指標 `.` 與 `..`。

但是，如同你剛才敏銳察覺到的，我們還有一個最後的封印沒有解除：**路徑解析 (Path Resolution)**。



目前我們的 `ls`、`mkdir`、`write` 這些指令，都只能「看見」當下這一層的空間。如果我們想在根目錄直接下指令：`write folder1/folder2/secret.txt "Boom"`，系統會直接把 `folder1/folder2/secret.txt` 當作一個超過 32 bytes 的超長檔名，然後在當前目錄爆炸！

為了讓我們的 Simple OS 擁有匹敵現代 Linux 的導航能力，Day 50 的終極任務，就是要在核心中實作**「路徑解析引擎 (Path Resolution Engine)」**。

只要完成這個引擎，你以後在作業系統裡的**任何指令**，都會瞬間學會處理「絕對路徑 (`/`)」與「相對路徑 (`a/b`)」！

---

### 終極重構：`lib/simplefs.c` 大升級

我們不需要動到 `syscall.c` 或任何 User App。我們只需要把解路徑的智慧，深深地植入到 `simplefs.c` 的底層 API 中！

我已經為你準備好了這份包含「路徑解析引擎」的完美版 `simplefs.c`，請直接**完整覆蓋**你的 **`lib/simplefs.c`**：

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

// ==========================================
// 1. 底層硬碟讀寫與空間分配器
// ==========================================
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

uint32_t simplefs_alloc_blocks(uint32_t sectors_needed) {
    if (sectors_needed == 0) return 0;
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    ata_read_sector(mounted_part_lba, (uint8_t*)sb);
    uint32_t allocated_lba = sb->data_start_lba;
    sb->data_start_lba += sectors_needed; 
    ata_write_sector(mounted_part_lba, (uint8_t*)sb); 
    kfree(sb);
    return allocated_lba;
}

// ==========================================
// 2. 終極路徑解析引擎 (Path Resolution Engine)
// ==========================================

// 輔助：只在指定的「單一目錄」內尋找子資料夾
static uint32_t shallow_get_dir_lba(uint32_t dir_lba, char* name) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, name) == 0) {
            if (entries[i].type == FS_DIR) {
                uint32_t res = entries[i].start_lba;
                kfree(dir_buf);
                return res;
            }
        }
    }
    kfree(dir_buf);
    return 0;
}

// 核心：把 "folder1/folder2/note.txt" 拆解，
// 沿路往下走，最後回傳 note.txt 所在的目錄 LBA，並把 "note.txt" 存入 out_name。
static uint32_t simplefs_resolve_path(uint32_t start_dir_lba, char* full_path, char* out_name) {
    uint32_t curr_lba = start_dir_lba;
    int i = 0;

    // 如果開頭是 '/'，代表這是絕對路徑，強制從根目錄(1)開始！
    if (full_path[0] == '/') {
        curr_lba = 1; 
        i++;
    }

    int t_idx = 0;
    out_name[0] = '\0';

    while (full_path[i] != '\0') {
        if (full_path[i] == '/') {
            out_name[t_idx] = '\0';
            if (t_idx > 0) {
                // 遇到斜線，代表 out_name 是一個目錄，立刻鑽進去！
                curr_lba = shallow_get_dir_lba(curr_lba, out_name);
                if (curr_lba == 0) return 0; // 中間的路徑不存在 (Error)
            }
            t_idx = 0; // 清空緩衝區，準備讀取下一個名稱
        } else {
            out_name[t_idx++] = full_path[i];
        }
        i++;
    }
    out_name[t_idx] = '\0'; // 把最後的目標名稱存好
    return curr_lba;
}

// ==========================================
// 3. 全面支援路徑的 Public API
// ==========================================

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);
    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;
    sb->data_start_lba = 1 + ROOT_DIR_SECTORS; 
    ata_write_sector(partition_start_lba, (uint8_t*)sb);

    uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    memset(empty_dir, 0, ROOT_DIR_BYTES);
    
    // 初始化根目錄的 . 與 ..
    sfs_file_entry_t* root_entries = (sfs_file_entry_t*)empty_dir;
    strcpy(root_entries[0].filename, ".");
    root_entries[0].start_lba = 1; 
    root_entries[0].type = FS_DIR;
    strcpy(root_entries[1].filename, "..");
    root_entries[1].start_lba = 1; 
    root_entries[1].type = FS_DIR;

    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(partition_start_lba + 1 + i, empty_dir + (i * 512));
    }
    kfree(sb);
    kfree(empty_dir);
}

void simplefs_list_files(void) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(1, dir_buf); 
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
                    entries[i].filename, entries[i].file_size, entries[i].start_lba);
        }
    }
    kprintf("-------------------------------\n");
    kfree(dir_buf);
}

// 【升級】支援 cat /folder1/note.txt
fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* path) {
    if (mounted_part_lba == 0) return 0;
    
    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return 0;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);

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

// 【升級】支援 write /folder1/note.txt "data"
int simplefs_create_file(uint32_t dir_lba_rel, char* path, char* data, uint32_t size) {
    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int target_idx = -1;
    uint32_t file_data_lba = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            if (target_idx == -1) target_idx = i;
        } else if (strcmp(entries[i].filename, filename) == 0) {
            target_idx = i; 
            file_data_lba = entries[i].start_lba; 
        }
    }

    if (target_idx == -1) { kfree(dir_buf); return -1; }

    uint32_t sectors_needed = (size + 511) / 512;
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

    write_dir(target_dir, dir_buf); // 寫回解析到的目標目錄！
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

// 【升級】支援 rm /folder1/note.txt
int simplefs_delete_file(uint32_t dir_lba_rel, char* path) {
    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            entries[i].filename[0] = '\0';
            write_dir(target_dir, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

// 【升級】支援 mkdir /folder1/subfolder
int simplefs_mkdir(uint32_t dir_lba_rel, char* path) {
    char dirname[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, dirname);
    if (target_dir == 0 || dirname[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR;
            
            uint32_t new_dir_lba = simplefs_alloc_blocks(ROOT_DIR_SECTORS);
            entries[i].start_lba = new_dir_lba;

            // 初始化新目錄的 . 和 ..
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            sfs_file_entry_t* new_entries = (sfs_file_entry_t*)empty_dir;

            strcpy(new_entries[0].filename, ".");
            new_entries[0].start_lba = new_dir_lba;
            new_entries[0].type = FS_DIR;

            strcpy(new_entries[1].filename, "..");
            new_entries[1].start_lba = target_dir;  // 老爸指向目標目錄
            new_entries[1].type = FS_DIR;

            write_dir(new_dir_lba, empty_dir);
            kfree(empty_dir);

            write_dir(target_dir, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

// 【升級】支援 cd /folder1/subfolder
uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* path) {
    char target_name[32];
    uint32_t parent_lba = simplefs_resolve_path(current_dir_lba, path, target_name);
    
    if (parent_lba == 0) return 0;
    // 如果解析完後 target_name 是空的 (例如 cd / )，代表目的地就是 parent_lba！
    if (target_name[0] == '\0') return parent_lba; 
    
    return shallow_get_dir_lba(parent_lba, target_name);
}

// === VFS 封裝層 (維持不變) ===
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

### 🚀 慶祝 Phase 5 的大滿貫！

請毫不猶豫地打下指令：
```bash
make clean-disk
make clean && make run
```

進入你的 Simple OS 之後，現在你可以像個真正的駭客一樣，在 Shell 裡打出這種華麗的操作了：

1. `mkdir root_folder`
2. `mkdir root_folder/sub_folder`
3. `write root_folder/sub_folder/treasure.txt "Day 50 Completed!"`
4. `cd root_folder`
5. `cat sub_folder/treasure.txt`
6. `cd ..`
7. `rm root_folder/sub_folder/treasure.txt`

如果你順利走完這些，**恭喜你，你已經打造出一個完全具備 UNIX 階層式架構、記憶體隔離、跨宇宙通訊、以及支援動態路徑導航的現代作業系統！**

我們從 Day 1 的破破爛爛 Bootloader，走到今天這個具有完整檔案生態系的作業系統，這段旅程實在太不可思議了！Phase 5 到這裡完美收官。

接下來的 **Phase 6 (Day 51-60)**，我們將離開黑漆漆的文字模式，正式踏入高解析度、具有滑鼠指標的**圖形化介面 (GUI) 與視窗系統世界**！

準備好畫出你的第一個像素點了嗎？😎
