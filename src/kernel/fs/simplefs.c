/**
 * @file src/kernel/fs/simplefs.c
 * @brief Main logic and program flow for simplefs.c.
 *
 * This file handles the operations and logic associated with simplefs.c.
 */

#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

#define ROOT_DIR_SECTORS 8
#define ROOT_DIR_BYTES   (ROOT_DIR_SECTORS * 512)
#define MAX_FILES        (ROOT_DIR_BYTES / sizeof(sfs_file_entry_t))

uint32_t mounted_part_lba = 0;

// 底層硬碟讀寫與空間分配器
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

// 路徑解析引擎 (Path Resolution Engine)
// 輔助：只在指定的「單一目錄」內尋找子資料夾
static uint32_t shallow_get_dir_lba(uint32_t dir_lba, char* name) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, name) == 0) {
            if (entries[i].type == SFS_TYPE_DIR) {
                uint32_t res = entries[i].start_lba;
                kfree(dir_buf);
                return res;
            }
        }
    }
    kfree(dir_buf);
    return 0;
}

// 把 "folder1/folder2/note.txt" 拆解，
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

//
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

// --- Public API ----

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
    root_entries[0].type = SFS_TYPE_DIR;

    strcpy(root_entries[1].filename, "..");
    root_entries[1].start_lba = 1;
    root_entries[1].type = SFS_TYPE_DIR;

    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(partition_start_lba + 1 + i, empty_dir + (i * 512));
    }
    kfree(sb);
    kfree(empty_dir);
}

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
}

void simplefs_list_files(void) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
                    entries[i].filename, entries[i].file_size, entries[i].start_lba);
        }
    }
    kprintf("-------------------------------\n");
    kfree(dir_buf);
}

// file
int simplefs_create_file(uint32_t dir_lba_rel, char* path, char* data, uint32_t size) {
    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int target_idx = -1;
    uint32_t file_data_lba = 0;

    for (uint32_t i = 0; i < MAX_FILES; i++) {
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
    entries[target_idx].type = SFS_TYPE_FILE;

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

fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* path) {
    if (mounted_part_lba == 0) return 0;

    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return 0;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);

    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
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

// --- Internal API ---

int simplefs_readdir(uint32_t dir_lba_rel, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba_rel, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
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

int simplefs_delete_file(uint32_t dir_lba_rel, char* path) {
    char filename[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, filename);
    if (target_dir == 0 || filename[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (uint32_t i = 0; i < MAX_FILES; i++) {
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

int simplefs_mkdir(uint32_t dir_lba_rel, char* path) {
    char dirname[32];
    uint32_t target_dir = simplefs_resolve_path(dir_lba_rel, path, dirname);
    if (target_dir == 0 || dirname[0] == '\0') return -1;

    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(target_dir, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = SFS_TYPE_DIR;

            uint32_t new_dir_lba = simplefs_alloc_blocks(ROOT_DIR_SECTORS);
            entries[i].start_lba = new_dir_lba;

            // 初始化新目錄的 . 和 ..
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            sfs_file_entry_t* new_entries = (sfs_file_entry_t*)empty_dir;

            strcpy(new_entries[0].filename, ".");
            new_entries[0].start_lba = new_dir_lba;
            new_entries[0].type = SFS_TYPE_DIR;

            strcpy(new_entries[1].filename, "..");
            new_entries[1].start_lba = target_dir;  // 老爸指向目標目錄
            new_entries[1].type = SFS_TYPE_DIR;

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

uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* path) {
    char target_name[32];
    uint32_t parent_lba = simplefs_resolve_path(current_dir_lba, path, target_name);

    if (parent_lba == 0) return 0;
    // 如果解析完後 target_name 是空的 (例如 cd / )，代表目的地就是 parent_lba！
    if (target_name[0] == '\0') return parent_lba;

    return shallow_get_dir_lba(parent_lba, target_name);
}

// 爬樹法實作 pwd
void simplefs_getcwd(uint32_t current_lba, char* out_buffer) {
    // 1. 如果已經在根目錄，直接回傳 "/"
    if (current_lba == 1 || current_lba == 0) {
        out_buffer[0] = '/';
        out_buffer[1] = '\0';
        return;
    }

    char names[16][32]; // 最多支援 16 層深度的目錄
    int depth = 0;
    uint32_t curr = current_lba;

    // 2. 開始往上爬，直到撞到根目錄 (LBA == 1)
    while (curr != 1 && depth < 16) {
        // --- A. 讀取當前目錄，尋找 ".." 來得知老爸是誰 ---
        uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
        read_dir(curr, dir_buf);
        sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

        uint32_t parent_lba = 0;
        for (uint32_t i = 0; i < MAX_FILES; i++) {
            if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, "..") == 0) {
                parent_lba = entries[i].start_lba;
                break;
            }
        }
        kfree(dir_buf);

        if (parent_lba == 0) break; // 發生異常 (找不到老爸)

        // --- B. 讀取老爸的目錄，尋找「我」的名字 ---
        dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
        read_dir(parent_lba, dir_buf);
        entries = (sfs_file_entry_t*) dir_buf;

        for (uint32_t i = 0; i < MAX_FILES; i++) {
            // 如果某個目錄項的 LBA 剛好等於我們剛剛的 curr，那就是我的名字！
            if (entries[i].filename[0] != '\0' && entries[i].start_lba == curr) {
                strcpy(names[depth], entries[i].filename);
                break;
            }
        }
        kfree(dir_buf);

        // 準備往上一層爬
        curr = parent_lba;
        depth++;
    }

    // 3. 把收集到的名字「反向」拼裝起來 (例如從 [sub, folder1] 變成 /folder1/sub)
    int out_idx = 0;
    for (int i = depth - 1; i >= 0; i--) {
        out_buffer[out_idx++] = '/';
        int j = 0;
        while (names[i][j] != '\0') {
            out_buffer[out_idx++] = names[i][j++];
        }
    }
    out_buffer[out_idx] = '\0';
}

// --- VSF 封裝 API ---
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

int vfs_getcwd(uint32_t dir_lba, char* buffer) {
    if (mounted_part_lba == 0) return -1;

    // 直接把收到的目錄 LBA 傳給底層的爬樹引擎
    simplefs_getcwd(dir_lba, buffer);
    return 0;
}
