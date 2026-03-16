### Simple OS 核心名詞對照表 (Day 01 - 33 終極版)

這份清單涵蓋了我們這段期間所接觸的所有底層硬體、CPU 架構、軟體工具與程式設計概念。我已經將最新的多工排程與狀態管理概念加入其中。

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
| **TLB** | Translation Lookaside Buffer | 轉譯後備緩衝區 | 14 | CPU 內部的硬體快取，用來記住「虛擬位址到實體位址」的翻譯結果。修改分頁表後必須用 `invlpg` 指令刷新它。 |
| **Ring 0 / 3** | Privilege Rings | 特權等級 | 16 | x86 硬體的權限分級防護。Ring 0 是核心專屬的最高權限（上帝視角），Ring 3 是使用者應用程式的最低權限。 |
| **ATA/IDE** | Advanced Technology Attachment | 先進技術附加介面 | 21 | 傳統硬碟的傳輸介面標準，我們透過 I/O Port (`0x1F0`~`0x1F7`) 與其控制器溝通。 |
| **LBA** | Logical Block Addressing | 邏輯區塊位址 | 21 | 現代硬碟的尋址方式。將硬碟視為一個巨大的陣列，從 LBA 0 開始，每個區塊通常是 512 Bytes。 |

#### CPU 內部暫存器 (CPU Registers)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **esp** | Extended Stack Pointer | 堆疊指標暫存器 | 2 | [極重要] 記錄當前堆疊（工作桌）的最高（最舊）位址。在排程器中，保存與替換 `esp` 是 Context Switch 的核心魔法。 |
| **eip** | Extended Instruction Pointer | 指令指標暫存器 | 12 | 記錄 CPU 當前（或下一條）要執行的指令在記憶體中的位址。 |
| **CR0** | Control Register 0 | 控制暫存器 0 | 10 | 內含多個系統狀態旗標，最高位 PG (bit 31) 是虛擬記憶體（分頁機制）的總開關。 |
| **CR3** | Control Register 3 | 控制暫存器 3 | 10 | 負責存放當前任務的「分頁目錄 (Page Directory)」的實體位址（翻譯字典的首頁）。 |
| **cs** | Code Segment | 程式碼區段暫存器 | 16 | 記錄當前執行的程式碼區段。CPU 也是透過讀取它底層的 CPL (Current Privilege Level) 來判斷現在是處於 Ring 0 還是 Ring 3。 |
| **eflags** | Extended Flags Register | 狀態旗標暫存器 | 17 | 存放各種系統狀態（如是否允許中斷 IF 旗標）。在切換至 Ring 3 時，必須透過假堆疊特別設定它以確保中斷開啟。 |

#### x86 核心資料結構 (Core Data Structures)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **GDT** | Global Descriptor Table | 全域描述符表 | 6 | 定義記憶體「區塊 (Segments)」屬性與「特權等級 (Ring)」的名冊。系統安管基石。 |
| **IDT** | Interrupt Descriptor Table | 中斷描述符表 | 7 | 記錄中斷與例外錯誤「發生時要跳去哪裡 (ISR)」的聯絡簿。 |
| **ISR** | Interrupt Service Routine | 中斷處理程式 | 7 | 專門用來處理特定中斷或例外錯誤的組合語言跳板或 C 語言函式。 |
| **TCB** | Task Control Block | 任務控制區塊 | 12 | 作業系統內部用來管理任務（Process/Thread）所有資訊的 C 語言結構。在 Day 32 加入了 `kernel_stack`，Day 33 加入了 `state`。 |
| **PD** | Page Directory | 分頁目錄 | 10 | 虛擬記憶體字典的「目錄」，包含 1024 個指向分頁表 (PT) 的項目。 |
| **PT** | Page Table | 分頁表 | 10 | 虛擬記憶體字典的「內文」，包含 1024 個指向 4KB 實體頁框的項目。 |
| **Bitmap** | Bitmap | 位元圖 | 13 | 使用 0 或 1 來極致壓縮記錄實體記憶體 (Page Frame) 使用狀態的陣列。 |
| **Block Header** | Heap Block Header | 區塊標頭 | 15 | Heap 管理中，偷偷安插在每一個分配出去的記憶體區塊前方的結構，用來記錄該區塊的「大小」與「是否空閒」。 |
| **TSS** | Task State Segment | 任務狀態段 | 17 | CPU 規定的「逃生路線圖」。Day 32 學到，當多工切換時，必須動態更新其中的 `esp0`，確保每個 User App 被中斷時都有獨立的核心堆疊可用。 |
| **MBR** | Master Boot Record | 主開機紀錄 | 23 | 硬碟第 0 號磁區 (LBA 0) 的地契資料，存放了開機引導程式與「分區表 (Partition Table)」。 |
| **fs_node** | File System Node | 檔案節點 (inode) | 24 | VFS 中代表一個檔案、目錄或裝置的核心結構，內部綁定了具體驅動程式的函式指標。 |
| **Superblock** | Superblock | 超級區塊 | 25 | 檔案系統的「總地契」，位於分區的起點，記錄魔法數字、根目錄位置與分區總大小等關鍵 Metadata。 |
| **File Entry** | File/Directory Entry | 檔案實體紀錄 | 25 | 存在於目錄區塊中的結構，負責登記單一檔案的「檔名」、「大小」與「資料起始相對 LBA」。 |
| **Ring Buffer** | Circular Buffer | 環狀緩衝區 | 30 | 鍵盤驅動使用的資料結構，利用頭尾指標的追梭，實作 FIFO (先進先出) 的字元暫存區。 |

#### 軟體工具與概念 (Software & Concepts)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **Context Switch** | Context Switch | 上下文切換 | 12 | [極核心] 保存當前任務的 CPU 狀態（esp、暫存器）並載入下一個任務狀態的過程。我們在 Day 31 透過組合語言完美實現了「狸貓換太子」。 |
| **Preemption** | Preemptive Multitasking | 搶佔式多工作業 | 31 | 利用硬體時鐘 (PIT) 中斷，強行從目前執行的程式手中奪走 CPU 控制權，並交給下一個程式的排程機制。 |
| **Race Condition** | Race Condition | 競爭危害 | 32 | 當多個行程（如雙胞胎 Shell）同時且無序地存取共享資源（如鍵盤緩衝區），導致結果不可預期或資料破碎的現象。 |
| **PMM** | Physical Memory Management | 實體記憶體管理 | 13 | 系統的「地政事務所」，負責精準追蹤與分配實體 RAM 中每一塊空地。 |
| **Paging** | Paging Mechanism | 分頁機制 (虛擬記憶體) | 14 | 將應用程式眼中的「虛擬位址」翻譯對應到「實體頁框」的 CPU 特權魔法。 |
| **Syscall** | System Call | 系統呼叫 | 16 | OS 提供給 User App 的「防彈玻璃窗口」。Day 33 新增了 `sys_yield` (讓出 CPU) 與 `sys_exit` (終止生命)。 |
| **iret** | Interrupt Return | 中斷返回指令 | 17 | 組合語言指令。我們在 Day 32 利用偽造它的彈出資料 (IRET Frame)，達成了將新建立的行程首次降級到 Ring 3 的作弊跳轉。 |
| **ELF** | Executable and Linkable Format | 可執行與可連結格式 | 18 | Linux 與 Unix 系統標準的二進位執行檔格式，包含了機器碼與作業系統所需的載入資訊。 |
| **Loader** | ELF Loader | 程式載入器 | 29 | 核心中負責讀取 ELF、分配虛擬分頁、精準計算 `offset` 複製機器碼，並將權限交接給應用程式的 C 語言邏輯。 |
| **PIO** | Programmed I/O | 可程式化輸出入 | 21 | 最基礎的硬碟通訊模式。CPU 必須親自透過 I/O Port，將資料一個字元一個字元地在 RAM 與硬碟間搬運。 |
| **VFS** | Virtual File System | 虛擬檔案系統 | 24 | 作業系統中的抽象介面層，利用「萬物皆檔案」的設計，將不同的實體檔案系統或硬體裝置統一封裝為標準 API。 |
| **SimpleFS** | Simple File System | 簡單檔案系統 | 25 | 我們為了 Simple OS 手工打造的專屬輕量級檔案系統，直接架構在 ATA 硬碟的 LBA 系統之上。 |
| **FD** | File Descriptor | 檔案描述符 | 28 | 核心發放給 Ring 3 應用程式的整數號碼牌，代表一個已開啟的檔案節點。 |
| **User Stack** | User Stack | 使用者堆疊 | 28 | 核心專門分配給 Ring 3 應用程式的私有記憶體空間。Day 32 學到，為了避免跨界 Page Fault，必須謹慎安排在同一個 4MB PDE 安全區內。 |
