[自幹作業系統][ct1] 完成第一階段之後，感覺意猶未竟，有很多東西想繼續玩玩看，所以開啟了第二階段，其中最複雜也數最有趣的是 `網路基礎 (Networking Fundamentals)` 部分，從最底層硬體 PCI Bus 掃描，實作驅動程式 (RTL8139)、到 Ethernet Layer (L2)、IPv4、ARP、ARP Table、UDP、TCP ... 一路打上去，最後用 ping、host、wget、udprecv、tcptest 等 User Space 封裝的應用程式做驗收。

下圖是我請 Gemini 幫我畫的總結，總結第二階段網路部分的學習。

![](/about/NetworkOverview_20260408.png)


<!-- more -->


---

## 實作內容

網路實作包含以下：

- Kernel Space：
    - 掃描 PCI Bus
    - 驅動網路卡 RTL8139: 在 QEMU 新增網卡設定
    - 實作 Ethernet Layer (L2), IPv4, ARP, ICMP, UDP, TCP
- Application in User Space：
    - `ping`: 送出封包給 QEMU router
    - `udprecv`: 接收 UDP 封包
    - `tcptest`: 送出 TCP 封包，完成三項交握，同時包含資料
    - `host`: DNS resovler，需要調整 QEMU 網路卡的設定，讓他跟 host machine 對接
        - DNS 整個實作完全是在 User Space，沒有走 syscall
    - `wget`: 透過 `GET / HTTP/1.1` 取得 網站 內容，寫入到 `index.html`
- Socket API between Kernel and User Space

底下整理每個階段的驗證與結果截圖。


### Day81-83: 掃描 PCI Bus，驅動網路卡 RTL8139

如下圖，先執行 PCI Bus 掃描，圖中找到了一張網卡 `Realtek RTL8139`
然後初始 RTL8139 這張網卡，顯示他的 Mac Address 和 IRQ

![](/about/Networking/01-pci-scanning-and-drive-rtl8139.png)


### Day84: 接收網路卡收到的封包

解讀 L2 (ethernet layer) 的來源與目標 的 Mac Address

![](/about/Networking/02-ethernet-layer.png)


### Day85-86: 送出 ARP 廣播封包，暫存在 ARP Table

廣播請問誰的 IP 是 `10.0.2.2`，請回覆你的 Mac Address，然後存到 ARP Table 裡。

![](/about/Networking/03-arp-broadcast.png)

![](/about/Networking/04-arp-table.png)


### Day87-89: IPv4, ICMP, Ping (User Space)

實作 IPv4 與 ICMP 兩個協議, 底層網卡驅動 (rtl8139.c) 則會調用 ICMP 的實作，然後透過 Ping 封裝 ICMP 的 REPLY 

這個 ping 的位址 `10.0.2.2` 是 QEMU 的 Router 位址。

![](/about/Networking/06-ping-wireshark.png)

![](/about/Networking/07-ping-user-space.png)


### Day90-92: UDP receive and Socket API

先在 host machine，用 netcut 送一個 UDP 到 QENU router，然後確認 可以收到 UDP 封包，

在 User Space 提供了 Socket 的 Bind (Port) 和 Listen，透過 `udprecv` 這個 application 接收訊息。

![](/about/Networking/08-send-udp-from-macos.png)

![](/about/Networking/09-receive-in-sos.png)


### Day93-95: TCP, 3-Way Handshake, Connection

建立 TCP 封包，然後建立 SYN, ACK 三項交握，狀態為 `ESTABLISHED`


![](/about/Networking/11-tcp-3way-handshake.png)

底下兩張是用 `tcptest` 這個應用程式送出 `TCP_PSH` flag，把資料送透過 QEMU router 轉送給 Host Machine 的 nc 接收到資料 `Hello macOS, ...`，最後則是結束對話 `TCP_FIN` flag，完成整個交握。

![](/about/Networking/12-tcp-push-data.png)

![](/about/Networking/13-recv-data-over-tcp.png)


## Day96-98: DNS, Wget

TCP 打通後，就可以做真正應用層的東西，像是 DNS、wget 等。

![](/about/Networking/14-dns-resolver.png)

![](/about/Networking/15-wget.png)

---

## 重構

原本 [原始碼](https://github.com/rickhw/simple-operating-systems) 的結構很簡單，大概如下：

```bash
❯ tree -d -L 4
.
├── scripts/
└── src/
    ├── kernel/
    │   ├── asm/
    │   ├── include/
    │   └── lib/
    └── user/
        ├── asm/
        ├── bin/
        ├── include/
        └── lib/
```

只有分成 Kernel & User Space 兩層，裡面各有 組合語言 和 *.c 與 header ，但網路部分進來之後，整個結構就複雜了，所以重構 Kernel 結構，改成貼近 Linux Kernel 的結構如下。User space 則暫時不動。

```bash
src/kernel/
├── arch/x86/          # 處理器架構相關實作
├── drivers/           # 硬體驅動程式
├── fs/                # 檔案系統層
├── init/              # 系統引導與入口
│   └── main.c         # 核心主程式 (初始化所有次系統)
├── kernel/            # 核心邏輯層
├── mm/                # 記憶體管理
├── net/               # 網路協定棧
├── lib/               # 核心基礎函式庫 (utils, string, memory)
└── include/           # 核心標頭檔 (.h)
```



## 課程清單


### Phase 9：Network Awakening: From Hardware Scan to ICMP

**目標：啟動網卡驅動，實作基礎網路堆疊，驗收 `ping` 指令。**

網路是作業系統最龐大、但也最迷人的子系統。我們將由下而上，從實體層一路打通到網路層。

* **Day 81：PCI 匯流排掃描 (PCI Bus Scanning)**
    * 掃描主機板上的 PCI 配置空間 (Configuration Space)，尋找 QEMU 內建的網路卡 (Realtek RTL8139)。
* **Day 82-83：網卡驅動程式初始化 (NIC Driver Initialization)**
    * 啟動 RTL8139 網卡、讀取實體 MAC 位址、解開 PCI Bus Mastering 封印。
    * 設定 Rx 接收環狀緩衝區 (Ring Buffer) 與 IRQ 11 硬體中斷。
* **Day 84：乙太網路層與接收處理 (Ethernet Layer & Rx Handler)**
    * 定義乙太網路標頭 (Ethernet Header)。
    * 實作中斷驅動的 L2 封包接收機制，防禦髒資料與無限迴圈陷阱。
* **Day 85-86：ARP 協定與快取表 (ARP Protocol & Cache Table)**
    * 實作 ARP 請求廣播：網路世界的翻譯蒟蒻。
    * 建立 Kernel 網路層的電話簿 (ARP Table)，動態記錄 IP 與 MAC 的對應關係。
* **Day 87-88：IPv4、ICMP 協定與 Ping 驗收 (IPv4, ICMP & Ping Verification)**
    * 封裝 IPv4 與 ICMP 標頭、實作網路 Checksum 演算法。
    * 處理「首包掉落定律」，實作 ARP Reply 回應機制，並成功收到虛擬路由器的 Ping 回應。
* **Day 89：系統呼叫與 User Space Ping (System Calls & User Space Ping)**
    * 開通網路系統呼叫 (Syscall)，將底層網路邏輯與應用層分離。
    * 開發 User Space 的 `ping.elf`，讓 Ring 3 應用程式能安全地觸發網路請求，完成跨界打擊！

---

###  Phase 10：Conquering TCP/UDP & Web Exploration

**目標：實作 TCP/UDP，解析 DNS，最終驗收 `wget`。(約 8-9 天)**
有了 IP 之後，我們要實作網路層之上的傳輸層，挑戰最困難的 TCP 狀態機，讓 OS 能夠與全世界的 Web Server 溝通。

* **Day 90-92：UDP 協定與 Socket 接收器 (UDP Protocol & Socket Receiver)**
    * 實作 UDP 無狀態傳輸，建立 Kernel 級別的 Port 綁定 (Bind) 信箱機制。
    * 打通 QEMU Port Forwarding，開發 `udpsend.elf` 與 `udprecv.elf`，實現與 macOS 宿主機 (Host) 跨次元壁的雙向文字通訊。
* **Day 93-94：TCP 狀態機與三方交握 (TCP State Machine & 3-Way Handshake) **
    * 實作複雜的 TCP 偽標頭 (Pseudo Header) 計算與 32-bit 大小端序 (Endianness) 轉換。
    * 精準計算 Sequence/Acknowledge Number，完美攔截 `[SYN, ACK]` 並回傳 `[ACK]`，完成連線建立 (ESTABLISHED)。
* **Day 95：TCP 資料傳輸與優雅中斷 (TCP Data Transfer & Connection Teardown)**
    * 實作帶有 `PSH` 旗標的真實資料傳送機制。
    * 實作帶有 `FIN` 旗標的四方揮手 (4-Way Handshake)，安全釋放雙方連線資源。
* **Day 96：DNS 網域解析與 Socket API (DNS Resolution & Socket API)**
    * 封裝更完整的 Socket 系統呼叫介面 (如 `sys_connect`, `sys_send`, `sys_recv`)。
    * 開發 DNS Client，透過 UDP 向 `8.8.8.8` 查詢，將人類可讀的 `google.com` 解析成真實 IP。
* **Day 97-98：HTTP 協定解析與 `wget` 驗收 (HTTP Protocol & `wget` Verification)**
    * 網路篇的最終大魔王驗收！在 User Space 組裝 HTTP GET 請求，連上真實的 Web Server 下載 HTML 原始碼。
    * 結合 SimpleFS，將網頁內容直接寫入虛擬硬碟中，完成網路與檔案系統的終極整合！


---

## 小結

第二階段除了網路，還有其他主題：

1. 透過 GRUB 實作多重開機 (Termianl or Desktop)
2. Process Management: 實作 top, ps, kill 等 process management
3. 多工視窗環境，包含視窗縮放、task manager、clock、井字遊戲 ... 等

這些有時間再來慢慢整理。

這些東西都是已經非常成熟，無時不刻都在用的東西，是精華中的精華，人類智慧結晶中的結晶。過程中難免會讚嘆以前的設計可以用到現在，雖然有時候也會覺得有點搞剛 XD

自幹這東西，像毒藥一樣，一個做完會想再做下一個看看，然後題目就冒出很多，後面還有一堆可以做，我就慢慢玩了 XDD



---
# 延伸閱讀

## 站內文章

- [自幹作業系統 - SimpleOS][ct1]
- [DNS (Domain Name Service)][ct2]
- [IPv6 基本概念][ct3]

[ct1]: https://rickhw.github.io/2026/03/27/ComputerScience/Simple-OS/
[ct2]: https://rickhw.github.io/2016/03/14/ComputerScience/DNS/
[ct3]: https://rickhw.github.io/2018/11/29/ComputerScience/IPv6/
