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

// [Day48][Add] start
// 拔掉原本的 read_root_dir / write_root_dir，換成這兩個通用的：
static void read_dir(uint32_t dir_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_read_sector(dir_lba + i, buffer + (i * 512));
    }
}
static void write_dir(uint32_t dir_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(dir_lba + i, buffer + (i * 512));
    }
}
// [Day48][Add] end

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
    write_dir(partition_start_lba, empty_dir); // 寫入空白的巨型根目錄

    kfree(sb);
    kfree(empty_dir);
    kprintf("[SimpleFS] Format complete. Root Dir capacity: %d files.\n", MAX_FILES);
}

void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(part_lba + 1, dir_buf);
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
    read_dir(mounted_part_lba + 1, dir_buf);

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
    read_dir(part_lba + 1, dir_buf);
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

    write_dir(part_lba + 1, dir_buf);
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

// 【修改】新增第五個參數 out_type
int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;

                // 【關鍵點】把地契上的 Type 取出來，還給 User Space！
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

// 【修改】同步更新 VFS 的封裝
int vfs_readdir(int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size, out_type);
}

int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            entries[i].filename[0] = '\0';
            write_dir(part_lba + 1, dir_buf);
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
    read_dir(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR;
            entries[i].start_lba = part_lba + 20 + i;

            // ===============================================
            // 【新增】確保新資料夾裡面是乾淨的 (寫入空白的區塊)
            // ===============================================
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            write_dir(entries[i].start_lba, empty_dir);
            kfree(empty_dir);

            write_dir(part_lba + 1, dir_buf);
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


// [Day48][Add] start
// 【新增】尋找子目錄的專用函式
uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(current_dir_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, dirname) == 0) {
            if (entries[i].type == FS_DIR) {
                uint32_t target_lba = entries[i].start_lba;
                kfree(dir_buf);
                return target_lba; // 找到了！回傳這個資料夾的 LBA
            }
        }
    }
    kfree(dir_buf);
    return 0; // 找不到，或它根本不是一個資料夾
}
// [Day48][Add] end
