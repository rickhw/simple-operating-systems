### Simple OS 核心名詞對照表 (Day 01 - 38 終極版)

這份清單涵蓋了我們這段期間所接觸的所有底層硬體、CPU 架構、軟體工具與程式設計概念。我已經將最新的**多工作業排程、行程生命週期與 MMU 記憶體隔離概念**加入其中。

#### 硬體與 CPU 架構層 (The Hardware)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **CPU** | Central Processing Unit | 中央處理器 | 1 | 電腦的心臟，負責執行所有指令。我們使用的是 x86 架構。 |
| **RAM** | Random Access Memory | 隨機存取記憶體 | 1 | 實體記憶體，系統運行時存放程式碼與資料的地方。 |
| **VGA** | Video Graphics Array | 顯示卡 | 2 | 負責在螢幕上顯示影像的硬體。我們使用其「文字模式 (Text Mode)」。 |
| **PIC** | Programmable Interrupt Controller | 可程式化中斷控制器 | 8 | CPU 的硬體「秘書」，負責排隊與管理外部硬體發出的中斷訊號（IRQ）。通常指 8259 晶片。 |
| **PIT** | Programmable Interval Timer | 可程式化間隔計時器 | 11 | 系統的「石英震盪器」，提供精準的時間脈衝（IRQ 0），通常指 8254 晶片。在 Day 31 成為了驅動排程器心跳的關鍵。 |
| **IRQ** | Interrupt Request | 中斷請求 | 8 | 硬體對 PIC 發出的「有新資料」訊號（例如 IRQ 1 是鍵盤）。 |
| **Port** | I/O Port | I/O 通訊埠 | 4 | CPU 與硬體交換指令與資料的獨立通道（信箱編號）。 |
| **MMU** | Memory Management Unit | 記憶體管理單元 | 38 | CPU 內部負責將虛擬位址轉換為實體位址的硬體神經網路，是實現 Process 記憶體隔離的最核心機制。 |
| **TLB** | Translation Lookaside Buffer | 轉譯後備緩衝區 | 14 | MMU 的專屬快取，用來記住「虛擬位址到實體位址」的翻譯結果。切換宇宙 (CR3) 時硬體會自動清空它。 |
| **Ring 0 / 3** | Privilege Rings | 特權等級 | 16 | x86 硬體的權限分級防護。Ring 0 是核心專屬的最高權限（上帝視角），Ring 3 是使用者應用程式的最低權限。 |
| **ATA/IDE** | Advanced Technology Attachment | 先進技術附加介面 | 21 | 傳統硬碟的傳輸介面標準，我們透過 I/O Port (`0x1F0`~`0x1F7`) 與其控制器溝通。 |
| **LBA** | Logical Block Addressing | 邏輯區塊位址 | 21 | 現代硬碟的尋址方式。將硬碟視為一個巨大的陣列，從 LBA 0 開始，每個區塊通常是 512 Bytes。 |

#### CPU 內部暫存器 (CPU Registers)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **esp** | Extended Stack Pointer | 堆疊指標暫存器 | 2 | [極重要] 記錄當前堆疊的最高（最舊）位址。在排程器中，保存與替換 `esp` 是 Context Switch 的核心魔法。 |
| **eip** | Extended Instruction Pointer | 指令指標暫存器 | 12 | 記錄 CPU 當前（或下一條）要執行的指令在記憶體中的位址。 |
| **CR0** | Control Register 0 | 控制暫存器 0 | 10 | 內含多個系統狀態旗標，最高位 PG (bit 31) 是虛擬記憶體（分頁機制）的總開關。 |
| **CR3** | Control Register 3 | 控制暫存器 3 | 10 | [極重要] 存放當前任務的「分頁目錄 (Page Directory)」實體位址。Day 38 學到，切換 CR3 就是在切換不同的平行宇宙。 |
| **cs** | Code Segment | 程式碼區段暫存器 | 16 | 記錄當前執行的程式碼區段。CPU 透過讀取其底層的 CPL 判斷當前處於 Ring 0 還是 Ring 3。 |
| **eflags** | Extended Flags Register | 狀態旗標暫存器 | 17 | 存放各種系統狀態（如是否允許中斷 IF 旗標）。在切換至 Ring 3 或 Context Switch 時極度關鍵。 |

#### x86 核心資料結構 (Core Data Structures)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **GDT** | Global Descriptor Table | 全域描述符表 | 6 | 定義記憶體「區塊 (Segments)」屬性與「特權等級 (Ring)」的名冊。系統安管基石。 |
| **IDT** | Interrupt Descriptor Table | 中斷描述符表 | 7 | 記錄中斷與例外錯誤「發生時要跳去哪裡 (ISR)」的聯絡簿。 |
| **ISR** | Interrupt Service Routine | 中斷處理程式 | 7 | 專門用來處理特定中斷或例外錯誤的組合語言跳板或 C 語言函式。 |
| **TCB** | Task Control Block | 任務控制區塊 | 12 | OS 管理任務的所有資訊。包含 `esp`, `id`, `state` 以及 Day 38 新增的 `page_directory` (宇宙地契)。 |
| **PD/PT** | Page Directory/Table | 分頁目錄與分頁表 | 10 | 虛擬記憶體字典。PD 是目錄 (1024項)，PT 是內文 (指向 4KB 實體頁框)。 |
| **TSS** | Task State Segment | 任務狀態段 | 17 | CPU 規定的「逃生路線圖」。當多工切換時，必須動態更新其中的 `esp0`，確保中斷發生時有專屬的核心堆疊可用。 |
| **MBR** | Master Boot Record | 主開機紀錄 | 23 | 硬碟第 0 號磁區 (LBA 0) 的地契資料，存放開機引導程式與「分區表 (Partition Table)」。 |
| **fs_node** | File System Node | 檔案節點 (inode) | 24 | VFS 中代表檔案、目錄或裝置的核心結構，內部綁定了驅動程式的函式指標。 |
| **Superblock** | Superblock | 超級區塊 | 25 | 檔案系統的「總地契」，記錄魔法數字、根目錄位置與分區總大小等關鍵 Metadata。 |
| **Ring Buffer** | Circular Buffer | 環狀緩衝區 | 30 | 利用頭尾指標追梭，實作 FIFO (先進先出) 的字元暫存區，用於鍵盤輸入。 |

#### 軟體工具與作業系統概念 (Software & Concepts)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **Context Switch** | Context Switch | 上下文切換 | 12 | [極核心] 保存當前任務的 CPU 狀態並載入下一個任務的過程。Day 38 進化為連同 CR3 一併切換的「跨宇宙跳轉」。 |
| **Preemption** | Preemptive Multitasking | 搶佔式多工作業 | 31 | 利用硬體時鐘 (PIT) 中斷，強行從目前執行的程式手中奪走 CPU 控制權的排程機制。 |
| **Process State**| Task State Machine | 行程狀態機 | 33 | 描述任務生命週期的狀態：`RUNNING` (執行中)、`WAITING` (等待中)、`DEAD` (已死亡待清理)。 |
| **Fork** | sys_fork | 複製行程 | 34 | UNIX 哲學。將當前行程（包含暫存器、堆疊與記憶體映射）完美複製一份，產生「子行程」。 |
| **Exec** | sys_exec | 替換執行檔 | 35 | UNIX 哲學。將當前行程的靈魂（記憶體與宇宙）抹除，並載入全新的 ELF 檔案與傳遞 `argc/argv` 參數。 |
| **Idle Task** | System Idle Process | 系統空閒任務 | 36 | 永遠不會死亡、權限最低的背景任務。當沒有其他工作時負責執行 `hlt` 讓 CPU 省電。 |
| **Memory Isolation**| Memory Isolation | 記憶體隔離 | 38 | 透過為每個 Process 提供獨立的 Page Directory，確保應用程式之間不會互相存取或覆蓋對方（奪舍）。 |
| **VFS** | Virtual File System | 虛擬檔案系統 | 24 | 「萬物皆檔案」的介面層，將不同的實體檔案系統統一封裝為標準 API。 |
