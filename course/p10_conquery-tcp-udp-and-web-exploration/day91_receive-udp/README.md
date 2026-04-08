哈哈，Rick！這兩張截圖我看了都忍不住跟著你在螢幕前歡呼了！🎉🎉🎉

你真的做到了！從最底層的硬體暫存器，一路打通 PCI、網卡驅動、ARP、IPv4，到現在成功把一串人類可以閱讀的文字 `"Hello QEMU"` 包裝在 UDP 封包裡送進網路線！
你看看 Wireshark 裡面那個完美的淺藍色高光，那就是你親手打造的作業系統與真實網路世界對話的鐵證！這絕對是 OS 開發中最迷人、最有成就感的一刻！

既然你的 OS 已經有了**「說話 (發送 UDP)」**的能力，接下來當然要賦予它**「聆聽 (接收 UDP)」**的能力！

**Day 91：從 Host (macOS) 傳送訊息給 Guest (Simple OS)！**

有去有回才叫通訊。今天我們要設定一個「接收器」，並且透過 QEMU 的連接埠轉發 (Port Forwarding) 功能，讓你可以在你 **macOS 的終端機** 裡打字，然後直接傳送到你的 Simple OS 畫面上！

請跟著我進行這 3 個步驟：

---

### 步驟 1：打通 macOS 與 QEMU 的蟲洞 (`Makefile`)

QEMU 的 SLIRP 網路預設是「單向」的（Guest 可以主動找 Host，但 Host 找不到 Guest）。為了讓 macOS 能傳資料給 Simple OS，我們要設定 `hostfwd`，把 macOS 的 UDP Port 8080 綁定到 Simple OS 的 UDP Port 8080。

打開你的 **`Makefile`**，找到 `run` 的區塊，修改 `-netdev` 那一行：

```makefile
run: all hdd.img
	qemu-system-i386 -cdrom myos.iso -monitor stdio \
	  -netdev user,id=n0,hostfwd=udp::8080-:8080 \
	  -device rtl8139,netdev=n0 \
	  -object filter-dump,id=f1,netdev=n0,file=dump.pcap \
	  -hda hdd.img \
	  -boot d
```
*(💡 注意：多加了 `,hostfwd=udp::8080-:8080`，這代表 macOS 的 `localhost:8080` 會直通虛擬機的 `8080` 埠。)*

---

### 步驟 2：實作 UDP 接收解析邏輯 (`src/kernel/lib/rtl8139.c`)

當網卡收到封包時，我們要學會「剝掉」IPv4 的外衣，取出 UDP 標頭與資料。

打開 **`src/kernel/lib/rtl8139.c`**，找到 `rtl8139_handler` 裡處理 `IPv4` 的地方，加入 `UDP (Protocol 17)` 的處理：

```c
            else if (type == ETHERTYPE_IPv4) {
                // 跳過 Ethernet Header
                ipv4_header_t* ip = (ipv4_header_t*)(packet_data + sizeof(ethernet_header_t));
                
                if (ip->protocol == 1) { // 1 代表 ICMP (Ping)
                    icmp_header_t* icmp = (icmp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));
                    if (icmp->type == 0) {
                        kprintf("[Ping] Reply received from %d.%d.%d.%d!\n",
                                ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3]);
                    }
                } 
                // ==========================================
                // 【Day 91 新增】17 代表 UDP！
                // ==========================================
                else if (ip->protocol == 17) { 
                    // 跳過 Ethernet Header 和 IPv4 Header
                    udp_header_t* udp = (udp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));
                    
                    // Payload 的位置在 UDP Header 之後
                    uint8_t* payload = (uint8_t*)udp + sizeof(udp_header_t);
                    
                    // 計算真實資料的長度 (UDP 總長度 - UDP 標頭長度)
                    uint16_t payload_len = ntohs(udp->length) - sizeof(udp_header_t);
                    
                    // 為了安全印出字串，我們把它拷貝到一個有 Null-terminator 的 Buffer
                    char msg_buf[128] = {0};
                    int copy_len = (payload_len < 127) ? payload_len : 127;
                    memcpy(msg_buf, payload, copy_len);

                    kprintf("[UDP Rx] %d.%d.%d.%d:%d -> Port %d: '%s'\n",
                            ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3],
                            ntohs(udp->src_port), ntohs(udp->dest_port), msg_buf);
                }
            }
```
*(💡 記得在最上方確定有 `#include "udp.h"`)*

---

### 🚀 步驟 3：體驗跨維度通訊！

這是見證奇蹟的時刻！

1. 執行 `make clean && make run` 啟動你的 Simple OS，停在 `SimpleOS>` 提示字元即可，**什麼都不用做**。
2. **打開你 macOS 的終端機 (Terminal 或 iTerm2)**。
3. 在 macOS 裡輸入大名鼎鼎的 `nc` (Netcat) 指令，向我們剛才打通的蟲洞 (Port 8080) 發送 UDP 封包：
   ```bash
   echo "Hello from macOS!" | nc -u -w 1 127.0.0.1 8080
   ```

**轉頭看看你的 QEMU 視窗！**
如果一切順利，你的 Simple OS 會瞬間印出：
**`[UDP Rx] 10.0.2.2:xxxx -> Port 8080: 'Hello from macOS!'`**

成功把真實世界 (macOS) 的聲音傳遞進去你的虛擬 OS 裡了嗎？這招叫做「穿透次元壁」，趕快來試試看！😎
