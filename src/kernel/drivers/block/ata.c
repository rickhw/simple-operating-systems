/**
 * @file src/kernel/drivers/block/ata.c
 * @brief Main logic and program flow for ata.c.
 *
 * This file handles the operations and logic associated with ata.c.
 */

// ATA: Advanced Technology Attachment
// LBA: Logical Block Addressing
#include "ata.h"
#include "utils.h"
#include "tty.h"
#include "io.h"

// ATA Primary Bus Ports
#define ATA_PORT_DATA       0x1F0
#define ATA_PORT_SECT_COUNT 0x1F2
#define ATA_PORT_LBA_LOW    0x1F3
#define ATA_PORT_LBA_MID    0x1F4
#define ATA_PORT_LBA_HIGH   0x1F5
#define ATA_PORT_DRV_HEAD   0x1F6
#define ATA_PORT_STATUS     0x1F7
#define ATA_PORT_COMMAND    0x1F7

// 純粹等待硬碟不忙碌 (BSY bit 清零)
static void ata_wait_bsy() {
    while (inb(ATA_PORT_STATUS) & 0x80);
}

// 等待硬碟準備好傳輸資料 (BSY 清零 且 DRQ 設為 1)
static void ata_wait_drq() {
    while (1) {
        uint8_t status = inb(ATA_PORT_STATUS);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            break;
        }
    }
}

// 產生約 400ns 的延遲，讓硬碟有時間更新狀態
static void ata_delay() {
    inb(ATA_PORT_STATUS);
    inb(ATA_PORT_STATUS);
    inb(ATA_PORT_STATUS);
    inb(ATA_PORT_STATUS);
}

// --- 公開 API ---

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy(); // 開始前先確認硬碟有空

    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECT_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, 0x20); // 讀取

    ata_delay();    // 給硬碟一點時間掛上 BSY 旗標
    ata_wait_drq(); // 讀取前，等待硬碟說「我有資料要給你」(DRQ=1)

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PORT_DATA);
    }

    ata_wait_bsy(); // 讀完後確認硬碟收尾完成
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy();

    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECT_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, 0x30);

    ata_delay();
    ata_wait_drq();

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, ptr[i]);
    }

    // 必須先等硬碟把剛才的資料寫完（BSY 清零）！
    ata_delay();
    ata_wait_bsy();

    // 現在才可以安全地下達 Cache Flush
    outb(ATA_PORT_COMMAND, 0xE7);

    // 再次等待 Flush 完成
    ata_delay();
    ata_wait_bsy();
}
