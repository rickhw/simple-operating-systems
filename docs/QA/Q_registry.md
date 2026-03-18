# Q

我看到其他 asm 裡有更多其他暫存器，這些暫存器是根據不同的 CPU 晶片而定義？我要怎麼知道有哪一些暫存器可以用？寫應用程式，會有完整的 API 可以查，但我在反覆看 asm 會發現，老師給的範例，會出現很多我完全看不懂的暫存器，像是 user_mode.S 裡面看起來又出現其他的 暫存器？

```asm "user_mode.S"
global enter_user_mode
enter_user_mode:
    ; 標準的 C 語言函式開頭
    push ebp
    mov ebp, esp

    ; 因為 push 了 ebp，參數的位移要 +4
    mov eax, [ebp+8]  ; 參數 1: entry_point
    mov edx, [ebp+12] ; 參數 2: user_stack_ptr

    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23         ; SS (User Data Segment)
    push edx          ; ESP (給平民專用的堆疊！)
    pushfd
    pop ebx
    or ebx, 0x200     ; 開啟中斷
    push ebx
    push 0x1b         ; CS (User Code Segment)
    push eax          ; EIP
    iret
```


---

## A

這是一個非常關鍵的轉折點！當你從 `boot.S` 進到 `user_mode.S` 時，你其實是從「純運算」跨入了「CPU 架構管理」。

要回答你的問題：**這些暫存器不是根據不同晶片（如 Intel 或 AMD）隨機定義的，而是根據 CPU 的「指令集架構 (ISA)」定義的。** 在 OS 開發中，你主要面對的是 **x86 (IA-32)** 架構。

之所以會覺得「看不懂」，是因為 x86 的暫存器其實分為好幾類。我們來拆解 `user_mode.S` 裡出現的新面孔：

---

## 1. 段暫存器 (Segment Registers): `ds`, `es`, `fs`, `gs`, `cs`, `ss`

這就是你在 `user_mode.S` 看到 `mov cx, 0x23` 後面跟著的那一串。

* **全名：** Data Segment (DS), Extra Segment (ES), FS, GS (無特定全名), Code Segment (CS), Stack Segment (SS)。
* **用途：** 在 32 位元保護模式下，它們不是用來存數值的，而是用來存 **「選擇子 (Selector)」**。
* **為什麼要在這裡切換？** 因為你要從 **Kernel Mode (Ring 0)** 進入 **User Mode (Ring 3)**。
    * 在作業系統中，核心用的記憶體權限跟平民（User）用的不同。
    * `0x23` 和 `0x1b` 這些神祕數字，其實是 **GDT (全域描述符表)** 裡的索引，尾數代表了「我要切換到平民權限」。

### 2. 標誌暫存器與特殊指令：`eflags` (透過 `pushfd`)

* **全名：** EFLAGS Register (Extended Flags)。
* **用途：** 紀錄 CPU 目前的狀態（例如：剛才運算有沒有溢位、是否允許硬體中斷）。
* **程式碼解析：**
    ```asm
    pushfd      ; 將 EFLAGS 暫存器的內容壓入堆疊
    pop ebx     ; 彈出到 ebx，讓我們可以修改它
    or ebx, 0x200 ; 將第 9 位元 (IF, Interrupt Flag) 設為 1
    push ebx    ; 再壓回去，準備給 iret 使用
    ```
    這段動作是為了確保進入 User Mode 後，**中斷是開啟的**，否則你的電腦會因為收不到時鐘中斷而「凍結」。

---

## 3. 暫存器的分類表：你要去哪裡查？

在開發 OS 時，你不需要像查 API 一樣查幾千個函式，你只需要查 **"Intel 64 and IA-32 Architectures Software Developer's Manual"**（簡稱 Intel Manual）。



通常我們會把它們分成這幾箱：

| 類別 | 暫存器名稱 | 你的進度何時會用到？ |
| :--- | :--- | :--- |
| **通用 (GPRs)** | `eax`, `ebx`, `ecx`, `edx`, `esi`, `edi` | 任何時候（加減乘除、傳參數）。 |
| **指標 (Pointers)** | `esp` (堆疊頂), `ebp` (基底) | 處理函式呼叫、區域變數時。 |
| **段 (Segments)** | `cs`, `ds`, `ss`, `es`, `fs`, `gs` | **切換權限 (User/Kernel)**、設定記憶體保護時。 |
| **控制 (Control)** | `cr0`, `cr2`, `cr3`, `cr4` | **開啟分頁 (Paging)**、虛擬記憶體管理時。 |
| **系統標誌** | `eflags` | 控制中斷開關、比較運算結果。 |

---

## 4. 關鍵指令：`iret` (Interrupt Return)

這是 `user_mode.S` 最魔幻的地方。`iret` 原本是用來「從中斷返回」的，但 OS 開發者有個技巧：
**我們偽造一個中斷現場，然後用 `iret` 騙 CPU 跳過去。**

1.  我們手動 `push` 了 `SS`, `ESP`, `EFLAGS`, `CS`, `EIP` 到堆疊裡。
2.  執行 `iret` 時，CPU 會以為自己剛處理完一個中斷，自動從堆疊彈出這五個值並填入對應的暫存器。
3.  結果：CPU 瞬間切換了權限（從 CS/SS 切換），並跳到了你指定的 `entry_point` (EIP)。

---

## 總結：我要怎麼知道有什麼可以用？

1.  **大部分時間**：你只會用到 `eax`, `ebx`, `ecx`, `edx` 和 `esp`。
2.  **特定的系統任務**：
    * 看到 `ds`, `es` 等 -> 你在處理 **記憶體分段/權限**。
    * 看到 `cr3` -> 你在處理 **虛擬記憶體/分頁 (Paging)**。
    * 看到 `iret` -> 你在做 **任務切換 (Context Switch)**。

**你現在看到的這些「奇怪暫存器」，大多是為了達成「權限隔離」這個目的。需要我幫您把 `0x23` 或 `0x1b` 這種數值是怎麼算出來的（GDT 結構）再複習一次嗎？**
