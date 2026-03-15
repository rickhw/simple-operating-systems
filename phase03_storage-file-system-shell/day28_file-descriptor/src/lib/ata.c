// [Day21] add -- start
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

// [新增] 純粹等待硬碟不忙碌 (BSY bit 清零)
static void ata_wait_bsy() {
    while (inb(ATA_PORT_STATUS) & 0x80);
}

// [修改] 等待硬碟準備好傳輸資料 (BSY 清零 且 DRQ 設為 1)
static void ata_wait_drq() {
    while (1) {
        uint8_t status = inb(ATA_PORT_STATUS);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            break;
        }
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECT_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, 0x20); // 讀取

    // 讀取前，等待硬碟說「我有資料要給你」(DRQ=1)
    ata_wait_drq();

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PORT_DATA);
    }
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PORT_SECT_COUNT, 1);
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PORT_COMMAND, 0x30); // 寫入

    // 寫入前，等待硬碟說「我準備好接你的資料了」(DRQ=1)
    ata_wait_drq();

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, ptr[i]);
    }

    // 強制寫入碟片
    outb(ATA_PORT_COMMAND, 0xE7);

    // [關鍵修復] 這裡只要等硬碟不忙就好，不能等 DRQ！
    ata_wait_bsy();
}
// [Day21] add -- end
