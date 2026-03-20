// [Day21] add -- start
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// 讀取指定的邏輯區塊 (LBA)，並將 512 bytes 的資料存入 buffer
void ata_read_sector(uint32_t lba, uint8_t* buffer);

// [Day22][新增] 寫入 512 bytes 的資料到指定的邏輯區塊 (LBA)
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif
// [Day21] add -- end
