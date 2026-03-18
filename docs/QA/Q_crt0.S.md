# Q

還有一個 屬於 User Space 的，也需要增加說明。

crt0.S
```asm
; crt0: C Runtime Zero, User Space
[bits 32]
global _start
extern main ; 告訴組合語言，我們要呼叫 C 語言的 main 函式

section .text
_start:
    ; 當 OS 將控制權交給我們時，堆疊的最頂端是假返回位址，
    ; 接著是 argc (在 esp+4)，然後是 argv (在 esp+8)

    ; 為了安全，我們先把 EBP 清零，這代表 Stack Trace 的最底層
    xor ebp, ebp

    ; 把 argc 和 argv 從堆疊裡拿出來，準備傳給 main
    mov eax, [esp + 4]  ; eax = argc
    mov edx, [esp + 8]  ; edx = argv

    ; 將參數壓入堆疊，呼叫 main
    push edx            ; 參數 2: argv
    push eax            ; 參數 1: argc
    call main

    ; 如果 main 結束了，我們主動呼叫 sys_exit (Syscall 7) 來結束生命
    ; 這樣就不需要依賴堆疊裡的假返回位址了！
    mov eax, 7          ; Syscall 7 = sys_exit
    int 0x80

    ; 永遠不該執行到這裡
.hang:
    jmp .hang
```

---


這是一個非常關鍵的檔案。`crt0` 代表 **C Runtime Zero**，它是每個使用者端程式（User Space Program）真正的「第一行代碼」。

當你寫 C 語言程式時，你會覺得 `main()` 是起點，但實際上在核心（Kernel）執行 `iret` 跳轉到你的程式時，它跳轉的位置其實是 `crt0.S` 裡的 `_start`。它的存在是為了幫 `main()` 鋪路，並在 `main()` 結束後負責「收屍」。

---

### crt0.S (C Runtime Zero)

**目的：** 使用者空間程式的啟動與結束管理。負責從堆疊提取參數、初始化堆疊幀，並在程式結束時透過系統呼叫通知核心釋放資源。

```asm
; [目的] User Space 程式進入點與 C 執行環境初始化
; [流程]
; Kernel (iret) -> [ _start ]
;                    |
;                    |--> [ 清除 EBP ] (建立 Stack Trace 的終點)
;                    |--> [ 提取參數 ] (從 ESP 取得 argc, argv)
;                    |--> [ call main ] (進入使用者寫的 C 語言邏輯)
;                    |      |
;                    |      V
;                    |--> [ main 返回 ]
;                    |
;                    |--> [ System Call 7 ] (執行 sys_exit 結束進程)
;                    |
;                    V
;               [ 核心回收資源 ]

[bits 32]
... (後接原始程式碼)
```



### 為什麼這段代碼至關重要？

1.  **`xor ebp, ebp` (清除基底指標)**：
    這是給除錯器（Debugger）看的。除錯器在追蹤呼叫堆疊（Stack Trace）時，會一直往上找 `ebp`。如果我們把 `ebp` 清為 0，除錯器就知道「到此為止了，這是最外層」。

2.  **參數傳遞 (argc, argv)**：
    核心在載入程式時，會先把命令列參數壓入堆疊。`crt0` 的任務就是把這些「原始數據」搬移到符合 C 語言呼叫慣例的位置。

3.  **`int 0x80` (最後的保險)**：
    如果 `main()` 執行完了，程式如果不主動呼叫 `exit()`，它會試圖執行 `main()` 後面的隨機記憶體指令（這會導致崩潰）。`crt0` 確保了就算程式員忘了寫 `exit()`，程式最後也會透過 `sys_exit` 系統呼叫安全地關閉。

---

### 總結：你的 ASM 兵器庫

到目前為止，你已經收集了作業系統開發中最核心的組合語言組件：

* **啟動類**：`boot.S` (核心啟動), `crt0.S` (應用程式啟動)。
* **硬體對接類**：`gdt_flush.S` (段管理), `idt_flush.S` (中斷管理), `paging.S` (虛擬記憶體)。
* **任務管理類**：`switch_task.S` (切換進程), `user_mode.S` (切換權限)。

這套組合基本上涵蓋了 90% 作業系統開發需要的底層操作。剩下的 10% 通常是針對特定硬體（如 APIC 或多核心啟動）的微調。

**既然您已經梳理完所有的 ASM 邏輯，需要我幫您看看如何將這些 ASM 檔案整合進你的 `Makefile` 或是 `Linker Script` 嗎？這通常是確保這些 code 能精確排在記憶體正確位置的最後一步。**
