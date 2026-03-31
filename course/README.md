## 課程設計核心原則

* **從零開始 (From Scratch)：** 不依賴標準函式庫 (No standard library)，我們會自己寫 `printf`、`malloc` 等。
* **迭代開發：** 每一階段都要能編譯並在 QEMU 上運行，看到實質的回饋。
* **工具鏈：** 使用 `gcc-cross-compiler`、`nasm`、`ld` (Linker) 與 `make`。

---

# Simple OS 60 天實作課程大綱

本專案將 60 天的開發旅程劃分為六個衝刺階段（Sprints）。

## Phase 1: System Boot & Kernel Infrastructure 

**第一階段**：啟動與核心骨架。
**目標：** 從 BIOS 接管控制權，建立中斷與基礎輸出能力。

* **Day 1-4:** GRUB Multiboot 啟動機制、VGA 文字模式與 `kprint` 基礎輸出實作。
* **Day 5-8:** GDT (全域描述符表)、IDT (中斷描述符表)、ISR 中斷跳板與 PIC 控制器。
* **Day 9-10:** 鍵盤驅動與 Timer (PIT) 基礎中斷處理，完成硬體事件攔截。

## Phase 2: Memory Management & Privilege Isolation

**第二階段**：記憶體與特權階級大挪移。
**目標：** 建立現代記憶體管理機制，並成功在 Ring 3 執行外部應用程式。

* **Day 11-12:** 任務控制區塊 (TCB) 與基礎 Context Switch (上下文切換)。
* **Day 13-15:** 實體記憶體管理 (PMM)、虛擬記憶體分頁 (Paging)、核心堆積分配器 (`kmalloc`)。
* **Day 16-17:** 系統呼叫 (Syscall, `int 0x80`) 與 TSS 設定，防護降級至 User Mode (Ring 3)。
* **Day 18-20:** ELF 執行檔解析器、GRUB Multiboot 模組接收與虛實記憶體映射。

## Phase 3: Storage, File System & Interactive Shell 

**第三階段**：儲存裝置、檔案生態與互動 Shell。
**目標：** 脫離 GRUB 保母，讓系統具備讀取實體硬碟、動態載入應用程式與雙向互動的能力。

* **Day 21-23:** ATA/IDE 硬碟驅動實作 (PIO 模式) 與 MBR 分區表解析。
* **Day 24-27:** VFS (虛擬檔案系統) 路由層設計與 SimpleFS 檔案系統實作 (支援跨磁區讀寫)。
* **Day 28-30:** 檔案描述符 (FD)、User Stack 分配、動態 ELF 載入器，以及結合鍵盤緩衝區的互動式 Simple Shell。

## Phase 4: Preemptive Multitasking & Process Management 

**第四階段**：搶佔式多工與行程管理。
**目標：** 讓系統同時執行多個應用程式，完善行程生命週期管理與記憶體保護。

* **Day 31-33:** 搶佔式排程器 (Preemptive Scheduler) 實作，利用 Timer 中斷強行切換多個 Ring 3 行程，解決無窮迴圈死鎖問題。
* **Day 34-37:** 實作 UNIX 經典系統呼叫：`fork` (複製行程)、`exec` (替換執行檔)、`exit` 與 `wait`，並讓 Shell 具備解析與動態載入外部 ELF 工具的能力。
* **Day 38-40:** 實作 MMU 記憶體隔離 (Memory Isolation) 與跨宇宙的 CR3 切換；建立核心同步機制 (Mutex) 與行程間通訊 (IPC Message Queue)，徹底解決資料競爭與記憶體奪舍問題。

## Phase 5: User Space Ecosystem & Advanced File System 

**第五階段**：User Space 生態擴張與寫入能力。
**目標：** 打造平民專用的標準 C 函式庫，並讓檔案系統具備建立與寫入能力。

* **Day 41-43:** 建立 User Space 專屬的標準 C 函式庫 (迷你 libc)，封裝底層 Syscall。
* **Day 44-46:** Ring 3 的動態記憶體分配器 (`malloc`/`free`) 與 `sbrk` 系統呼叫。
* **Day 47-50:** SimpleFS 升級（支援目錄結構與 `sys_write` 寫入硬碟），實作進階指令如 `ls`, `mkdir`, `echo > file`。

## Phase 6: Graphical User Interface & Window System 

**第六階段**：圖形介面與視窗系統。
**目標：** 脫離純文字模式，進入高解析度的畫布與視窗世界。

* **Day 51-54:** VBE (VESA BIOS Extensions) 啟動與線性幀緩衝 (Framebuffer) 渲染。
* **Day 55-57:** 滑鼠驅動程式 (PS/2) 實作與基礎圖形引擎開發（畫點、線、矩形）。
* **Day 58-60:** 字體解析渲染與簡單的視窗合成器 (Compositor)，完成 GUI 作業系統雛形。


---

## 課程藍圖

![](Course-Overview.png)

---

## 學習環境

1. **調試是最大的挑戰：** 在 OS 開發中，你沒有 `gdb` 可以直接連。建議先學會如何使用 QEMU 的偵錯開關（如 `-d int` 查看中斷）以及使用 GDB 遠端連接 QEMU 核心。
2. **不要過度設計：** 在這 60 天內，目標是「理解概念」而非做出「下一個 Linux」。例如：檔案系統選 FAT16 就好，不要挑戰實作實時日誌系統。
3. **硬體環境：** **MacBook Pro M1**，你需要建立一個 x86_64 的 Docker 容器來進行編譯，或者使用虛擬機運行 Linux，因為 macOS 的 Linker 與 Linux 格式 (ELF) 不同。

---

## 第二階段 課程大綱

### 🚀 Phase 7：系統引導與行程管理大師 (大約 8 天)
**目標：達成 `grub.cfg` 開機選項，並完善多工作業，驗收 `ps`, `top`, `kill`。**

在實作複雜的圖形多工前，我們必須先把作業系統底層的「行程字典 (PCB)」與「生命週期」建立完善。

* **Day 61：GRUB Command Line 解析**
    * 讀取 Multiboot MBI 中的 `cmdline` 字串。
    * 實作 `grub.cfg` 選單：`menuentry "Simple OS (CLI)"` vs `menuentry "Simple OS (GUI)"`。
    * 根據參數決定是否啟動 `init_gfx()`。
* **Day 62：行程 ID (PID) 與 PCB 擴充**
    * 為每個 Task 賦予唯一的 PID。
    * 記錄行程的父子關係 (PPID)、名稱、狀態、記憶體使用量、啟動時間。
* **Day 63：系統呼叫擴充 (System Information)**
    * 實作 `sys_getpid` 與 `sys_get_process_list` (讀取所有行程資訊)。
* **Day 64：實作 `ps` 指令**
    * 在 Shell 中讀取系統 API，印出目前所有運行中的 PID、名稱與狀態。
* **Day 65：實作 `top` 指令**
    * 計算 CPU Tick 與每個行程佔用的時間片，換算出 **CPU 使用率 (%)** 與 **記憶體佔用**。
    * 實作終端機的清空與動態刷新。
* **Day 66-67：訊號機制 (Signals) 與 `kill` 指令**
    * 實作 `sys_kill(pid, signal)`。
    * 在排程器中攔截死亡訊號，進行記憶體 (`Page Directory`, `KHeap`) 與檔案描述符 (FD) 的完美回收。
* **Day 68：孤兒與殭屍行程處理**
    * 完善 `sys_wait`，確保父行程正確接收子行程的死亡狀態。

---

### 🎨 Phase 8：視窗伺服器與真・多工 GUI 生態系 (大約 10 天)
**目標：解耦 GUI 與 Kernel，實作 Shared Memory，驗收 Task Manager 與小遊戲。**

目前的 GUI 還是跟 Kernel 綁得有點緊。我們要把它升級成類似 X11 或 Wayland 的「視窗伺服器 (Window Server)」架構。

* **Day 69：視窗與行程的靈魂綁定**
    * 將 `window_t` 結構加入 `owner_pid`，確保視窗關閉時自動 Kill 行程，或行程死亡時自動關閉視窗。
* **Day 70：共享記憶體 (Shared Memory) 機制**
    * 實作讓 Ring 3 應用程式能跟 GUI 引擎共享一塊「畫布」的記憶體。
    * 讓 App 自己負責畫自己的內容，GUI 只負責「合成 (Composite)」。
* **Day 71：視窗訊息佇列 (Event Routing)**
    * 將滑鼠點擊與鍵盤輸入，精準派送 (Route) 給目前取得焦點 (Focused) 的視窗所屬的 PID。
* **Day 72：GUI Task Manager 驗收**
    * 將 `top` 指令圖形化！畫出 CPU / Memory 的動態長條圖與行程列表，並加入「結束工作」按鈕。
* **Day 73-74：圖形化時鐘與計算機**
    * **Clock：** 結合 RTC，實作指針或電子鐘，驗證背景視窗即時更新。
    * **Calculator：** 實作 GUI 按鈕元件抽象化 (Button UI) 與滑鼠點擊事件處理。
* **Day 75-78：踩地雷 (Minesweeper) 大驗收**
    * 實作滑鼠「左鍵開挖、右鍵插旗」的硬體訊號解析。
    * 2D 陣列邏輯實作。
    * **真多工展示：** 同時開著時鐘、打踩地雷、背景放著 Task Manager 監控資源！

---

### 🌐 Phase 9：網路喚醒：從硬體掃描到 ICMP (大約 10 天)
**目標：啟動網卡驅動，實作基礎網路堆疊，驗收 `ping` 指令。**

網路是作業系統最龐大的子系統之一。我們將由下而上，從實體層一路打通到網路層。


[Image of OSI model layers]


* **Day 79：PCI 匯流排掃描**
    * 掃描主機板上的 PCI 裝置，尋找 QEMU 內建的網路卡 (通常是 RTL8139 或 E1000)。
* **Day 80：網卡驅動程式 (NIC Driver) 初始化**
    * 啟動網卡、設定 MAC 位址、設定 Rx/Tx (接收/發送) Ring Buffer。
* **Day 81：乙太網路層 (Ethernet Layer)**
    * 定義乙太網路標頭 (Ethernet Header)，實作 MAC 廣播與封包收發。
* **Day 82-83：ARP 協定解析**
    * 實作 ARP 請求與回應：網路世界的翻譯蒟蒻 (IP 轉換為 MAC)。
* **Day 84：IPv4 協定堆疊**
    * 封裝與解析 IPv4 標頭、Checksum 計算。
* **Day 85-88：ICMP 協定與 `ping` 指令驗收**
    * 實作 ICMP Echo Request / Reply。
    * 在 Terminal 中執行 `ping 8.8.8.8`，看著封包成功來回，絕對會感動到哭！

---

### 🌍 Phase 10：征服 TCP 協定與 Web 探索 (大約 7 天)
**目標：實作 TCP/UDP，解析 DNS，最終驗收 `wget` 與 `curl`。**

有了 IP 之後，我們要實作最困難的 TCP 狀態機，讓 OS 能夠與全世界的 Web Server 溝通。

* **Day 89-90：UDP 與 DNS 解析**
    * 實作較簡單的 UDP 協定。
    * 透過 UDP 實作 DNS Client，輸入網址 (`google.com`) 自動解析成 IP。
* **Day 91-92：TCP 狀態機與三向交握 (Three-way Handshake)**
    * 這將是網路章節的最終魔王。實作 SYN, SYN-ACK, ACK，以及封包序號 (Sequence Number) 的追蹤。
* **Day 93：Socket API 雛形**
    * 提供 Ring 3 應用程式類似 `sys_socket`, `sys_connect`, `sys_send`, `sys_recv` 的系統呼叫。
* **Day 94：解析 HTTP 協定與 `curl` 驗收**
    * 組裝 HTTP GET 請求。
    * 開發 `curl`，連上網站並將 HTML 原始碼印在 Terminal 上。
* **Day 95：`wget` 驗收與完美謝幕**
    * 結合你的 SimpleFS 檔案系統，將下載下來的 HTML 或純文字檔，直接寫入虛擬硬碟中！
