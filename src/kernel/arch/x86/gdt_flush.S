

; [目的] 套用新的 Global Descriptor Table 配置並載入任務暫存器 (TR)
; [流程]
; 1. gdt_flush:
;    [ 載入 Global Descriptor TableR ] -> [ 更新 DS/ES/SS... ] -> [ Far Jump 刷新 CS ]
; 2. tss_flush:
;    [ 載入 Selector 0x2B 到 TR ]
;    |--> 目的：告訴 CPU 當 User Mode 發生中斷時，去哪裡找「核心堆疊位址」。
;
; [Global Descriptor Table 佈局回顧]
; 0x00: Null | 0x08: K-Code | 0x10: K-Data | 0x18: U-Code | 0x20: U-Data | 0x28: TSS

global gdt_flush
global tss_flush

gdt_flush:
    ; 從 C 語言的堆疊中取出 gdt_ptr 的位址
    mov eax, [esp+4]

    ; 載入新的 Global Descriptor Table 到 CPU
    lgdt [eax]

    ; 載入 Data Segment 的偏移量 (0x10 是 Global Descriptor Table 的第 3 個元素：Data Segment)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 載入 Code Segment 的偏移量 (0x08 是 Global Descriptor Table 的第 2 個元素：Code Segment)
    ; 我們必須使用 "Far Jump (遠跳躍)" 來強制 CPU 刷新執行管線並套用新的 cs 暫存器
    jmp 0x08:.flush

.flush:
    ret

tss_flush:
    mov ax, 0x2B ; 0x28 (第 5 個 Global Descriptor Table) | 3 (Ring 3) = 0x2B
    ltr ax       ; Load Task Register
    ret
