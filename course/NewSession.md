你是一個計算機科學系教授，教受作業系統實作課程！我正在開發一個 32-bit 的 x86 作業系統 (Simple OS)。在另一個 Session (60 天作業系統實作課程大綱) 已經完成了：實體記憶體管理、分頁、中斷處理、VFS (虛擬檔案系統)、SimpleFS、基礎的 Ring 3 User Space、Syscall 分派器，以及完整的 TCP/IP 網路堆疊 (RTL8139)。底下是之前完成的課程大綱：

---
## 第一階段

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

### 🚀 Phase 7：系統引導與行程管理大師 
**成果：完成動態模式切換，建立完善的生命週期與垃圾回收機制。**
* **Day 61-63：** 實作 `grub.cfg` 解析，成功切換 CLI/GUI 模式，並建立包含 PID, PPID 等完整屬性的 PCB (行程控制區塊)。
* **Day 64-65：** 完成 `ps` 與 `top` 指令，成功追蹤 CPU 狀態與動態記憶體 (Heap) 使用量。
* **Day 66-68：** 實作 `sys_kill` 訊號機制、`sys_wait` 收屍機制，並完美解決了孤兒與殭屍行程的記憶體回收問題。

### 🎨 Phase 8：視窗伺服器與真・多工 GUI 生態系
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

接下來的開發階段，我想要聚焦在：

1. 時間相關：現在 code 裡 timer.c 有個 tick 概念，我一直不是很懂這東西。我想透過寫一個 ntp client & date 來了解時間再計算機裡的運作.
2. 登入：unix 系統都是可以多人登入，然後有自己的 home，以及 process，我想實作這一部分。
3. 排程演算法：在作業系統概念 (恐龍書)，提到 process 排程，有討論很多演算法，我也想實作看看。
4. I/O 概念：實際應用中，會用到比較進階的 I/O 機制，像是 epoll、io_uring 等機制，我也想了解這方面的實作。

底下是完整的 source code 結構，以及目前核心的 *.h ... 等定義檔。

```bash
.
├── Dockerfile
├── Makefile
├── README.md
├── scripts
│   ├── grub.cfg
│   └── linker.ld
├── src
│   ├── kernel
│   │   ├── arch
│   │   │   └── x86
│   │   ├── drivers
│   │   │   ├── block
│   │   │   ├── input
│   │   │   ├── net
│   │   │   ├── pci
│   │   │   ├── rtc
│   │   │   └── video
│   │   ├── fs
│   │   │   ├── elf.c
│   │   │   ├── simplefs.c
│   │   │   └── vfs.c
│   │   ├── include
│   │   │   ├── arp.h
│   │   │   ├── ata.h
│   │   │   ├── config.h
│   │   │   ├── elf.h
│   │   │   ├── ethernet.h
│   │   │   ├── font8x8.h
│   │   │   ├── gdt.h
│   │   │   ├── gfx.h
│   │   │   ├── gui.h
│   │   │   ├── icmp.h
│   │   │   ├── idt.h
│   │   │   ├── io.h
│   │   │   ├── ipv4.h
│   │   │   ├── keyboard.h
│   │   │   ├── kheap.h
│   │   │   ├── mbr.h
│   │   │   ├── mouse.h
│   │   │   ├── multiboot.h
│   │   │   ├── paging.h
│   │   │   ├── pci.h
│   │   │   ├── pmm.h
│   │   │   ├── rtc.h
│   │   │   ├── rtl8139.h
│   │   │   ├── simplefs.h
│   │   │   ├── syscall.h
│   │   │   ├── task.h
│   │   │   ├── tcp.h
│   │   │   ├── timer.h
│   │   │   ├── tty.h
│   │   │   ├── udp.h
│   │   │   ├── utils.h
│   │   │   └── vfs.h
│   │   ├── init
│   │   │   └── main.c
│   │   ├── kernel
│   │   │   ├── gui.c
│   │   │   ├── syscall.c
│   │   │   └── task.c
│   │   ├── lib
│   │   │   └── utils.c
│   │   ├── mm
│   │   │   ├── kheap.c
│   │   │   └── pmm.c
│   │   └── net
│   │       ├── arp.c
│   │       ├── icmp.c
│   │       ├── tcp.c
│   │       └── udp.c
│   └── user
│       ├── asm
│       │   └── crt0.S
│       ├── bin
│       │   ├── cat.c
│       │   ├── clock.c
│       │   ├── echo.c
│       │   ├── explorer.c
│       │   ├── free.c
│       │   ├── host.c
│       │   ├── kill.c
│       │   ├── logo.bmp
│       │   ├── ls.c
│       │   ├── mkdir.c
│       │   ├── notepad.c
│       │   ├── paint.c
│       │   ├── ping.c
│       │   ├── ping2.c
│       │   ├── pong.c
│       │   ├── ps.c
│       │   ├── rm.c
│       │   ├── segfault.c
│       │   ├── shell.c
│       │   ├── status.c
│       │   ├── taskmgr.c
│       │   ├── tcptest.c
│       │   ├── tictactoe.c
│       │   ├── top.c
│       │   ├── touch.c
│       │   ├── udprecv.c
│       │   ├── udpsend.c
│       │   ├── viewer.c
│       │   └── wget.c
│       ├── include
│       │   ├── dns.h
│       │   ├── net.h
│       │   ├── simpleui.h
│       │   ├── stdarg.h
│       │   ├── stdio.h
│       │   ├── stdlib.h
│       │   ├── string.h
│       │   ├── syscall.h
│       │   └── unistd.h
│       └── lib
│           ├── net.c
│           ├── simpleui.c
│           ├── stdio.c
│           ├── stdlib.c
│           ├── string.c
│           ├── syscall.c
│           └── unistd.c

```

---
src/kernel/include/tcp.h
```h
#ifndef TCP_H
#define TCP_H

#include <stdint.h>

// 1. 真實的 TCP 標頭
typedef struct {
    uint16_t src_port;     // 來源 Port
    uint16_t dest_port;    // 目標 Port
    uint32_t seq_num;      // 序號 (Sequence Number)
    uint32_t ack_num;      // 確認號 (Acknowledge Number)
    uint8_t  data_offset;  // 標頭長度 (高 4 bits) + 保留位元
    uint8_t  flags;        // 控制旗標 (SYN, ACK, FIN...)
    uint16_t window_size;  // 接收視窗大小
    uint16_t checksum;     // 檢查碼 (絕對不能為 0)
    uint16_t urgent_ptr;   // 緊急指標
} __attribute__((packed)) tcp_header_t;

// 2. 用來計算 Checksum 的「偽標頭」
typedef struct {
    uint8_t  src_ip[4];
    uint8_t  dest_ip[4];
    uint8_t  reserved;     // 永遠填 0
    uint8_t  protocol;     // 永遠填 6 (TCP)
    uint16_t tcp_length;   // TCP 標頭 + 資料的總長度
} __attribute__((packed)) tcp_pseudo_header_t;

// TCP 旗標定義 (Bitmask)
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port);
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num);

int tcp_recv_data(char* buffer);

#endif
```
---
src/kernel/include/utils.h
```h
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// === Memory Utils ===
void* memcpy(void* dstptr, const void* srcptr, size_t size);
void* memset(void* bufptr, int value, size_t size);
// 比較兩塊記憶體前 n 個 bytes 是否相同
int memcmp(const void *s1, const void *s2, size_t n);

// === String Utils ===
void reverse_string(char* str, int length);
void reverse_string(char* str, int length);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
int strlen(const char* str);
// 尋找子字串 (如果找到 needle，回傳它在 haystack 中的指標，否則回傳 0)
char* strstr(const char* haystack, const char* needle);
// 安全的字串拷貝 (最多拷貝 n 個字元，並保證 null 結尾)
char *strncpy(char *dest, const char *src, size_t n);

// 核心工具：整數轉字串 (itoa), value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base);

uint16_t calculate_checksum(void* data, int count);

// === IO Utils ===
void kprintf(const char* format, ...);

#endif
```
---
src/kernel/include/task.h
```h
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// ==========================================
// 記憶體分佈與權限常數定義 (Memory Layout & Privileges)
// ==========================================
#define PAGE_SIZE           4096
#define USER_HEAP_START     0x10000000
#define USER_HEAP_PAGES     256
#define USER_STACK_TOP      0x083FF000
#define USER_STACK_PADDING  64
#define USER_ARGS_LIMIT     0x08000000
#define MAX_EXEC_ARGS       16

// 段選擇子 (Segment Selectors)
#define USER_DS             0x23
#define USER_CS             0x1B
#define KERNEL_DS           0x10
#define KERNEL_CS           0x08
#define EFLAGS_DEFAULT      0x0202

#define KERNEL_IDLE_PID     9999

/** 行程狀態定義 */
#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2
#define TASK_ZOMBIE  3  ///< 行程已結束，但父行程尚未 wait() 回收它
#define TASK_SLEEPING 4 ///< 睡眠中 (未來擴充使用)

/**
 * @brief 中斷上下文快照 (Interrupt Context Snapshot)
 * * 此結構體的佈局必須嚴格遵循 x86 中斷發生時的堆疊增長順序。
 * 記憶體佈局 (由低位址往高位址)：
 * [低] edi -> esi -> ebp -> esp -> ebx -> edx -> ecx -> eax -> eip -> cs -> eflags -> user_esp -> user_ss [高]
 */
typedef struct {
    /* 由 pusha 指令壓入的通用暫存器 (順序由後往前) */
    uint32_t edi;    ///< Destination Index: 常用於記憶體拷貝目的地
    uint32_t esi;    ///< Source Index: 常用於記憶體拷貝來源
    uint32_t ebp;    ///< Base Pointer: 堆疊基底指標，用於 Stack Trace
    uint32_t esp;    ///< Stack Pointer: 核心模式下的堆疊頂端 (pusha 壓入的值)
    uint32_t ebx;    ///< Base Register: 常用於存取資料段位址
    uint32_t edx;    ///< Data Register: 常用於 I/O 埠操作或乘除法
    uint32_t ecx;    ///< Counter Register: 常用於迴圈計數
    uint32_t eax;    ///< Accumulator Register: 函式回傳值或 Syscall 編號

    /* 由 CPU 在某些例外發生時自動壓入的錯誤碼 (一般中斷手動補 0) */
    uint32_t err_code;

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    ///< Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     ///< Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; ///< CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; ///< User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  ///< User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
} registers_t;

/**
 * @brief 任務控制區塊 (Process Control Block, PCB)
 */
typedef struct task {
    uint32_t esp;               ///< 核心堆疊指標

    // === 行程身分資訊 ===
    uint32_t pid;               ///< 行程 ID
    uint32_t ppid;              ///< 父行程 ID
    char name[32];              ///< 行程名稱

    uint32_t total_ticks;       ///< 行程總執行時間 (Ticks)

    uint32_t page_directory;    ///< 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;      ///< 核心堆疊頂部

    uint32_t state;             ///< 行程當前狀態
    uint32_t wait_pid;          ///< 正在等待的子行程 PID

    uint32_t heap_start;        ///< Heap 初始起點
    uint32_t heap_end;          ///< User Heap 的當前頂點
    uint32_t cwd_lba;           ///< 當前工作目錄 (CWD) 的 LBA

    struct task *next;          ///< 環狀鏈結串列指標
} task_t;

/**
 * @brief 用於傳遞給 User Space 的行程資訊
 */
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; ///< 佔用記憶體大小 (Bytes)
    uint32_t total_ticks; ///< 總執行時間
} process_info_t;

extern volatile task_t *current_task;

/** @brief 初始化多工排程子系統 */
void init_multitasking();

/**
 * @brief 建立第一個 User Task
 * @param entry_point 程式進入點
 * @param user_stack_top User Stack 頂部
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);

/** @brief 結束當前行程 */
void exit_task();

/** @brief 排程器 (Round Robin) 切換下一個行程 */
void schedule();

/**
 * @brief 複製當前行程 (Fork)
 * @param regs 當前的暫存器狀態
 * @return 子行程回傳 0，父行程回傳子行程的 PID，失敗回傳 -1
 */
int sys_fork(registers_t *regs);

/**
 * @brief 替換當前行程的執行檔影像 (Exec)
 * @param regs 包含新執行檔名稱與參數的暫存器
 * @return 成功不回傳，失敗回傳 -1
 */
int sys_exec(registers_t *regs);

/**
 * @brief 等待子行程結束
 * @param pid 要等待的子行程 PID
 * @return 成功回傳 0，失敗回傳 -1
 */
int sys_wait(uint32_t pid);

/**
 * @brief 強制終止行程
 * @param pid 要終止的行程 PID
 * @return 成功回傳 0，找不到行程回傳 -1
 */
int sys_kill(uint32_t pid);

/**
 * @brief 取得系統中所有活躍的行程清單
 * @param list 用於儲存結果的陣列
 * @param max_count 陣列最大容量
 * @return 實際取回的行程數量
 */
int task_get_process_list(process_info_t* list, int max_count);

#endif
```
---
src/kernel/include/ata.h
```h
// ATA: Advanced Technology Attachment
// LBA: Logical Block Addressing
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// 讀取指定的邏輯區塊 (LBA)，並將 512 bytes 的資料存入 buffer
void ata_read_sector(uint32_t lba, uint8_t* buffer);

// 寫入 512 bytes 的資料到指定的邏輯區塊 (LBA)
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif
```
---
src/kernel/include/rtl8139.h
```h
#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void init_rtl8139(uint8_t bus, uint8_t slot);
void rtl8139_handler(void);

// 
void rtl8139_send_packet(void* data, uint32_t len);
uint8_t* rtl8139_get_mac(void);

#endif
```
---
src/kernel/include/icmp.h
```h
#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include "ipv4.h"

typedef struct {
    uint8_t  type;          // 類型 (Echo Request = 8, Echo Reply = 0)
    uint8_t  code;          // 代碼 (通常為 0)
    uint16_t checksum;      // 檢查碼
    uint16_t identifier;    // 識別碼 (用來對應 Request/Reply)
    uint16_t sequence;      // 序號
} __attribute__((packed)) icmp_header_t;

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

void ping_send_request(uint8_t* target_ip);

#endif
```
---
src/kernel/include/tty.h
```h
#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_bind_window(int win_id);
void terminal_set_mode(int is_gui);

void tty_render_fullscreen(void);
void tty_render_window(int win_id);

#endif
```
---
src/kernel/include/idt.h
```h
// Interrupt Descriptor Table: Interrupt Descriptor Table
// ISR: Interrupt Service Routine
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Interrupt Descriptor Table 單一條目的結構
struct idt_entry_struct {
    uint16_t base_lo; // 中斷處理程式 (ISR) 位址的下半部
    uint16_t sel;     // Global Descriptor Table 裡的 Kernel Code Segment 選擇子 (通常是 0x08)
    uint8_t  always0; // 必須為 0
    uint8_t  flags;   // 屬性與權限標籤
    uint16_t base_hi; // 中斷處理程式 (ISR) 位址的上半部
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// 指向 Interrupt Descriptor Table 陣列的指標結構 (給 lidt 指令用的)
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt(void);
void pic_remap(void);

#endif
```
---
src/kernel/include/config.h
```h
#ifndef CONFIG_H
#define CONFIG_H

// --- Screen Configuration ---
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_DEPTH  32

// --- Memory Configuration ---
#define KERNEL_STACK_SIZE 16384
#define PMM_BITMAP_SIZE   32768
#define PMM_RESERVED_MEM  1024 // 4MB (1024 * 4KB)
#define INITIAL_PMM_SIZE  16384 // 16MB

// --- GUI Configuration ---
#define MAX_WINDOWS      10
#define TASKBAR_HEIGHT   28
#define START_MENU_WIDTH 150
#define START_MENU_HEIGHT 130
#define TITLE_BAR_HEIGHT 18

// --- Colors (Classic Windows-like theme) ---
#define COLOR_DESKTOP     0x008080 // Teal
#define COLOR_WINDOW_BG   0xC0C0C0 // Light Gray
#define COLOR_TITLE_ACTIVE 0x000080 // Dark Blue
#define COLOR_TITLE_INACTIVE 0x808080 // Gray
#define COLOR_WHITE       0xFFFFFF
#define COLOR_BLACK       0x000000
#define COLOR_DARK_GRAY   0x808080

#endif
```
---
src/kernel/include/simplefs.h
```h

#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include "vfs.h"

// SimpleFS 的魔法數字 ("SFS!" 的十六進位)
#define SIMPLEFS_MAGIC 0x21534653

// 定義檔案型態
#define SFS_TYPE_FILE 0
#define SFS_TYPE_DIR  1

// Superblock 結構 (佔用一個 512 bytes 磁區，但只用前面幾個 bytes)
typedef struct {
    uint32_t magic;         // 證明這是一個 SimpleFS 分區
    uint32_t total_blocks;  // 分區總容量 (以 512 bytes 磁區為單位)
    uint32_t root_dir_lba;  // 根目錄所在的相對 LBA
    uint32_t data_start_lba;// 檔案資料區開始的相對 LBA
    uint8_t  padding[496];  // 補齊到 512 bytes
} __attribute__((packed)) sfs_superblock_t;

// 檔案目錄項目結構
typedef struct {
    char     filename[32];  // 檔案名稱 (包含結尾 \0)
    uint32_t start_lba;     // 檔案內容開始的相對 LBA
    uint32_t file_size;     // 檔案大小 (Bytes)
    uint32_t type;         // 【新增】檔案型態：0=檔案, 1=目錄 (4 bytes)
    uint32_t reserved[5];  // 保留空間，湊滿 64 bytes 完美對齊 (20 bytes)
} __attribute__((packed)) sfs_file_entry_t;

// disk utils
extern uint32_t mounted_part_lba;

// 格式化指定的分區
void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);
void simplefs_mount(uint32_t part_lba);
void simplefs_list_files(void);

int simplefs_create_file(uint32_t dir_lba_rel, char* filename, char* data, uint32_t size);
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

// 用相對目錄位址 (dir_lba_rel)
fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* filename);
int simplefs_readdir(uint32_t dir_lba_rel, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* path);

#endif
```
---
src/kernel/include/mbr.h
```h
#ifndef MBR_H
#define MBR_H

#include <stdint.h>

// 單個分區紀錄的結構 (固定 16 bytes)
typedef struct {
    uint8_t  status;       // 0x80 代表這是可開機分區，0x00 代表不可開機
    uint8_t  chs_first[3]; // 舊版的 CHS 起始位址 (現代硬碟已廢棄，忽略)
    uint8_t  type;         // 分區格式 (例如 0x83 是 Linux, 0x0B 是 FAT32)
    uint8_t  chs_last[3];  // 舊版的 CHS 結束位址 (忽略)
    uint32_t lba_start;    // [極度重要] 這個分區真正的起始 LBA 磁區號碼！
    uint32_t sector_count; // 這個分區總共有幾個磁區 (Size = count * 512 bytes)
} __attribute__((packed)) mbr_partition_t;

// 解析 MBR 的函式
uint32_t parse_mbr(void);

#endif
```
---
src/kernel/include/font8x8.h
```h
#ifndef FONT8X8_H
#define FONT8X8_H

#include <stdint.h>

// 標準 8x8 ASCII 點陣字型 (ASCII 32 ' ' 到 126 '~')
// 每個字元由 8 個 byte 組成，每個 byte 代表橫向的一排像素
static const uint8_t font8x8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 Space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 !
    {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // 35 #
    {0x18,0x7E,0xC0,0x7E,0x06,0x7E,0x18,0x00}, // 36 $
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, // 37 %
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 &
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 (
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 *
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ,
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 .
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // 47 /
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00}, // 48 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 1
    {0x7C,0xC6,0x06,0x3C,0x60,0xC0,0xFE,0x00}, // 50 2
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, // 51 3
    {0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00}, // 52 4
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, // 53 5
    {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, // 54 6
    {0xFE,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 7
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, // 56 8
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, // 57 9
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 :
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 ;
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 <
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 =
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 >
    {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00}, // 63 ?
    {0x7C,0xC6,0xCE,0xDE,0xD6,0xC0,0x7C,0x00}, // 64 @
    {0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // 65 A
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, // 66 B
    {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, // 67 C
    {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, // 68 D
    {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, // 69 E
    {0xFE,0x62,0x68,0x78,0x60,0x60,0xF0,0x00}, // 70 F
    {0x3C,0x66,0xC0,0xCE,0xC6,0x66,0x3E,0x00}, // 71 G
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // 72 H
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // 73 I
    {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, // 74 J
    {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00}, // 75 K
    {0xF0,0x60,0x60,0x60,0x60,0x62,0xFE,0x00}, // 76 L
    {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00}, // 77 M
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, // 78 N
    {0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 79 O
    {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, // 80 P
    {0x38,0x6C,0xC6,0xC6,0xDA,0x6C,0x36,0x00}, // 81 Q
    {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, // 82 R
    {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7E,0x00}, // 83 S
    {0x7E,0x7E,0x18,0x18,0x18,0x18,0x3C,0x00}, // 84 T
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // 85 U
    {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 86 V
    {0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0x6C,0x00}, // 87 W
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, // 88 X
    {0xC6,0xC6,0xC6,0x7C,0x18,0x18,0x3C,0x00}, // 89 Y
    {0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00}, // 90 Z
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 [
    {0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x00}, // 92 backslash
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ]
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00}, // 95 _
    {0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x3C,0x06,0x3E,0xC6,0x7E,0x00}, // 97 a
    {0xE0,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 b
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 c
    {0x0E,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 d
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 e
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x78,0x00}, // 102 f
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C}, // 103 g
    {0xE0,0x60,0x7C,0x66,0x66,0x66,0xE6,0x00}, // 104 h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 i
    {0x06,0x00,0x06,0x06,0x06,0x06,0x06,0x3C}, // 106 j
    {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00}, // 107 k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 l
    {0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0x00}, // 109 m
    {0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00}, // 110 n
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 o
    {0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0}, // 112 p
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x0F}, // 113 q
    {0x00,0x00,0xDC,0x66,0x60,0x60,0xF0,0x00}, // 114 r
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 s
    {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00}, // 116 t
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 u
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 v
    {0x00,0x00,0xC6,0xD6,0xD6,0xFE,0x6C,0x00}, // 119 w
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 x
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C}, // 121 y
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 z
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 |
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 }
    {0x3B,0x6E,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 ~
};

#endif
```
---
src/kernel/include/multiboot.h
```h
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// GRUB 載入完畢後，eax 暫存器一定會存放這個魔法數字，用來證明它是合法的 Bootloader
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// 模組 (Module) 的結構
typedef struct {
    uint32_t mod_start;  // 這個檔案被 GRUB 載入到的起始實體位址
    uint32_t mod_end;    // 這個檔案的結束位址
    uint32_t string;     // 模組名稱字串的位址
    uint32_t reserved;
} multiboot_module_t;

// Multiboot 資訊結構 (MBI)
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    // ==========================================
    // Multiboot Framebuffer 資訊
    // ==========================================
    uint64_t framebuffer_addr;   // 畫布的實體記憶體起點
    uint32_t framebuffer_pitch;  // 每一橫列的位元組數 (Bytes per line)
    uint32_t framebuffer_width;  // 畫布寬度 (Pixels)
    uint32_t framebuffer_height; // 畫布高度 (Pixels)
    uint8_t  framebuffer_bpp;    // 色彩深度 (Bits per pixel，例如 32)
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

#endif
```
---
src/kernel/include/elf.h
```h
// ELF: Executable and Linkable Format
#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stdbool.h>

// ELF 檔案開頭永遠是這四個字元： 0x7F, 'E', 'L', 'F'
// 在 Little-Endian 系統中，這組合成一個 32-bit 的整數如下：
#define ELF_MAGIC 0x464C457F

// 32 位元 ELF 標頭結構
typedef struct {
    uint32_t magic;      // 魔法數字 (0x7F 'E' 'L' 'F')
    uint8_t  ident[12];  // 架構資訊 (32/64 bit, Endianness 等)
    uint16_t type;       // 檔案類型 (1=Relocatable, 2=Executable)
    uint16_t machine;    // CPU 架構 (3=x86)
    uint32_t version;    // ELF 版本
    uint32_t entry;      // [極度重要] 程式的進入點 (Entry Point) 虛擬位址！
    uint32_t phoff;      // Program Header Table 的偏移量
    uint32_t shoff;      // Section Header Table 的偏移量
    uint32_t flags;      // 架構特定標籤
    uint16_t ehsize;     // 這個 ELF Header 本身的大小
    uint16_t phentsize;  // 每個 Program Header 的大小
    uint16_t phnum;      // Program Header 的數量
    uint16_t shentsize;  // 每個 Section Header 的大小
    uint16_t shnum;      // Section Header 的數量
    uint16_t shstrndx;   // 字串表索引
} __attribute__((packed)) elf32_ehdr_t;

typedef struct {
    uint32_t type;   // 區段類型 (1 代表需要載入記憶體的 PT_LOAD)
    uint32_t offset; // 該區段在檔案中的偏移量
    uint32_t vaddr;  // [關鍵] 應用程式期望的虛擬位址！
    uint32_t paddr;  // 實體位址 (通常忽略)
    uint32_t filesz; // 區段在檔案中的大小
    uint32_t memsz;  // 區段在記憶體中的大小
    uint32_t flags;  // 讀/寫/執行權限
    uint32_t align;  // 對齊要求
} __attribute__((packed)) elf32_phdr_t;

// 宣告我們的解析函式
bool elf_check_supported(elf32_ehdr_t* header);

// 新增 Loader 宣告
uint32_t elf_load(elf32_ehdr_t* header);

#endif
```
---
src/kernel/include/ipv4.h
```h
#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include "ethernet.h"

typedef struct {
    uint8_t  ihl : 4;       // 標頭長度 (Internet Header Length)
    uint8_t  version : 4;   // 版本 (IPv4 = 4)
    uint8_t  tos;           // 服務類型 (Type of Service)
    uint16_t total_length;  // 總長度 (Header + Data)
    uint16_t ident;         // 識別碼
    uint16_t flags_frag;    // 標誌與片段偏移
    uint8_t  ttl;           // 存活時間 (Time to Live)
    uint8_t  protocol;      // 協定 (ICMP = 1, TCP = 6, UDP = 17)
    uint16_t checksum;      // 標頭檢查碼
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) ipv4_header_t;

#define IPV4_PROTO_ICMP 1
#define IPV4_PROTO_TCP  6
#define IPV4_PROTO_UDP  17

#endif
```
---
src/kernel/include/vfs.h
```h
// Virtual File System: Virtual File System
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

// 定義節點的類型 (Linux "萬物皆檔案" 的基礎)
#define FS_FILE        0x01   // 一般檔案
#define FS_DIRECTORY   0x02   // 目錄
#define FS_CHARDEVICE  0x03   // 字元裝置 (如鍵盤、終端機)
#define FS_BLOCKDEVICE 0x04   // 區塊裝置 (如硬碟)
#define FS_PIPE        0x05   // 管線 (Pipe)
#define FS_MOUNTPOINT  0x08   // 掛載點

// 前置宣告，因為函式指標會用到自己
struct fs_node;

// 定義「讀取」與「寫入」的函式指標型別 (這就是 Interface)
// 參數: 節點指標, 偏移量(offset), 讀寫大小(size), 緩衝區(buffer)
typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);

// Virtual File System 核心節點結構 (類似 Linux 的 inode)
typedef struct fs_node {
    char name[128];     // 檔案名稱
    uint32_t flags;     // 節點類型 (FS_FILE, FS_DIRECTORY 等)
    uint32_t length;    // 檔案大小 (bytes)
    uint32_t inode;     // 該檔案系統專用的 ID
    uint32_t impl;      // 給底層驅動程式偷藏資料用的數字

    // [多型] 綁定在這個檔案上的操作函式！
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;

    struct fs_node *ptr; // 用來處理掛載點或符號連結
} fs_node_t;

// 系統全域的「根目錄」節點
extern fs_node_t *fs_root;

// Virtual File System 暴露給 Kernel 其他模組呼叫的通用 API
void vfs_open(fs_node_t *node);
void vfs_close(fs_node_t *node);

uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
int vfs_delete_file(uint32_t dir_lba, char* filename);
int vfs_mkdir(uint32_t dir_lba, char* dirname);
int vfs_getcwd(uint32_t dir_lba, char* buffer);

#endif
```
---
src/kernel/include/mouse.h
```h
#ifndef MOUSE_H
#define MOUSE_H

void init_mouse(void);

#endif
```
---
src/kernel/include/timer.h
```h
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void init_timer(uint32_t frequency);
void timer_handler(void);

#endif
```
---
src/kernel/include/gui.h
```h
#ifndef GUI_H
#define GUI_H

#include <stdint.h>

// 定義視窗結構
typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[32];
    int is_active;
    uint32_t owner_pid; // 視窗背後的靈魂 (PID)
    uint32_t* canvas; // 視窗專屬的像素緩衝區

    // 視窗內部事件暫存區
    int last_click_x;    // 點擊在畫布上的相對 X 座標
    int last_click_y;    // 點擊在畫布上的相對 Y 座標
    int has_new_click;   // 1 代表有新事件尚未被 App 讀取

    // 鍵盤事件暫存區
    char last_key;
    int has_new_key;

    // 視窗狀態與控制
    int is_minimized;  // 1 代表縮小到工作列
    int is_maximized;  // 1 代表最大化
    int orig_x;        // 記憶最大化之前的 X 座標
    int orig_y;        // 記憶最大化之前的 Y 座標
    int orig_w;        // 記憶最大化之前的寬度
    int orig_h;        // 記憶最大化之前的高度
    int z_layer;       // 1 為一般層，0 為置底層
} window_t;


// --- GUI API ----

void init_gui(void);
void gui_render(int mouse_x, int mouse_y);
void gui_redraw(void); // 不改變滑鼠座標的原地重繪

// 非同步渲染架構 API
void gui_handle_timer(void);
void gui_update_mouse(int x, int y);

// 檢查是否點擊了系統 UI，回傳 1 代表已處理，0 代表未攔截
int gui_check_ui_click(int x, int y);

// ---- Window API ----
// 註冊一個新視窗
int create_window(int x, int y, int width, int height, const char* title, uint32_t owner_pid);
void close_window(int id);

window_t* get_windows(void);    // 取得全域視窗陣列 (給滑鼠碰撞偵測用)
window_t* get_window(int id);   // 取得單一視窗 (給 TTY 用)

void set_focused_window(int id);
int get_focused_window(void);

void enable_gui_mode(int enable);
void switch_display_mode(int is_gui);

void gui_mouse_down(int x, int y);
void gui_mouse_up(void);

void gui_close_windows_by_pid(uint32_t pid);
int gui_handle_keyboard(char c);

#endif
```
---
src/kernel/include/ethernet.h
```h
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// 乙太網路標頭 (長度剛好 14 bytes)
typedef struct {
    uint8_t  dest_mac[6]; // 目標 MAC 位址
    uint8_t  src_mac[6];  // 來源 MAC 位址
    uint16_t ethertype;   // 協定類型 (如 IPv4, ARP)
} __attribute__((packed)) ethernet_header_t;

// 常見的 EtherType (Big-Endian)
#define ETHERTYPE_IPv4 0x0800
#define ETHERTYPE_ARP  0x0806


/**
 * 因為 x86 CPU 是 Little-Endian（小端序），而網路世界的標準是 Big-Endian（大端序），
 * 所以我們必須手動把這 4 個 bytes 的順序完全反轉過來（從 A B C D 變成 D C B A）。
 * 這就是 htonl (Host to Network Long) 的工作。
 */

// ntohs: network to host short
// 說明：它的主要功能是將一個 16 位元（2 位元組） 的整數從 網路位元組順序（Network Byte Order） 轉換為 主機位元組順序（Host Byte Order）。
// 用途：通常用於處理從網路接收到的資料（如 TCP/UDP 埠號），因為網路通訊協定規定使用「大端序」（Big-Endian），而許多電腦（如 x86 架構）內部使用「小端序」（Little-Endian）。
// 參數與回傳值：它接收一個網路順序的 16 位元無符號短整數（uint16_t），並回傳該數值在當前主機系統上的正確順序。
// 平台差異：如果主機本身就使用大端序，此函式通常不執行任何操作；若主機使用小端序，它會將位元組順序反轉。
//
// htons: host to network short
// ntohl: network to host long
// htonl: host to network long
// more see: https://linux.die.net/man/3/ntohs
// 網路端與主機端的 Endianness 轉換
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}
#define htons ntohs

// 【新增】網路端與主機端的 Endianness 轉換 (32-bit)
static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8)  |
           ((hostlong & 0x00FF0000) >> 8)  |
           ((hostlong & 0xFF000000) >> 24);
}
#define ntohl htonl

#endif
```
---
src/kernel/include/pmm.h
```h
// Physical Memory Manager: Physical Memory Management
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

#define PMM_FRAME_SIZE 4096 // 每塊地的大小是 4KB

// 初始化 Physical Memory Manager，傳入系統總記憶體大小 (以 KB 為單位)
void init_pmm(uint32_t mem_size_kb);

// 索取一塊 4KB 的實體記憶體，回傳其記憶體位址
void* pmm_alloc_page(void);

// 釋放一塊 4KB 的實體記憶體
void pmm_free_page(void* ptr);

void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames);

#endif
```
---
src/kernel/include/gfx.h
```h
// 2D 繪圖引擎庫
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "multiboot.h"

// 初始化圖形引擎
void init_gfx(multiboot_info_t* mbd);

// 核心繪圖 API
void put_pixel(int x, int y, uint32_t color);
uint32_t get_pixel(int x, int y);

// 我們即將加入的進階 API
void draw_rect(int x, int y, int width, int height, uint32_t color);
void draw_char(char c, int x, int y, uint32_t fg_color, uint32_t bg_color);
void draw_cursor(int x, int y);
void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color);
void draw_window(int x, int y, int width, int height, const char* title);
void draw_char_transparent(char c, int x, int y, uint32_t fg_color); // 處理 double buffering

// 處理 Double Buffering
void gfx_swap_buffers(void);

#endif
```
---
src/kernel/include/keyboard.h
```h
#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void keyboard_handler(void);
char keyboard_getchar();

#endif
```
---
src/kernel/include/io.h
```h
#ifndef IO_H
#define IO_H

#include <stdint.h>

// ==========================================
// 8-bit I/O (Byte)
// ==========================================
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// ==========================================
// 16-bit I/O (Word)
// ==========================================
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// ==========================================
// 32-bit I/O (Long / Double Word)
// ==========================================
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#endif
```
---
src/kernel/include/rtc.h
```h
#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// 取得目前的小時與分鐘
void read_time(int* hour, int* minute);

#endif
```
---
src/kernel/include/paging.h
```h
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/**
 * @brief 初始化分頁 (Paging) 系統，建立 Kernel 基礎的記憶體映射。
 */
void init_paging(void);

/**
 * @brief 將實體位址 (Physical) 映射到虛擬位址 (Virtual)。
 * * @param virt  欲映射的虛擬位址。
 * @param phys  對應的實體位址。
 * @param flags 權限標籤。常見組合：
 * - 3 (0b011): Present + Read/Write (Kernel 模式)
 * - 7 (0b111): Present + Read/Write + User (User 模式)
 */
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/**
 * @brief 專門用來映射 Video RAM / Framebuffer (MMIO) 的輔助函數。
 * * @param virt 虛擬位址。
 * @param phys 實體位址。
 */
void map_vram(uint32_t virt, uint32_t phys);

/**
 * @brief 取得目前已分配的 Universe (獨立位址空間) 總數。
 * * @return 正在使用的 Universe 數量。
 */
uint32_t paging_get_active_universes(void);

/**
 * @brief 為新的 Process 建立一個獨立的 Page Directory (Universe)。
 * * @return 新 Page Directory 的實體位址。
 */
uint32_t create_page_directory(void);

/**
 * @brief 釋放指定的 Page Directory，並回收其所佔用的實體記憶體。
 * * @param pd_phys 欲釋放的 Page Directory 實體位址。
 */
void free_page_directory(uint32_t pd_phys);

#endif
```
---
src/kernel/include/arp.h
```h
#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include "ethernet.h"

#define ARP_REQUEST 1
#define ARP_REPLY   2

// ARP 封包結構 (IPv4 over Ethernet)
typedef struct {
    uint16_t hardware_type; // 0x0001 (Ethernet)
    uint16_t protocol_type; // 0x0800 (IPv4)
    uint8_t  hw_addr_len;   // 6 (MAC 位址長度)
    uint8_t  proto_addr_len;// 4 (IPv4 位址長度)
    uint16_t opcode;        // 1 代表 Request, 2 代表 Reply
    uint8_t  src_mac[6];    // 來源 MAC
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_mac[6];   // 目標 MAC (Request 時填 0)
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) arp_packet_t;


void arp_send_request(uint8_t* target_ip);
void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac);
void arp_update_table(uint8_t* ip, uint8_t* mac);
uint8_t* arp_lookup(uint8_t* ip);

#endif
```
---
src/kernel/include/kheap.h
```h
#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

// 記憶體區塊的標頭 (類似地契，記錄這塊地有多大、有沒有人住)
typedef struct block_header {
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

// 初始化 Kernel Heap
void init_kheap(void);

// 動態配置指定大小的記憶體 (類似 malloc)
void* kmalloc(size_t size);

// 釋放記憶體 (類似 free)
void kfree(void* ptr);

#endif
```
---
src/kernel/include/gdt.h
```h
// Global Descriptor Table: Global Descriptor Table
// TSS: Task State Segment
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Global Descriptor Table 單一條目的結構
struct gdt_entry_struct {
    uint16_t limit_low;   // 區塊長度 (下半部)
    uint16_t base_low;    // 區塊起始位址 (下半部)
    uint8_t  base_middle; // 區塊起始位址 (中間)
    uint8_t  access;      // 存取權限 (Ring 0/3, 程式碼/資料)
    uint8_t  granularity; // 顆粒度與長度 (上半部)
    uint8_t  base_high;   // 區塊起始位址 (上半部)
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// 指向 Global Descriptor Table 陣列的指標結構 (給 lgdt 指令用的)
struct gdt_ptr_struct {
    uint16_t limit;       // Global Descriptor Table 陣列的總長度 - 1
    uint32_t base;        // Global Descriptor Table 陣列的起始位址
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

struct tss_entry_struct {
    uint32_t prev_tss;   // 舊的 TSS (不用理它)
    uint32_t esp0;       // [極重要] Ring 0 的堆疊指標
    uint32_t ss0;        // [極重要] Ring 0 的堆疊區段 (通常是 0x10)
    // ... 中間還有非常多暫存器狀態，為了簡化，我們宣告成一個大陣列填滿空間
    uint32_t unused[23];
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;


// 初始化 Global Descriptor Table 的公開函式
void init_gdt(void);

// 新增在 lib/gdt.h
void set_kernel_stack(uint32_t esp);


#endif
```
---
src/kernel/include/pci.h
```h
#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI Configuration Space 的標準 I/O Ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// 常見的網卡硬體 ID
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139
#define E1000_VENDOR_ID    0x8086
#define E1000_DEVICE_ID    0x100E

void init_pci(void);
uint32_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);

#endif
```
---
src/kernel/include/syscall.h
```h
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "task.h"

#define TEST_TCP_PORT    54321

// ==========================================
// 系統呼叫編號定義 (Syscall Numbers)
// ==========================================
#define SYS_PRINT_STR_LEN   0
#define SYS_PRINT_STR       2
#define SYS_OPEN            3
#define SYS_READ            4
#define SYS_GETCHAR         5
#define SYS_YIELD           6
#define SYS_EXIT            7
#define SYS_FORK            8
#define SYS_EXEC            9
#define SYS_WAIT            10
#define SYS_IPC_SEND        11
#define SYS_IPC_RECV        12
#define SYS_SBRK            13
#define SYS_CREATE          14
#define SYS_READDIR         15
#define SYS_REMOVE          16
#define SYS_MKDIR           17
#define SYS_CHDIR           18
#define SYS_GETCWD          19
#define SYS_GETPID          20
#define SYS_GETPROCS        21
#define SYS_CLEAR_SCREEN    22
#define SYS_KILL            24
#define SYS_GET_MEM_INFO    25
#define SYS_CREATE_WINDOW   26
#define SYS_UPDATE_WINDOW   27
#define SYS_GET_TIME        28
#define SYS_GET_WIN_EVENT   29
#define SYS_GET_WIN_KEY     30
#define SYS_PING            31
#define SYS_NET_UDP_SEND    32
#define SYS_NET_UDP_BIND    33
#define SYS_NET_UDP_RECV    34
#define SYS_NET_TCP_CONNECT 35
#define SYS_NET_TCP_SEND    36
#define SYS_NET_TCP_CLOSE   37
#define SYS_NET_TCP_RECV    38
#define SYS_WRITE           39
#define SYS_SET_DISPLAY_MODE 99

/**
 * @brief 系統記憶體狀態資訊
 */
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;

/** @brief 系統呼叫處理函式原型 */
typedef int (*syscall_t)(registers_t*);

/** @brief 初始化系統呼叫子系統 */
void init_syscalls(void);

/**
 * @brief 系統呼叫主要分派器 (Dispatcher)
 * @param regs 中斷發生時的暫存器快照
 */
void syscall_handler(registers_t *regs);

#endif
```
---
src/kernel/include/udp.h
```h
#ifndef UDP_H
#define UDP_H

#include <stdint.h>

typedef struct {
    uint16_t src_port;   // 來源通訊埠
    uint16_t dest_port;  // 目標通訊埠
    uint16_t length;     // UDP 總長度 (標頭 + 資料)
    uint16_t checksum;   // 檢查碼
} __attribute__((packed)) udp_header_t;

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);

#endif
```


---

---
src/user/include/stdlib.h
```h
#ifndef _STDLIB_H
#define _STDLIB_H

/**
 * @file stdlib.h
 * @brief 標準程式庫 (記憶體管理)
 */

#include <stddef.h>

/**
 * @brief 動態分配記憶體
 * @param size 分配大小
 * @return 指向分配空間的指標，若失敗則回傳 0
 */
void* malloc(int size);

/**
 * @brief 釋放動態分配的記憶體
 * @param ptr 指向要釋放空間的指標
 */
void free(void* ptr);

void* memset(void* bufptr, int value, size_t size);

#endif
```
---
src/user/include/net.h
```h
#ifndef _LIBC_NET_H
#define _LIBC_NET_H

#include <stdint.h>

/**
 * @file net.h
 * @brief 網路相關工具與系統呼叫介面
 */

/**
 * @brief 簡易的字串轉 IP 陣列函式
 * @param str IP 字串 (例如 "10.0.2.2")
 * @param ip 輸出的 4 位元組 IP 陣列
 */
void parse_ip(char* str, uint8_t* ip);

/**
 * @brief 發送 ICMP Ping 請求
 * @param ip 目標 IP 陣列
 */
void sys_ping(uint8_t* ip);

// UDP
void sys_udp_send(uint8_t* ip, uint16_t port, uint8_t* data, int len);
void sys_udp_bind(uint16_t port);
int sys_udp_recv(char* buffer);

// TCP
void sys_tcp_connect(uint8_t* ip, uint16_t port);
void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg);
void sys_tcp_close(uint8_t* ip, uint16_t port);
int sys_tcp_recv(char* buffer);

#endif
```
---
src/user/include/unistd.h
```h
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stdint.h>

/**
 * @file unistd.h
 * @brief 定義標準的 POSIX 系統呼叫封裝介面
 */

// 行程資訊結構 (用於 get_process_list)
typedef struct {
    unsigned int pid;
    unsigned int ppid;
    char name[32];
    unsigned int state;
    unsigned int memory_used;
    unsigned int total_ticks;
} process_info_t;

// 系統記憶體資訊結構 (與核心核心核心同步)
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;

// ==========================================
// 行程管理 (Process Management)
// ==========================================
int fork(void);
int exec(char* filename, char** argv);
int wait(int pid);
void yield(void);
void exit(void) __attribute__((noreturn));
int getpid(void);
int get_process_list(process_info_t* list, int max_count);
int kill(int pid);

// ==========================================
// 檔案系統 (File System)
// ==========================================
int open(const char* filename);
int read(int fd, void* buffer, int size);
int create_file(const char* filename, const char* content);
int remove(const char* filename);
int readdir(int index, char* out_name, int* out_size, int* out_type);
int mkdir(const char* dirname);
int chdir(const char* dirname);
int getcwd(char* buffer);

// ==========================================
// 記憶體管理 (Memory Management)
// ==========================================
void* sbrk(int increment);
int get_mem_info(mem_info_t* info);

// ==========================================
// 行程間通訊 (IPC)
// ==========================================
void send(char* msg);
int recv(char* buffer);

// ==========================================
// 系統控制 (System Control)
// ==========================================
int set_display_mode(int is_gui);
void clear_screen(void);
void get_time(int* h, int* m);

#endif
```
---
src/user/include/stdio.h
```h
#ifndef _STDIO_H
#define _STDIO_H

/**
 * @file stdio.h
 * @brief 標準輸入輸出函式
 */

/**
 * @brief 印出字串到終端機 (目前為同步模式)
 * @param str 要印出的字串
 */
void print(const char* str);

/**
 * @brief 從鍵盤讀取一個字元 (若無按鍵則阻塞)
 * @return 讀取到的字元
 */
char getchar(void);

/**
 * @brief 格式化輸出到終端機
 * @details 目前支援 %d (十進位), %x (十六進位), %s (字串), %c (字元), %% (百分比)
 * @param format 格式字串
 * @param ... 變數參數列表
 */
void printf(const char* format, ...);

#endif
```
---
src/user/include/stdarg.h
```h
#ifndef _STDARG_H
#define _STDARG_H

// 直接借用 GCC 編譯器內建的不定參數處理巨集
#define va_list         __builtin_va_list
#define va_start(ap, p) __builtin_va_start(ap, p)
#define va_arg(ap, t)   __builtin_va_arg(ap, t)
#define va_end(ap)      __builtin_va_end(ap)

#endif
```
---
src/user/include/dns.h
```h
#ifndef DNS_H
#define DNS_H

#include <stdint.h>

typedef struct {
    uint16_t transaction_id; // 交易 ID (我們自己亂數定一個，例如 0x1234)
    uint16_t flags;          // 旗標 (查詢是 0x0100)
    uint16_t questions;      // 幾個問題？ (通常是 1)
    uint16_t answer_rrs;     // 幾個回答？ (發問時是 0)
    uint16_t authority_rrs;  // 授權記錄數量
    uint16_t additional_rrs; // 額外記錄數量
} __attribute__((packed)) dns_header_t;

// User Space 用的簡易 Endianness 轉換 (從 ethernet.h 抄過來的)
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}
static inline uint16_t ntohs(uint16_t netshort) { return htons(netshort); }

#endif
```
---
src/user/include/syscall.h
```h
#ifndef _LIBC_SYSCALL_H
#define _LIBC_SYSCALL_H

#include <stdint.h>

/**
 * @file syscall.h
 * @brief 定義系統呼叫編號與通用呼叫介面
 */

// ==========================================
// 系統呼叫編號 (與核心定義同步)
// ==========================================
#define SYS_PRINT_STR_LEN    0
#define SYS_PRINT_STR        2
#define SYS_OPEN             3
#define SYS_READ             4
#define SYS_GETCHAR          5
#define SYS_YIELD            6
#define SYS_EXIT             7
#define SYS_FORK             8
#define SYS_EXEC             9
#define SYS_WAIT             10
#define SYS_IPC_SEND         11
#define SYS_IPC_RECV         12
#define SYS_SBRK             13
#define SYS_CREATE           14
#define SYS_READDIR          15
#define SYS_REMOVE           16
#define SYS_MKDIR            17
#define SYS_CHDIR            18
#define SYS_GETCWD           19
#define SYS_GETPID           20
#define SYS_GETPROCS         21
#define SYS_CLEAR_SCREEN     22
#define SYS_KILL             24
#define SYS_GET_MEM_INFO     25
#define SYS_CREATE_WINDOW    26
#define SYS_UPDATE_WINDOW    27
#define SYS_GET_TIME         28
#define SYS_GET_WIN_EVENT    29
#define SYS_GET_WIN_KEY      30
#define SYS_PING             31
#define SYS_NET_UDP_SEND     32
#define SYS_NET_UDP_BIND     33
#define SYS_NET_UDP_RECV     34
#define SYS_NET_TCP_CONNECT  35
#define SYS_NET_TCP_SEND     36
#define SYS_NET_TCP_CLOSE    37
#define SYS_NET_TCP_RECV     38
#define SYS_SET_DISPLAY_MODE 99

/**
 * @brief 通用系統呼叫封裝
 * @param num 系統呼叫編號
 * @param p1 參數 1 (ebx)
 * @param p2 參數 2 (ecx)
 * @param p3 參數 3 (edx)
 * @param p4 參數 4 (esi)
 * @param p5 參數 5 (edi)
 * @return 核心回傳值 (eax)
 */
int syscall(int num, int p1, int p2, int p3, int p4, int p5);

#endif
```
---
src/user/include/simpleui.h
```h
#ifndef SIMPLEUI_H
#define SIMPLEUI_H

/**
 * @file simpleui.h
 * @brief 使用者界面基礎繪圖與視窗管理介面
 */

// ==========================================
// 標準調色盤 (Standard Palette)
// ==========================================
#define UI_COLOR_WHITE      0xFFFFFF
#define UI_COLOR_BLACK      0x000000
#define UI_COLOR_GRAY       0xC0C0C0
#define UI_COLOR_DARK_GRAY  0x808080
#define UI_COLOR_RED        0xFF0000
#define UI_COLOR_BLUE       0x4169E1
#define UI_COLOR_YELLOW     0xFFD700
#define UI_COLOR_GREEN      0x00FF00

// ==========================================
// 視窗管理 (Window Management)
// ==========================================
int create_gui_window(const char* title, int width, int height);
void update_gui_window(int win_id, unsigned int* buffer);
int get_window_event(int win_id, int* x, int* y);
int get_window_key_event(int win_id, char* key);

// ==========================================
// 繪圖元件與工具 (UI Components & Tools)
// ==========================================

// 標準按鈕結構
typedef struct {
    int x, y, w, h;
    char text[32];
    unsigned int bg_color;
    unsigned int text_color;
    int is_pressed; // 記錄按鈕是否正在被按下 (用來畫 3D 凹陷效果)
} ui_button_t;

/**
 * @brief 畫實心矩形
 */
void ui_draw_rect(unsigned int* canvas, int cw, int ch, int x, int y, int w, int h, unsigned int color);

/**
 * @brief 在畫布上繪製文字 (8x8 字型)
 */
void ui_draw_text(unsigned int* canvas, int cw, int ch, int x, int y, const char* str, unsigned int color);

/**
 * @brief 在畫布上繪製單一數字 (放大版 3x5 字型)
 */
void ui_draw_digit(unsigned int* canvas, int cw, int ch, int digit, int ox, int oy, unsigned int color);

/**
 * @brief 繪製具有 3D 效果的按鈕
 */
void ui_draw_button(unsigned int* canvas, int cw, int ch, ui_button_t* btn);

/**
 * @brief 偵測座標是否落在按鈕範圍內
 */
int ui_is_clicked(ui_button_t* btn, int cx, int cy);

#endif
```
---
src/user/include/string.h
```h
#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

/**
 * @file string.h
 * @brief 標準字串處理函式
 */

/**
 * @brief 計算字串長度
 * @param s 字串
 * @return 字串長度 (不含空字元)
 */
int strlen(const char* s);

/**
 * @brief 複製字串
 * @param dest 目標緩衝區
 * @param src 來源字串
 */
void strcpy(char* dest, const char* src);

/**
 * @brief 比較兩個字串
 * @param s1 字串 1
 * @param s2 字串 2
 * @return 0 表示相等，非 0 表示不相等 (其差值)
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief 解析指令參數 (處理空白與引號)
 * @param input 輸入字串 (會被修改，插入 '\0')
 * @param argv 輸出的參數指標陣列
 * @return 參數個數 (argc)
 */
int parse_args(char* input, char** argv);

// 將字串轉為整數 (用來解析 Port)
int atoi(char* str);

#endif
```
