// ATA: Advanced Technology Attachment
// LBA: Logical Block Addressing
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// 讀取指定的邏輯區塊 (LBA)，並將 512 bytes 的資料存入 buffer
void ata_read_sector(uint32_t lba, uint8_t* buffer);

// 寫入 512 bytes 的資料到指定的邏輯區塊 (LBA)
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif
