### Simple OS 核心名詞對照表

這份清單涵蓋了我們這兩週所接觸的所有底層硬體、CPU 架構、軟體工具與程式設計概念。

#### 硬體與 CPU 架構層 (The Hardware)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **CPU** | Central Processing Unit | 中央處理器 | 1 | 電腦的心臟，負責執行所有指令。我們使用的是 x86 架構。 |
| **RAM** | Random Access Memory | 隨機存取記憶體 | 1 | 實體記憶體，系統運行時存放程式碼與資料的地方。 |
| **VGA** | Video Graphics Array | 顯示卡 | 2 | 負責在螢幕上顯示影像的硬體。我們使用其「文字模式 (Text Mode)」。 |
| **PIC** | Programmable Interrupt Controller | 可程式化中斷控制器 | 8 | CPU 的硬體「秘書」，負責排隊與管理外部硬體發出的中斷訊號（IRQ）。通常指 8259 晶片。 |
| **PIT** | Programmable Interval Timer | 可程式化間隔計時器 | 11 | 系統的「石英震盪器」，提供精準的時間脈衝（IRQ 0），通常指 8254 晶片。 |
| **IRQ** | Interrupt Request | 中斷請求 | 8 | 硬體對 PIC 發出的「有新資料」訊號（例如 IRQ 1 是鍵盤）。 |
| **Port** | I/O Port | I/O 通訊埠 | 4 | CPU 與硬體交換指令與資料的獨立通道（信箱編號）。 |
| **TLB** | Translation Lookaside Buffer | 轉譯後備緩衝區 | 14 | CPU 內部的硬體快取，用來記住「虛擬位址到實體位址」的翻譯結果。修改分頁表後必須用 `invlpg` 指令刷新它。 |
| **Ring 0 / 3** | Privilege Rings | 特權等級 | 16 | x86 硬體的權限分級防護。Ring 0 是核心專屬的最高權限（上帝視角），Ring 3 是使用者應用程式的最低權限。 |

#### CPU 內部暫存器 (CPU Registers)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **esp** | Extended Stack Pointer | 堆疊指標暫存器 | 2 | [極重要] 記錄當前堆疊（工作桌）的最高（最舊）位址。堆疊是 C 語言函式呼叫與區域變數的依賴。 |
| **eip** | Extended Instruction Pointer | 指令指標暫存器 | 12 | 記錄 CPU 當前（或下一條）要執行的指令在記憶體中的位址。 |
| **CR0** | Control Register 0 | 控制暫存器 0 | 10 | 內含多個系統狀態旗標，最高位 PG (bit 31) 是虛擬記憶體（分頁機制）的總開關。 |
| **CR3** | Control Register 3 | 控制暫存器 3 | 10 | 負責存放當前任務的「分頁目錄 (Page Directory)」的實體位址（翻譯字典的首頁）。 |
| **cs** | Code Segment | 程式碼區段暫存器 | 16 | 記錄當前執行的程式碼區段。CPU 也是透過讀取它底層的 CPL (Current Privilege Level) 來判斷現在是處於 Ring 0 還是 Ring 3。 |

#### x86 核心資料結構 (Core Data Structures)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **GDT** | Global Descriptor Table | 全域描述符表 | 6 | 定義記憶體「區塊 (Segments)」屬性與「特權等級 (Ring)」的名冊。系統安管基石。 |
| **IDT** | Interrupt Descriptor Table | 中斷描述符表 | 7 | 記錄中斷與例外錯誤「發生時要跳去哪裡 (ISR)」的聯絡簿。 |
| **ISR** | Interrupt Service Routine | 中斷處理程式 | 7 | 專門用來處理特定中斷或例外錯誤的組合語言跳板或 C 語言函式。 |
| **TCB** | Task Control Block | 任務控制區塊 | 12 | 作業系統內部用來管理任務（Process/Thread）所有資訊（特別是其 `esp`）的 C 語言資料結構。 |
| **PD** | Page Directory | 分頁目錄 | 10 | 虛擬記憶體字典的「目錄」，包含 1024 個指向分頁表 (PT) 的項目。 |
| **PT** | Page Table | 分頁表 | 10 | 虛擬記憶體字典的「內文」，包含 1024 個指向 4KB 實體頁框的項目。 |
| **Bitmap** | Bitmap | 位元圖 | 13 | 使用 0 或 1 來極致壓縮記錄實體記憶體 (Page Frame) 使用狀態的陣列。 |
| **Block Header** | Heap Block Header | 區塊標頭 | 15 | Heap 管理中，偷偷安插在每一個分配出去的記憶體區塊前方的結構，用來記錄該區塊的「大小」與「是否空閒」。 |

#### 軟體工具與概念 (Software & Concepts)

| 縮寫 | 全名 | 繁體中文名 | Day | 用途與說明 |
| --- | --- | --- | --- | --- |
| **OK** | Green "OK" | 綠色 OK 字樣 | 1 | 我們作業系統成功開機後的畫面。我們用它來證明核心運作正常。 |
| **QEMU** | QEMU Emulator | QEMU 模擬器 | 1 | 我們用來測試作業系統的虛擬機軟體。 |
| **GRUB** | GRUB Bootloader | GRUB 啟動載入器 | 1 | 負責從硬碟載入我們核心的軟體，並交出 CPU 控制權。 |
| **Freestanding** | Freestanding Environment | 獨立執行環境 | 4 | 告訴 GCC：「我的核心是直接跑在硬體上的，沒有標準函式庫可用，一切自己來。」的編譯參數。 |
| **Multitasking** | Multitasking | 多工處理 | 11 | CPU 利用 Timer 中斷，快速地在多個任務之間交替執行，營造出「同時運行」的幻覺。 |
| **Context Switch** | Context Switch | 上下文切換 (任務切換) | 12 | [極核心] 保存當前任務的 CPU 狀態（esp、暫存器）並載入下一個任務狀態的過程。 |
| **PMM** | Physical Memory Management | 實體記憶體管理 | 13 | 系統的「地政事務所」，負責精準追蹤與分配實體 RAM 中每一塊空地。 |
| **Frame** | Page Frame | 實體頁框 | 13 | 實體記憶體被劃分成的最小標準單位，在 x86 系統中通常為 4KB。 |
| **Paging** | Paging Mechanism | 分頁機制 (虛擬記憶體) | 14 | 將應用程式眼中的「虛擬位址」翻譯對應到「實體頁框」的 CPU 特權魔法。 |
| **Heap** | Kernel Heap | 核心堆積 | 15 | OS 向 PMM 批發一大塊 4KB 記憶體後，用來「零售（切割）」給各種動態需求（字串、結構體）的記憶體池。 |
| **kmalloc** | Kernel Malloc | 核心動態配置 | 15 | 核心專屬的動態記憶體分配函式，負責在 Heap 中尋找空閒區塊並進行切割。 |
| **Syscall** | System Call | 系統呼叫 | 16 | OS 提供給 User App 的「防彈玻璃窗口」，讓缺乏權限的程式能安全地請求核心服務（如要記憶體、印字串）。 |
| **Soft IRQ** | Software Interrupt | 軟體中斷 | 16 | 程式透過指令 (如 `int`) 主動觸發的中斷，通常用於呼叫 Syscall。 |
| **int 0x80** | Interrupt 0x80 | 0x80 號中斷 | 16 | Linux 與 x86 架構下，最傳統且神聖的系統呼叫專用中斷號碼（十進位 128）。 |
