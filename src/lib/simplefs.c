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

    // ==========================================
    // 【新增】UNIX 魔法：為根目錄建立 . 與 ..
    // ==========================================
    sfs_file_entry_t* root_entries = (sfs_file_entry_t*)empty_dir;

    // 自己 (.)
    strcpy(root_entries[0].filename, ".");
    root_entries[0].start_lba = 1;
    root_entries[0].type = FS_DIR;

    // 老爸 (..) -> 根目錄的老爸還是根目錄
    strcpy(root_entries[1].filename, "..");
    root_entries[1].start_lba = 1;
    root_entries[1].type = FS_DIR;
    // ==========================================

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

            uint32_t new_dir_lba = simplefs_alloc_blocks(ROOT_DIR_SECTORS);
            entries[i].start_lba = new_dir_lba;

            // ===============================================
            // 【新增】UNIX 魔法：初始化新目錄的 . 和 ..
            // ===============================================
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            sfs_file_entry_t* new_entries = (sfs_file_entry_t*)empty_dir;

            // 自己 (.) 指向剛剛申請到的 new_dir_lba
            strcpy(new_entries[0].filename, ".");
            new_entries[0].start_lba = new_dir_lba;
            new_entries[0].type = FS_DIR;

            // 老爸 (..) 指向建立它的目錄 dir_lba_rel
            strcpy(new_entries[1].filename, "..");
            new_entries[1].start_lba = dir_lba_rel;
            new_entries[1].type = FS_DIR;

            // 把初始化好的目錄內容寫入新空間
            write_dir(new_dir_lba, empty_dir);
            kfree(empty_dir);
            // ===============================================

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
