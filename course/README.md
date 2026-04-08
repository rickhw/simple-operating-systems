## 課程設計核心原則

* **從零開始 (From Scratch)：** 不依賴標準函式庫 (No standard library)，我們會自己寫 `printf`、`malloc` 等。
* **迭代開發：** 每一階段都要能編譯並在 QEMU 上運行，看到實質的回饋。
* **工具鏈：** 使用 `gcc-cross-compiler`、`nasm`、`ld` (Linker) 與 `make`。

---

## 學習環境

1. **調試是最大的挑戰：** 在 OS 開發中，你沒有 `gdb` 可以直接連。建議先學會如何使用 QEMU 的偵錯開關（如 `-d int` 查看中斷）以及使用 GDB 遠端連接 QEMU 核心。
2. **不要過度設計：** 在這 60 天內，目標是「理解概念」而非做出「下一個 Linux」。例如：檔案系統選 FAT16 就好，不要挑戰實作實時日誌系統。
3. **硬體環境：** **MacBook Pro M1**，你需要建立一個 x86_64 的 Docker 容器來進行編譯，或者使用虛擬機運行 Linux，因為 macOS 的 Linker 與 Linux 格式 (ELF) 不同。

---

# Simple OS 60 天實作課程大綱

## 第一階段

本專案將 60 天的開發旅程劃分為六個衝刺階段（Sprints）。

### Phase 1: System Boot & Kernel Infrastructure 

**第一階段**：啟動與核心骨架。
**目標：** 從 BIOS 接管控制權，建立中斷與基礎輸出能力。

* **Day 1-4:** GRUB Multiboot 啟動機制、VGA 文字模式與 `kprint` 基礎輸出實作。
* **Day 5-8:** GDT (全域描述符表)、IDT (中斷描述符表)、ISR 中斷跳板與 PIC 控制器。
* **Day 9-10:** 鍵盤驅動與 Timer (PIT) 基礎中斷處理，完成硬體事件攔截。

### Phase 2: Memory Management & Privilege Isolation

**第二階段**：記憶體與特權階級大挪移。
**目標：** 建立現代記憶體管理機制，並成功在 Ring 3 執行外部應用程式。

* **Day 11-12:** 任務控制區塊 (TCB) 與基礎 Context Switch (上下文切換)。
* **Day 13-15:** 實體記憶體管理 (PMM)、虛擬記憶體分頁 (Paging)、核心堆積分配器 (`kmalloc`)。
* **Day 16-17:** 系統呼叫 (Syscall, `int 0x80`) 與 TSS 設定，防護降級至 User Mode (Ring 3)。
* **Day 18-20:** ELF 執行檔解析器、GRUB Multiboot 模組接收與虛實記憶體映射。

### Phase 3: Storage, File System & Interactive Shell 

**第三階段**：儲存裝置、檔案生態與互動 Shell。
**目標：** 脫離 GRUB 保母，讓系統具備讀取實體硬碟、動態載入應用程式與雙向互動的能力。

* **Day 21-23:** ATA/IDE 硬碟驅動實作 (PIO 模式) 與 MBR 分區表解析。
* **Day 24-27:** VFS (虛擬檔案系統) 路由層設計與 SimpleFS 檔案系統實作 (支援跨磁區讀寫)。
* **Day 28-30:** 檔案描述符 (FD)、User Stack 分配、動態 ELF 載入器，以及結合鍵盤緩衝區的互動式 Simple Shell。

### Phase 4: Preemptive Multitasking & Process Management 

**第四階段**：搶佔式多工與行程管理。
**目標：** 讓系統同時執行多個應用程式，完善行程生命週期管理與記憶體保護。

* **Day 31-33:** 搶佔式排程器 (Preemptive Scheduler) 實作，利用 Timer 中斷強行切換多個 Ring 3 行程，解決無窮迴圈死鎖問題。
* **Day 34-37:** 實作 UNIX 經典系統呼叫：`fork` (複製行程)、`exec` (替換執行檔)、`exit` 與 `wait`，並讓 Shell 具備解析與動態載入外部 ELF 工具的能力。
* **Day 38-40:** 實作 MMU 記憶體隔離 (Memory Isolation) 與跨宇宙的 CR3 切換；建立核心同步機制 (Mutex) 與行程間通訊 (IPC Message Queue)，徹底解決資料競爭與記憶體奪舍問題。

### Phase 5: User Space Ecosystem & Advanced File System 

**第五階段**：User Space 生態擴張與寫入能力。
**目標：** 打造平民專用的標準 C 函式庫，並讓檔案系統具備建立與寫入能力。

* **Day 41-43:** 建立 User Space 專屬的標準 C 函式庫 (迷你 libc)，封裝底層 Syscall。
* **Day 44-46:** Ring 3 的動態記憶體分配器 (`malloc`/`free`) 與 `sbrk` 系統呼叫。
* **Day 47-50:** SimpleFS 升級（支援目錄結構與 `sys_write` 寫入硬碟），實作進階指令如 `ls`, `mkdir`, `echo > file`。

### Phase 6: Graphical User Interface & Window System 

**第六階段**：圖形介面與視窗系統。
**目標：** 脫離純文字模式，進入高解析度的畫布與視窗世界。

* **Day 51-54:** VBE (VESA BIOS Extensions) 啟動與線性幀緩衝 (Framebuffer) 渲染。
* **Day 55-57:** 滑鼠驅動程式 (PS/2) 實作與基礎圖形引擎開發（畫點、線、矩形）。
* **Day 58-60:** 字體解析渲染與簡單的視窗合成器 (Compositor)，完成 GUI 作業系統雛形。


---


## 🏆 第二階段 課程大綱

### 🚀 Phase 7：系統引導與行程管理大師 [✅ 已達成]
**成果：完成動態模式切換，建立完善的生命週期與垃圾回收機制。**
* **Day 61-63：** 實作 `grub.cfg` 解析，成功切換 CLI/GUI 模式，並建立包含 PID, PPID 等完整屬性的 PCB (行程控制區塊)。
* **Day 64-65：** 完成 `ps` 與 `top` 指令，成功追蹤 CPU 狀態與動態記憶體 (Heap) 使用量。
* **Day 66-68：** 實作 `sys_kill` 訊號機制、`sys_wait` 收屍機制，並完美解決了孤兒與殭屍行程的記憶體回收問題。

### 🎨 Phase 8：視窗伺服器與真・多工 GUI 生態系 [✅ 超越預期達成]
**成果：打造出具備事件防穿透、拖曳控制，且擁有獨立 SimpleUI 框架的現代桌面環境！**
* **Day 69-70：** 實作 Shared Memory (獨立畫布)，並加入 Page Fault Handler 完美保護 Kernel (Segmentation Fault 防護)。
* **Day 71-72：** 建立精準的 Z-Order 事件路由 (Event Routing) 與點擊穿透防護，實作無縫的視窗拖曳功能。
* **Day 73-74：** 實作 File I/O 與 RTC 系統呼叫，成功打造 `viewer` (BMP 圖片解碼器) 與 `clock` (電子鐘)。
* **Day 75-77：** 實作 GUI IPC 機制，完成 `explorer` (檔案總管支援點擊開啟檔案)、以及 `notepad` (鍵盤事件分發器與打字系統)。
* **Day 78-80：*

### 🌐 Phase 9：網路喚醒：從硬體掃描到 ICMP (Network Awakening: From Hardware Scan to ICMP)
**目標：啟動網卡驅動，實作基礎網路堆疊，驗收 `ping` 指令。(約 9 天)**
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

### 🌍 Phase 10：征服 TCP/UDP 協定與 Web 探索 (Conquering TCP/UDP & Web Exploration)
**目標：實作 TCP/UDP，解析 DNS，最終驗收 `wget`。(約 8-9 天)**
有了 IP 之後，我們要實作網路層之上的傳輸層，挑戰最困難的 TCP 狀態機，讓 OS 能夠與全世界的 Web Server 溝通。

* **Day 90-92：UDP 協定與 Socket 接收器 (UDP Protocol & Socket Receiver)**
    * 實作 UDP 無狀態傳輸，建立 Kernel 級別的 Port 綁定 (Bind) 信箱機制。
    * 打通 QEMU Port Forwarding，開發 `udpsend.elf` 與 `udprecv.elf`，實現與 macOS 宿主機 (Host) 跨次元壁的雙向文字通訊。
* **Day 93-94：TCP 狀態機與三方交握 (TCP State Machine & 3-Way Handshake) [✅ 我們在這裡！]**
    * 實作複雜的 TCP 偽標頭 (Pseudo Header) 計算與 32-bit 大小端序 (Endianness) 轉換。
    * 精準計算 Sequence/Acknowledge Number，完美攔截 `[SYN, ACK]` 並回傳 `[ACK]`，完成連線建立 (ESTABLISHED)。
* **Day 95：TCP 資料傳輸與優雅中斷 (TCP Data Transfer & Connection Teardown) [🔥 即將展開]**
    * 實作帶有 `PSH` 旗標的真實資料傳送機制。
    * 實作帶有 `FIN` 旗標的四方揮手 (4-Way Handshake)，安全釋放雙方連線資源。
* **Day 96：DNS 網域解析與 Socket API (DNS Resolution & Socket API)**
    * 封裝更完整的 Socket 系統呼叫介面 (如 `sys_connect`, `sys_send`, `sys_recv`)。
    * 開發 DNS Client，透過 UDP 向 `8.8.8.8` 查詢，將人類可讀的 `google.com` 解析成真實 IP。
* **Day 97-98：HTTP 協定解析與 `wget` 驗收 (HTTP Protocol & `wget` Verification)**
    * 網路篇的最終大魔王驗收！在 User Space 組裝 HTTP GET 請求，連上真實的 Web Server 下載 HTML 原始碼。
    * 結合 SimpleFS，將網頁內容直接寫入虛擬硬碟中，完成網路與檔案系統的終極整合！


---

## Network Overview

![](/about/NetworkOverview.png)

### 👈 左半部：Simple OS 網路分層架構圖 (Conceptual Architecture)

這部分採用上下分層（Top-Down）的概念，表達資料從 User Space（Ring 3）一路下放到實體硬體（Ring 0）的過程。

* **1. User Application Layer (User Space)：** 這是你的應用程式，像是 `udpsend.elf` 或 `ping.elf`。它們只需要知道你要傳送的文字和目標 IP/Port，完全不需要管底層細節。
* **2. Syscall Interface (The Boundary)：** 這是 Ring 3 和 Ring 0 的分界線。User App 透過呼叫特殊的指令（圖中標記為 `int 128 (0x80)`）來觸發 Kernel 接手。這裡標記了我們的 `syscall.c` 和 `syscall_lib.c` 處理的部分。
* **3. Transport Layer (Kernel Space)：** 這是核心的協定棧。這裡負責處理 UDP 標頭（例如 Port）或 ICMP 標頭（例如 Ping 類型），並計算 Checksum。標記了 `udp.c` 和 `icmp.c`。
* **4. Network Layer (Kernel Space)：** 這裡負責 L3 的邏輯。如果是 ARP 請求，會在這裡封裝 ARP 標頭（標記 `arp.c`），並在 ARP 表中查詢 MAC 位址。如果是正常的 IP 資料，會在這裡封裝 IPv4 標頭（標記 `ipv4.h`）並負責路由判斷。
* **5. Data Link Layer (Driver)：** 這裡負責把 L3 的封包貼上乙太網路標頭（標記 `ethernet.h`），並把完整的資料拷貝到網卡 DMA 緩衝區（Rx Ring Buffer）。標記了 `rtl8139.c` 的發送槽管理邏輯。
* **6. PCI & Hardware Layer：** 最底層的硬體。這裡負責 PCI 的掃描（標記 `pci.c`）來揪出網卡，以及網卡硬體本身（如 Tx/Rx 埠）把資料變成真正的 bit 噴射出去。

---

### 👉 右半部：網路封包概念圖 (Network Packet Conceptual Diagram)

這部分採用**俄羅斯娃娃（嵌套）**的概念，表達一個封包在不同層級是如何被一層一層包裝起來的。

* **Ethernet Frame (Blue)：** 這是最外層的信封（L2），所有封包都必須穿上它。它包含了目標 MAC（`Dest MAC`）和來源 MAC（`Src MAC`）。關鍵欄位是 `EtherType`，如果填 `0x0806`，裡面包的就是 ARP；如果填 `0x0800`，裡面包的就是 IPv4。
    * **分支：ARP Packet (Green)：** 當 EtherType 為 `0x0806` 時。這顆封包用來問「誰的 IP 是 10.0.2.2？」。它包含了 ARP 的專屬欄位，像是 Opcode（1 Request/2 Reply）、來源/目標 MAC/IP。
* **IPv4 Header (Green)：** 當 EtherType 為 `0x0800` 時（L3）。這是 IP 信封，它包含了來源 IP (`10.0.2.15`) 和目標 IP (`10.0.2.2`)。關鍵欄位是 `Protocol`，如果是 `1`，裡面包的就是 ICMP；如果是 `17`，裡面包的就是 UDP。
    * **分支：ICMP Packet (Yellow)：** 當 Protocol 為 `1` 時（Ping）。它包含了 Ping 的專屬欄位，像是 Type（8 Request/0 Reply）、我特別標記了你用來 Debug 的 `Ident 0x1337`，以及垃圾資料 Payload ("ABCD...")。
    * **分支：UDP Packet (Yellow)：** 當 Protocol 為 `17` 時（資料）。這就是你昨天成功的 UDP 封包。它包含了來源 Port (`12345`)、目標 Port (`8080`)，Checksum 欄位我標記為 `0`（不檢查）。最後就是你最感動的那個文字 Payload（**"Hello QEMU"**）。
