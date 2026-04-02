#include "io.h"
#include "rtl8139.h"
#include "pci.h"
#include "tty.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "utils.h"

// 儲存網卡的 I/O 基準位址與 MAC 位址
static uint32_t rtl_iobase = 0;
static uint8_t mac_addr[6];

// ==========================================
// 【Day 83 新增】接收環狀緩衝區 (Rx Ring Buffer)
// 網卡要求最少 8K (8192) bytes，外加 16 bytes 的檔頭與 1500 bytes 容錯
// __attribute__((aligned(4096))) 確保它在實體記憶體上是 4K 對齊的
// ==========================================
static uint8_t rx_buffer[8192 + 16 + 1500] __attribute__((aligned(4096)));

// [day84] 用來追蹤目前讀取到緩衝區的哪個位置
static uint16_t rx_offset = 0;

// ==========================================
// 【Day 88 終極修復】網卡發送函式 (支援 4 槽輪詢)
// ==========================================
static uint8_t tx_buffer[1536] __attribute__((aligned(4096)));
static uint8_t tx_cur = 0; // 記錄現在輪到哪一個發送槽 (0~3)


void init_rtl8139(uint8_t bus, uint8_t slot) {
    kprintf("[RTL8139] Initializing driver...\n");

    // 1. 取得 I/O Base Address
    // 網卡的 Base Address Register 0 (BAR0) 存在 PCI 設定空間的 Offset 0x10
    // 因為 BAR0 是 32-bit，我們用兩次 16-bit 讀取來組合它
    uint32_t bar0_low = pci_read_config_word(bus, slot, 0, 0x10);
    uint32_t bar0_high = pci_read_config_word(bus, slot, 0, 0x12);

    // 組合出 32-bit 的位址，並清除最低 2 個 bit (PCI 規範，用來標示這是一塊 I/O 空間)
    rtl_iobase = (bar0_low | (bar0_high << 16)) & ~3;
    kprintf("[RTL8139] I/O Base Address found at: [%x]\n", rtl_iobase);

    // ==========================================
    // 【Day 85 修復】啟用 PCI Bus Mastering！
    // 允許網卡使用 DMA 跨過 CPU 直接讀取主記憶體
    // ==========================================
    uint16_t pci_cmd = pci_read_config_word(bus, slot, 0, 0x04);
    // 0x0004 就是 Bit 2 (Bus Master Enable)
    pci_write_config_word(bus, slot, 0, 0x04, pci_cmd | 0x0004);

    // 2. 開機 (Power On)
    // RTL8139 的 CONFIG1 暫存器 (Offset 0x52)，寫入 0x00 即可喚醒
    outb(rtl_iobase + 0x52, 0x00);

    // 3. 軟體重置 (Software Reset)
    // 寫入 0x10 到 Command 暫存器 (Offset 0x37) 觸發重置
    outb(rtl_iobase + 0x37, 0x10);

    // 不斷讀取，直到 0x10 這個 bit 被硬體清空，代表重置完成
    while ((inb(rtl_iobase + 0x37) & 0x10) != 0) {
        // 等待硬體重置...
    }
    kprintf("[RTL8139] Device reset successful.\n");

    // 4. 讀取 MAC 位址
    // RTL8139 的 MAC 位址直接存在 I/O Base 的最前面 6 個 Byte (Offset 0x00 ~ 0x05)
    for (int i = 0; i < 6; i++) {
        mac_addr[i] = inb(rtl_iobase + i);
    }

    kprintf("[RTL8139] MAC Address: [%x:%x:%x:%x:%x:%x]\n",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);


    // ==========================================
    // 【Day 83 新增】網卡接收引擎與中斷設定
    // ==========================================

    // 5. 將我們準備好的 Rx Buffer 實體位址告訴網卡 (寫入 RBSTART 暫存器 0x30)
    // 註：這裡假設你的 Kernel 是 Identity Mapped (虛擬位址 == 實體位址)
    outl(rtl_iobase + 0x30, (uint32_t)rx_buffer);

    // 6. 設定中斷遮罩 (IMR - Interrupt Mask Register, 0x3C)
    // 0x0005 代表我們只想監聽 "Rx OK (接收成功)" 和 "Tx OK (發送成功)" 的中斷
    outw(rtl_iobase + 0x3C, 0x0005);

    // 7. 設定接收配置暫存器 (RCR - Receive Configuration Register, 0x44)
    // (AB|AM|APM|AAP) = 0xF 代表允許接收：廣播、多播、MAC符合的封包
    // WRAP (bit 7) = 1 代表當 Buffer 滿了時，網卡會自動從頭覆寫 (環狀機制)
    outl(rtl_iobase + 0x44, 0xf | (1 << 7));

    // 8. 正式啟動接收 (Rx) 與發送 (Tx) 引擎！
    // Command Register (0x37), RE(bit 3)=1, TE(bit 2)=1 -> 組合起來就是 0x0C
    outb(rtl_iobase + 0x37, 0x0C);

    // 9. 從 PCI Config Space 讀出這張網卡被分配到的 IRQ 號碼
    uint32_t irq_info = pci_read_config_word(bus, slot, 0, 0x3C);
    uint8_t irq_line = irq_info & 0xFF; // 取出最低 8 bits

    kprintf("[RTL8139] Hardware initialized. Assigned IRQ: [%d]\n", irq_line);
}


// ==========================================
// 【Day 84 新增】RTL8139 中斷處理常式
// ==========================================
void rtl8139_handler(void) {
    // 1. 讀取中斷狀態暫存器 (ISR, Offset 0x3E)
    uint16_t status = inw(rtl_iobase + 0x3E);

    if (!status) return;

    // 2. 寫回 ISR 來清除中斷旗標 (寫入 1 代表清除)
    outw(rtl_iobase + 0x3E, status);

    // 3. 檢查是否為 "Receive OK" (Bit 0)
    if (status & 0x01) {

        // 不斷讀取，直到硬體告訴我們 Buffer 空了 (CR 暫存器的 Bit 0 為 1 代表空)
        while ((inb(rtl_iobase + 0x37) & 0x01) == 0) {

            // RTL8139 封包的開頭會有 4 個 bytes 的硬體檔頭：
            // [ 16-bit Rx Status ] [ 16-bit Packet Length ]
            uint16_t* rx_header = (uint16_t*)(rx_buffer + rx_offset);
            uint16_t rx_status = rx_header[0];
            uint16_t rx_length = rx_header[1];

            // ==========================================
            // 【Day 84 修復】防禦性編程：避免無限迴圈！
            // 如果長度為 0，或者 Rx Status 的 Bit 0 (ROK) 不為 1，
            // 代表這是一筆尚未完成 DMA 或無效的髒資料，立刻中斷讀取！
            // ==========================================
            if (rx_length == 0 || (rx_status & 0x01) == 0) {
                break;
            }

            // 實際的網路封包位址 (跳過那 4 bytes 硬體檔頭)
            uint8_t* packet_data = rx_buffer + rx_offset + 4;

            // 將封包轉型為乙太網路標頭！
            ethernet_header_t* eth = (ethernet_header_t*)packet_data;

            kprintf("[Net] Packet RX! Len: [%d] bytes\n", rx_length);
            kprintf("  -> Dest MAC: [%x:%x:%x:%x:%x:%x]\n",
                    eth->dest_mac[0], eth->dest_mac[1], eth->dest_mac[2],
                    eth->dest_mac[3], eth->dest_mac[4], eth->dest_mac[5]);

            uint16_t type = ntohs(eth->ethertype);
            if (type == ETHERTYPE_ARP) {
                // 轉型為 ARP 封包
                arp_packet_t* arp = (arp_packet_t*)(packet_data + sizeof(ethernet_header_t));
                uint16_t arp_op = ntohs(arp->opcode);

                kprintf("  -> Protocol: ARP\n");
                kprintf("  -> ARP OPCode: [%d]\n", arp_op);

                // 如果這是一封 ARP Reply (Opcode == 2)
                if (arp_op == ARP_REPLY) {
                    // 呼叫我們剛剛寫的 API，把對方的 IP 和 MAC 記下來！
                    extern void arp_update_table(uint8_t* ip, uint8_t* mac);
                    arp_update_table(arp->src_ip, arp->src_mac);
                }
                // ==========================================
                // 【Day 88 新增】如果別人在找我們，立刻回覆！
                // ==========================================
                else if (arp_op == ARP_REQUEST) {
                    // 檢查他找的 IP 是不是我們？(10.0.2.15)
                    uint8_t my_ip[4] = {10, 0, 2, 15};
                    if (memcmp(arp->dest_ip, my_ip, 4) == 0) {
                        kprintf("[ARP] Someone is asking for our MAC! Replying...\n");
                        extern void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac);
                        // 把發問者的 IP 和 MAC 傳給回覆函式
                        arp_send_reply(arp->src_ip, arp->src_mac);
                    }
                }
            }
            else if (type == ETHERTYPE_IPv4) {
                kprintf("  -> Protocol: IPv4\n");
                // 跳過 Ethernet Header
                ipv4_header_t* ip = (ipv4_header_t*)(packet_data + sizeof(ethernet_header_t));
                if (ip->protocol == 1) { // 1 代表 ICMP
                    icmp_header_t* icmp = (icmp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));
                    if (icmp->type == 0) { // 0 代表 Echo Reply
                        kprintf("[Ping] Reply received from [%d.%d.%d.%d]!\n",
                                ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3]);
                    }
                }
            }
            else {
                kprintf("  -> Protocol: Unknown (%x)\n", type);
            }

            // 4. 更新 Rx_offset
            // 長度包含 4 bytes CRC，且必須對齊 4 bytes 邊界
            rx_offset = (rx_offset + rx_length + 4 + 3) & ~3;

            // 若超過 8K (8192) 邊界，會繞回開頭 (Wrap around)
            if (rx_offset >= 8192) {
                rx_offset -= 8192;
            }

            // 5. 告訴網卡我們讀到哪裡了 (寫入 CAPR 暫存器 0x38)
            // (註：硬體設計怪癖，必須減掉 16)
            outw(rtl_iobase + 0x38, rx_offset - 16);
        }
    }

    // 發送 EOI 給中斷控制器 (PIC)
    // 因為 IRQ 11 接在 Slave PIC 上，所以要發給 Master & Slave
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}


// void rtl8139_send_packet(void* data, uint32_t len) {
//     // 1. 將資料拷貝到對齊的實體緩衝區
//     memcpy(tx_buffer, data, len);

//     // 2. 告訴網卡緩衝區的位址 (TSAD0 = 0x20)
//     outl(rtl_iobase + 0x20, (uint32_t)tx_buffer);

//     // 3. 開始傳輸！設定長度並觸發 (TSD0 = 0x10)
//     // 我們將傳輸門檻設為 0 (立刻開始)，Bit 0~12 是長度
//     outl(rtl_iobase + 0x10, len & 0x1FFF);

//     // kprintf("[Net] Packet Sent! Len: %d\n", len);
// }

void rtl8139_send_packet(void* data, uint32_t len) {
    uint32_t send_len = len;
    // 乙太網路最小限制 60 Bytes，不足要補 0 (Padding)
    if (send_len < 60) send_len = 60;

    // 計算當前要用的暫存器位址 (每個槽相差 4 bytes)
    // TSD: 0x10, 0x14, 0x18, 0x1C
    // TSAD: 0x20, 0x24, 0x28, 0x2C
    uint32_t tsd_port = rtl_iobase + 0x10 + (tx_cur * 4);
    uint32_t tsad_port = rtl_iobase + 0x20 + (tx_cur * 4);

    // 等待當前的發送槽有空 (Bit 13 為 1 代表 Owned by Host)
    while ((inl(tsd_port) & (1 << 13)) == 0) {
        // Busy wait (等待上一次這個槽的資料送完)
    }

    // 將資料拷貝到實體緩衝區 (先清零以防夾帶垃圾資料)
    memset(tx_buffer, 0, send_len);
    memcpy(tx_buffer, data, len);

    // 寫入實體位址到 TSAD
    outl(tsad_port, (uint32_t)tx_buffer);
    // 寫入長度並觸發發送 TSD
    outl(tsd_port, send_len & 0x1FFF);

    // 輪到下一個發送槽 (0->1->2->3->0)
    tx_cur = (tx_cur + 1) % 4;
}

// [Day 85 新增] 讓其他模組取得本機的 MAC 位址
uint8_t* rtl8139_get_mac(void) {
    return mac_addr;
}
