等一下，我檢查了 ARP_RELAY 這段邏輯的 code 是對的，但我把 opcode 印出來發現結果是錯的！所以邏輯根本沒有進去！

rtl8139_handler 裡我用 `kprintf("    ARP OPCode: [%d]\n", ntohs(arp->opcode));` 印出 opcode 結果是 `21077`

回去看 arp_send_request 裡送出的 opcode 好像不對？幫我確認一下 ．．．


arp.c
```c
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
```

---

rtl8139.c
```c
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
                kprintf("  -> Protocol: ARP\n");

                // 轉型為 ARP 封包
                arp_packet_t* arp = (arp_packet_t*)packet_data;

                kprintf("    ARP OPCode: [%d]\n", ntohs(arp->opcode));

                // 如果這是一封 ARP Reply (Opcode == 2)
                if (ntohs(arp->opcode) == ARP_REPLY) {
                    // 呼叫我們剛剛寫的 API，把對方的 IP 和 MAC 記下來！
                    extern void arp_update_table(uint8_t* ip, uint8_t* mac);
                    arp_update_table(arp->src_ip, arp->src_mac);
                }
            }
            else if (type == ETHERTYPE_IPv4) {
                kprintf("  -> Protocol: IPv4\n");
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
}```
