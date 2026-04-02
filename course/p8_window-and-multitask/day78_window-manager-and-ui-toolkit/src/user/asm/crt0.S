; [目的] C Runtime Zero - 使用者程式的最後一道防線
; [堆疊初始狀態 (自低往高)]
;  [esp+0]: 假返回位址
;  [esp+4]: argc (參數數量)
;  [esp+8]: argv (參數陣列指標)
; [流程]
;  1. xor ebp, ebp (終止 Stack Trace)
;  2. 提取 argc, argv 並壓入堆疊
;  3. call main (執行 App)
;  4. 若 main 返回 -> mov eax, 7 (sys_exit) -> int 0x80

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
    mov eax, [esp + 4]  ; eax = argc
    mov edx, [esp + 8]  ; edx = argv

    ; 將參數壓入堆疊，呼叫 main
    push edx            ; 參數 2: argv
    push eax            ; 參數 1: argc
    call main

    ; 如果 main 結束了，我們主動呼叫 sys_exit (Syscall 7) 來結束生命
    ; 這樣就不需要依賴堆疊裡的假返回位址了！
    mov eax, 7          ; Syscall 7 = sys_exit
    int 0x80

    ; 永遠不該執行到這裡
.hang:
    jmp .hang
