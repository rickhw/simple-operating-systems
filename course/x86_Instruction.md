## x86 核心開發指令集彙整表 (32-bit)

### 1. 基礎資料傳送與運算
| 指令 | 全名 | 功能說明 | 在 OS 中的典型用途 |
| :--- | :--- | :--- | :--- |
| **MOV** | Move | 將來源操作數複製到目的操作數 | 初始化暫存器、從堆疊讀取參數。 |
| **OR** | Logical Inclusive OR | 執行位元「或」運算 | 開啟特定控制位（如開啟分頁或開啟中斷）。 |
| **XOR** | Logical Exclusive OR | 執行位元「互斥或」運算 | `xor ebp, ebp` 用於將暫存器清零，這比 `mov` 快。 |
| **ADD** | Add | 兩數相加 | 函式返回後調整 `esp` 以清理堆疊。 |

### 2. 流程控制 (Flow Control)
| 指令 | 全名 | 功能說明 | 在 OS 中的典型用途 |
| :--- | :--- | :--- | :--- |
| **CALL** | Call Procedure | 呼叫函式，將返回位址壓入堆疊並跳轉 | 進入 C 語言 `kernel_main` 或 `syscall_handler`。 |
| **RET** | Return | 從函式返回，彈出返回位址到 EIP | 結束彙編子程式，回到呼叫者。 |
| **JMP** | Jump | 無條件跳轉至目標位址 | `jmp 0x08:.flush` 執行遠跳轉以更新代碼段暫存器。 |
| **IRET** | Interrupt Return | 從中斷返回（彈出 EIP, CS, EFLAGS, ESP, SS） | **切換權限的核心指令**。用於從核心返回應用程式。 |

### 3. 堆疊操作 (Stack Operations)
| 指令 | 全名 | 功能說明 | 在 OS 中的典型用途 |
| :--- | :--- | :--- | :--- |
| **PUSH** | Push onto Stack | 將數值壓入堆疊頂端，ESP 減 4 | 傳遞函式參數。 |
| **POP** | Pop from Stack | 將堆疊頂端數值彈出至暫存器，ESP 加 4 | 恢復暫存器狀態。 |
| **PUSHA** | Push All | 一次壓入所有通用暫存器 (EAX...EDI) | 中斷發生時備份整個 CPU 現場。 |
| **POPA** | Pop All | 一次彈出所有通用暫存器 | 恢復現場，準備執行任務。 |
| **PUSHFD** | Push EFLAGS | 將標誌暫存器壓入堆疊 | 修改中斷旗標（IF）前備份原狀態。 |

### 4. 系統與 CPU 特權指令 (Privileged)
| 指令 | 全名 | 功能說明 | 在 OS 中的典型用途 |
| :--- | :--- | :--- | :--- |
| **LGDT** | Load GDT Register | 載入全域描述符表位址到 GDTR | 初始化記憶體分段模型。 |
| **LIDT** | Load IDT Register | 載入中斷描述符表位址到 IDTR | 讓 CPU 知道中斷處理程式在哪裡。 |
| **LTR** | Load Task Register | 載入任務暫存器（指向 TSS） | 用於硬體任務切換，讓 Ring 3 能切回 Ring 0。 |
| **CLI** | Clear Interrupts | 關閉外部硬體中斷 (IF=0) | 進入臨界區，防止執行過程被干擾。 |
| **STI** | Set Interrupts | 開啟外部硬體中斷 (IF=1) | 準備好接收時鐘或鍵盤訊號。 |
| **HLT** | Halt | 讓 CPU 進入休眠直到下一個中斷 | 防止 CPU 閒置時空轉發熱（省電模式）。 |
| **INT** | Interrupt | 觸發軟體中斷 | `int 0x80`：應用程式呼叫系統服務的窗口。 |

---

## NASM 組譯器指示字 (Directives)
這些不是 CPU 指令，而是告訴組譯器如何安排檔案內容：

* **`equ`**: 定義常數（類似 `#define`）。
* **`dd` / `dw` / `db`**: 分別定義 32-bit / 16-bit / 8-bit 的資料。
* **`resb`**: 保留（Reserve）位元組空間，不佔用二進位檔體積，僅在執行時分配空間（常用於堆疊）。
* **`align`**: 位址對齊，提升 CPU 存取效率或符合硬體規範（如 Multiboot 需要 4 位元組對齊）。
* **`global` / `extern`**: 宣告符號為全域可見，或引用外部（如 C 語言）定義的符號。
