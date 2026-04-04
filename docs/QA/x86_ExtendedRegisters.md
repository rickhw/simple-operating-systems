在 x86 (32-bit) 架構中，這些以 `E` 開頭的暫存器被稱為 **Extended Registers**（擴充暫存器）。我將這些暫存器依照功能進行了分類，並整理成一份專業的參考列表，你可以將這部分直接作為開發文件（Documentation）的一部分。

---

### 🧠 x86 (32-bit) 暫存器功能參考清單
**用於中斷上下文 (Context) 備份與系統呼叫分析**

當我們執行 `pusha` 時，CPU 會按照特定的順序將以下暫存器的值壓入堆疊。以下是每個暫能器的詳細說明：

#### 1. 通用運算暫存器 (General Purpose Registers)
這類暫存器主要用於算術運算、邏輯運算以及資料搬移。

| 暫存器名稱 | 全名 (Full Name) | 核心用途 (Core Function) | 在作業系統/中斷中的角色 (Role in OS/ISR) |
| :--- | :--- | :--- | :--- |
| **EAX** | **E**xtended **A**ccumulator Register | **累加器**。主要用於算術運算、I/O 埠存取。 | **關鍵！** 在 System Call 中，它是**傳回值**的唯一通道。C 語言修改此值，User Mode 才能讀取結果。 |
| **ECX** | **E**xtended **C**ounter Register | **計數器**。用於迴圈 (`loop`)、字串操作的次數控制。 | 用於處理字串複製或在 Interrupt 中控制重試次數。 |
| **EDX** | **E**xtended **D**ata Register | **資料暫存器**。常用於處理大型算術運算（如 `mul`/`div`）的輔助。 | 在處理硬體 I/O（如讀取硬碟、網卡資料）時，常用於存放副結果或位址偏移。 |
| **EBX** | **E**xtended **B**ase Register | **基底暫存器**。常用於存放記憶體位址的基底（Base Address）。 | 在進入 C 語言 handler 前，常作為指標的暫存存放處。 |

#### 2. 指標與索引暫存器 (Pointer & Index Registers)
這類暫存器主要負責「指向」記憶體中的位址，對於管理堆疊與記憶體操作至關重要。

| 暫存器名稱 | 全名 (Full Name) | 核心用途 (Core Function) | 在作業系統/中斷中的角色 (Role in OS/ISR) |
| :--- | :--- | :--- | :--- |
| **ESP** | **E**xtended **S**tack **P**ointer | **堆疊指標**。始終指向目前堆疊的**頂端 (Top of Stack)**。 | **極度重要！** 它是我們定位 `registers_t` 結構體的基準。透過 `push esp`，我們把這個「地圖位址」傳給 C 語言。 |
| **EBP** | **E**xtended **B**ase **P**ointer | **基底指標**。用於建立函式的「堆疊框架 (Stack Frame)」。 | 在 C 語言函式內，用來存取局部變數 (Local Variables) 與函式參數。 |
| **ESI** | **E**xtended **S**ource **I**ndex | **來源索引**。在字串或記憶體區塊移動時，作為「來源端」位址。 | 在處理 DMA 或記憶體拷貝（如 `memcpy`）時，標記資料從哪裡讀取。 |
| **EDI** | **E**xtended **D**estination **I**ndex | **目的索引**。在字串或記憶體區塊移動時，作為「目的端」位址。 | 在處理記憶體拷貝時，標記資料要寫入到哪裡。 |

#### 3. 隱藏的關鍵暫存器 (Special Purpose Registers)
雖然 `pusha` 不包含它們，但它們在中斷發生時會自動被 CPU 處理，是理解 `iret` 的關鍵。

| 暫存器名稱 | 全名 (Full Name) | 核心用途 (Core Function) | 在作業系統/中斷中的角色 (Role in OS/ISR) |
| :--- | :--- | :--- | :--- |
| **EIP** | **E**xtruction **I**nstruction **P**ointer | **指令指標**。存放「下一條即將執行的指令」位址。 | 當中斷發生時，CPU 會自動將目前的 EIP 壓入堆疊，以便 `iret` 能跳回原位。 |
| **EFLAGS** | **E**xtended **FLAGS** Register | **狀態旗標**。記錄 CPU 的運算狀態（如 Zero, Carry, Interrupt enable）。 | 控制中斷是否開啟 (`cli`/`sti`)。若在 ISR 中不慎修改此值，系統可能會崩潰。 |

---

### 💡 給開發者的開發小筆記 (Pro-Tips)

1.  **為什麼 `pusha` 之後要 `push esp`？**
    因為 `pusha` 把暫存器全部壓入堆疊後，堆疊的頂端剛好就是這組暫存器資料的開頭。我們把 `esp` 傳給 C 語言，C 語言就可以直接用一個 `struct` (例如 `struct registers_t`) 來對應這些位址，非常方便。

2.  **為什麼 `EAX` 是神聖不可侵犯的？**
    在 System Call 機制中，User Mode 程式會把「功能號」放在 `EAX`，而 Kernel 執行完後，必須把「結果」放回 `EAX`。如果在 `popa` 之前，C 語言沒有正確處理 `EAX`，或是被其他指令覆蓋了，User Mode 就會拿到錯誤的結果。

3.  **關於 `ESP` 的變動**
    在寫 ISR 時，要時刻意識到 `ESP` 的移動。每一次 `push` 都會讓 `ESP` 減小，每一次 `pop` 都會讓 `ESP` 增加。如果 `add esp, 4` 少寫了，下次中斷時，堆疊結構就會錯位，導致系統直接進入 **Double Fault** 甚至重啟。
