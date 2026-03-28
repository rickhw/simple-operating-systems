這份 Log 是 QEMU 透過 `-d int` 參數輸出的 **CPU 暫存器快照與中斷日誌**。在沒有 `printf` 的 OS 開發初期，這就是你唯一的「黑盒子解碼器」。

解讀 Core Dump 的核心邏輯是：**找出發生問題的瞬間，CPU 正停在哪個位址（EIP），當時處於什麼狀態（Ring 0/3），以及正在處理什麼中斷（v=??）。**

---

## 1. 核心指標：發生問題的位置

當系統崩潰或觸發異常時，最先看這一行：

`0: v=20 e=0000 i=0 cpl=0 IP=0008:001002a1 pc=001002a1 SP=0010:00109d38`

* **`v=20` (Vector)**: 這是發生的中斷向量號（十六進位）。
    * `0x20` 是 **Timer Interrupt**（你設定的 IRQ0）。
    * 如果是 `0x0D` 代表 **General Protection Fault**（通常是記憶體越權）。
    * 如果是 `0x0E` 代表 **Page Fault**（存取了未映射的虛擬位址）。
* **`cpl=0` (Current Privilege Level)**:
    * `0` 代表發生時 CPU 處於 **Kernel Mode (Ring 0)**。
    * `3` 代表發生時處於 **User Mode (Ring 3)**。
* **`pc=001002a1` (Program Counter)**:
    * 這是最重要的資訊！它告訴你當下 CPU 正在執行哪一行指令。
    * **除錯動作**：去查你的 `myos.map` 檔案或對 `myos.bin` 進行反組譯 (`objdump -D`)，尋找這個位址對應到哪個 C 函式。

---

## 2. 暫存器快照：當時發生了什麼

在 `v=20` 下方的暫存器堆疊（Registers Dump），可以幫你還原現場細節：

### A. 通用暫存器 (GPRs)
`EAX=ffffff50 EBX=00105084 ...`
* 檢查 `EAX` 是否為預期回傳值。
* 檢查 `EBX`, `ESI`, `EDI` 是否指向正確的記憶體緩衝區。

### B. 段暫存器與權限 (Segment Registers)
`CS =0008 ... DPL=0`
`SS =0010 ... DPL=0`
* **`CS=0008`**: 這是 GDT 中的第 1 個索引（Kernel Code）。
* **`DPL=0`**: 再次確認這是核心特權級。

### C. 控制暫存器 (Control Registers) —— 虛擬記憶體除錯關鍵
`CR0=80000011 CR2=00000000 CR3=0010f000`
* **`CR0`**: `80000000` 位元若為 1，代表分頁啟動。
* **`CR2`**: 如果發生 **Page Fault (v=0E)**，這裡會存放「到底是哪個記憶體位址存取失敗」。
* **`CR3`**: 存放當前 Page Directory 的物理位址。檢查切換任務後 `CR3` 是否如期變動。



---

## 3. 特殊標記與日誌流程

* **`SMM: enter`**: 這是 System Management Mode，通常是 BIOS 在背景處理電源管理，OS 開發初期可先忽略。
* **`Servicing hardware INT=0x20`**:
    代表硬體時鐘發出了信號。如果你在 Log 看到一長串連續的 `0x20`，代表你的 Scheduler 正在頻繁切換。
* **`TR =002b ... DPL=3`**:
    * 這是 Task Register。
    * `0x2B` 代表指向你的 TSS 段。
    * 這很重要！如果從 User Mode 切回 Kernel Mode 崩潰（Triple Fault），通常是因為 TSS 設定錯誤。

---

## 4. 實戰除錯步驟建議

當你拿到這坨 Log，請按此順序操作：

1.  **定位向量**：看 `v=??`。是正常的時鐘中斷（20），還是報錯（0D/0E）？
2.  **定位程式碼**：拿起 `pc=00101870`。
    * 指令：`nm myos.bin | sort` 或 `readelf -s myos.bin`。
    * 看看這個位址落在哪個函式的範圍內。
3.  **檢查堆疊 (SP)**：
    * `SP=0010:00109fc8`。
    * 如果 `SP` 指向了一個非常奇怪的位址（例如 `0x00000000`），那代表你的 **Stack Overflow** 了，或是 `switch_task` 切換堆疊指標時切歪了。
4.  **檢查權限**：
    * 如果你本來要在 User Mode 跑，結果 `cpl=0`，代表你的 `iret` 沒成功。

---

### 總結

這份 Log 顯示你的系統正處於**分頁開啟 (CR0=80...)** 且**正在處理多工切換 (v=20)** 的狀態。暫存器 `EAX=c0000030` 等數值看起來像是正在處理核心的高位址記憶體（Higher Half Kernel）。

**需要我幫你針對 `0x0D` (General Protection Fault) 或 `0x0E` (Page Fault) 這兩種最常見的崩潰 Log 做更深入的對比說明嗎？**
