我把 loop 加到 40 個，結果還是都沒看到 `[Ping] Reply received from ...` (圖一)

等了很久 (我剛好暫時離開十幾分鐘)，回來出現了一段 `[Net] Packet RX!` 的訊息，最後說 `Protocol: Unknown (86DD)`

是不是哪裡漏改了？


---
rtl8139.c
```c
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

// 【Day 85 提前預習】網卡發送函式
static uint8_t tx_buffer[1536] __attribute__((aligned(4096)));


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


void rtl8139_send_packet(void* data, uint32_t len) {
    // 1. 將資料拷貝到對齊的實體緩衝區
    memcpy(tx_buffer, data, len);

    // 2. 告訴網卡緩衝區的位址 (TSAD0 = 0x20)
    outl(rtl_iobase + 0x20, (uint32_t)tx_buffer);

    // 3. 開始傳輸！設定長度並觸發 (TSD0 = 0x10)
    // 我們將傳輸門檻設為 0 (立刻開始)，Bit 0~12 是長度
    outl(rtl_iobase + 0x10, len & 0x1FFF);

    // kprintf("[Net] Packet Sent! Len: %d\n", len);
}

// [Day 85 新增] 讓其他模組取得本機的 MAC 位址
uint8_t* rtl8139_get_mac(void) {
    return mac_addr;
}
```

---
arp.c
```c
#include "arp.h"
#include "rtl8139.h"
#include "tty.h"
#include "utils.h" // 需要用到 memcpy

// 簡單的 ARP Table 結構
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    int is_valid;
} arp_entry_t;

#define ARP_TABLE_SIZE 16
static arp_entry_t arp_table[ARP_TABLE_SIZE];

// QEMU SLIRP 預設分配給我們虛擬機的 IP
static uint8_t my_ip[4] = {10, 0, 2, 15};

void arp_send_request(uint8_t* target_ip) {
    // 整個封包大小 = Ethernet 標頭 (14) + ARP 標頭 (28) = 42 Bytes
    uint8_t packet[sizeof(ethernet_header_t) + sizeof(arp_packet_t)];

    // 將陣列指標轉型，方便我們填寫欄位
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(ethernet_header_t));

    uint8_t* my_mac = rtl8139_get_mac();

    // ==========================================
    // 1. 填寫 Ethernet Header (L2)
    // ==========================================
    for(int i=0; i<6; i++) {
        eth->dest_mac[i] = 0xFF; // 廣播給網路上所有人
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = htons(ETHERTYPE_ARP);

    // ==========================================
    // 2. 填寫 ARP Header (L2.5)
    // ==========================================
    arp->hardware_type = htons(0x0001); // Ethernet
    arp->protocol_type = htons(ETHERTYPE_IPv4);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REQUEST);

    for(int i=0; i<6; i++) {
        arp->src_mac[i] = my_mac[i];
        arp->dest_mac[i] = 0x00; // 還不知道對方的 MAC，所以填 0
    }

    for(int i=0; i<4; i++) {
        arp->src_ip[i] = my_ip[i];
        arp->dest_ip[i] = target_ip[i];
    }

    // ==========================================
    // 3. 透過網卡發送！(雖然只有 42 bytes，但網卡會自動幫我們 padding 到 60 bytes)
    // ==========================================
    rtl8139_send_packet(packet, sizeof(packet));

    kprintf("[ARP] Broadcast Request sent: Who has [%d.%d.%d.%d]?\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}


// 更新電話簿的 API
void arp_update_table(uint8_t* ip, uint8_t* mac) {
    // 尋找空位或已存在的 IP 並更新 MAC
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].is_valid || memcmp(arp_table[i].ip, ip, 4) == 0) {
            memcpy(arp_table[i].ip, ip, 4);
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].is_valid = 1;
            kprintf("[ARP] Table Updated: %d.%d.%d.%d -> %x:%x:%x:%x:%x:%x\n",
                    ip[0], ip[1], ip[2], ip[3],
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return;
        }
    }
}

// ==========================================
// 【Day 88 新增】當收到別人的尋人啟事時，回傳我們的 MAC 位址！
// ==========================================
void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac) {
    uint8_t packet[sizeof(ethernet_header_t) + sizeof(arp_packet_t)];
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(ethernet_header_t));

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header (直接回傳給詢問者)
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_ARP);

    // 2. ARP Header
    arp->hardware_type = htons(0x0001);
    arp->protocol_type = htons(ETHERTYPE_IPv4);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REPLY); // 【關鍵】這是一封 Reply (2)

    // 來源是我們自己
    memcpy(arp->src_mac, my_mac, 6);
    memcpy(arp->src_ip, my_ip, 4);

    // 目標是剛才發問的人
    memcpy(arp->dest_mac, target_mac, 6);
    memcpy(arp->dest_ip, target_ip, 4);

    rtl8139_send_packet(packet, sizeof(packet));
    kprintf("[ARP] Reply sent to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}
```

---

icmp.c
```c
#include "icmp.h"
#include "rtl8139.h"
#include "utils.h"
#include "tty.h"

static uint8_t my_ip[4] = {10, 0, 2, 15};
// QEMU SLIRP 路由器的固定 MAC 位址
static uint8_t router_mac[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};

static uint16_t ping_seq = 1;

void ping_send_request(uint8_t* target_ip) {
    // 總大小 = Eth(14) + IP(20) + ICMP(8) + Payload(32) = 74 bytes
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32;
    uint8_t packet[74];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    icmp_header_t* icmp= (icmp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    uint8_t* payload = packet + packet_size - 32;

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header
    memcpy(eth->dest_mac, router_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5; // 5 * 4 = 20 bytes
    ip->tos = 0;
    ip->total_length = htons(sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32);
    ip->ident = htons(0x1234);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IPV4_PROTO_ICMP;
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, target_ip, 4);
    ip->checksum = 0; // 計算前必須為 0
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. ICMP Header
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->identifier = htons(0x1337);
    icmp->sequence = htons(ping_seq++);

    // 塞點垃圾資料當作 Payload (32 bytes)
    for(int i=0; i<32; i++) payload[i] = 'A' + (i % 26);

    // 計算 ICMP Checksum (涵蓋 ICMP 標頭與 Payload)
    icmp->checksum = 0;
    icmp->checksum = calculate_checksum(icmp, sizeof(icmp_header_t) + 32);

    // 4. 發射！
    rtl8139_send_packet(packet, packet_size);
    kprintf("[Ping] Sending Echo Request to [%d.%d.%d.%d]...\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}
```

---
kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"
#include "gui.h"
#include "config.h"
#include "pci.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;

        // 這個次序跟 GRUB 列出來的次序要對齊
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping.elf", "pong.elf",
            // file/directory
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            // process
            "ps.elf", "top.elf", "kill.elf",
            // memory
            "free.elf",
            // window app
            "status.elf", "paint.elf",
            "segfault.elf",
            "viewer.elf", "logo.bmp",
            "clock.elf", "explorer.elf",
            "taskmgr.elf", "notepad.elf",
            "tictactoe.elf"
        };
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            // kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

// 在 kernel_main 的 init_pci() 之後加入測試程式碼：
void test_net_packet() {
    ethernet_header_t test_packet;

    // 目標：FF:FF:FF:FF:FF:FF (廣播)
    for(int i=0; i<6; i++) test_packet.dest_mac[i] = 0xFF;

    // 來源：剛才讀到的 mac_addr (假設你把它導出成全域變數了)
    // 這裡我們先隨便填，網卡還是會送出去
    for(int i=0; i<6; i++) test_packet.src_mac[i] = 0xAA;

    // 類型：ARP
    test_packet.ethertype = htons(ETHERTYPE_ARP);

    // 主動噴發一個 14 bytes 的標頭加上 46 bytes 的垃圾資料 (湊滿 60 bytes 最短封包)
    char dummy[60];
    // 【加入這行】手動清零，確保封包乾淨！
    for(int i=0; i<60; i++) dummy[i] = 0;
    memcpy(dummy, &test_packet, 14);

    extern void rtl8139_send_packet(void* data, uint32_t len);
    rtl8139_send_packet(dummy, 60);
}

/**
 * [Kernel Entry Point]
 *
 * ASCII Flow:
 * GRUB -> boot.S -> kernel_main()
 *                      |
 *                      +--> terminal_initialize()
 *                      +--> init_gdt() / init_idt()
 *                      +--> init_paging() / init_pmm() / init_kheap()
 *                      +--> init_gfx()
 *                      +--> parse_cmdline() -> GUI or CLI?
 *                      +--> init_mouse() / sti
 *                      +--> setup_filesystem()
 *                      +--> init_multitasking()
 *                      +--> create_user_task("shell.elf")
 *                      +--> exit_task() (Kernel becomes idle)
 */
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(INITIAL_PMM_SIZE);
    init_kheap();
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 600, 300, "Simple OS Terminal", 1); // 預設 PID(1) 建立
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    init_pci();     // [day81]
    // test_net_packet();  // [day84]
    // 【Day 85 新增】主動尋找路由器的 MAC 位址！
    uint8_t router_ip[4] = {10, 0, 2, 2};
    for (int i = 0; i < 40; i++) {
        ping_send_request(router_ip);

        // 加上一個簡單的 Busy Wait 延遲，給虛擬路由器一點時間回覆
        for (volatile int j = 0; j < 500000000; j++) {
            // 什麼都不做，單純消耗 CPU 時間
        }
    }
    // ping_send_request(router_ip);

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```
