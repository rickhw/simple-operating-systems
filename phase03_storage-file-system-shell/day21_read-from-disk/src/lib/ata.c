// [Day21] add -- start
#include "ata.h"
#include "utils.h"
#include "tty.h"
#include "io.h"

// ATA Primary Bus Ports
#define ATA_PORT_DATA       0x1F0
#define ATA_PORT_ERROR      0x1F1
#define ATA_PORT_SECT_COUNT 0x1F2
#define ATA_PORT_LBA_LOW    0x1F3
#define ATA_PORT_LBA_MID    0x1F4
#define ATA_PORT_LBA_HIGH   0x1F5
#define ATA_PORT_DRV_HEAD   0x1F6
#define ATA_PORT_STATUS     0x1F7
#define ATA_PORT_COMMAND    0x1F7

// 等待硬碟不忙碌 (BSY bit 清零) 且資料準備好 (DRQ bit 設為 1)
static void ata_wait_ready() {
    while (1) {
        uint8_t status = inb(ATA_PORT_STATUS);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            break; // 0x80 是 Busy, 0x08 是 Data Request Ready
        }
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    // 1. 選擇 Master 硬碟 (0xE0) 與 LBA 模式，並設定 LBA 的最高 4 bits
    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));

    // 2. 告訴硬碟我們要讀取 1 個磁區 (Sector)
    outb(ATA_PORT_SECT_COUNT, 1);

    // 3. 將 24-bit 的 LBA 位址拆成三份送出去
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));

    // 4. 下達「讀取磁區 (Read Sectors)」的指令 (0x20)
    outb(ATA_PORT_COMMAND, 0x20);

    // 5. 等待硬碟把資料搬到控制器的緩衝區
    ata_wait_ready();

    // 6. 苦力時間：用 16-bit (2 bytes) 為單位，把 512 bytes 讀進來 (需要讀 256 次)
    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PORT_DATA);
    }
}
// [Day21] add -- end
