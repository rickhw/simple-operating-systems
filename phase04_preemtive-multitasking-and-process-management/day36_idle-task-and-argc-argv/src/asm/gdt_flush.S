; [目的] 套用新的 GDT 配置並更新 CPU 段暫存器
; [流程]
; C 語言呼叫 gdt_flush(ptr)
;    |
;    V
; [ 載入 GDTR ] (使用 lgdt 指令)
;    |
;    |--> [ 更新 Data 段 ] (ds, es, fs, gs, ss 設為 0x10)
;    |--> [ Far Jump ] ----> [ .flush 標籤 ] (更新 CS 為 0x08)
;                               |
;                               V
;                         [ ret 返回 C ]

global gdt_flush

gdt_flush:
    ; 從 C 語言的堆疊中取出 gdt_ptr 的位址
    mov eax, [esp+4]

    ; 載入新的 GDT 到 CPU
    lgdt [eax]

    ; 載入 Data Segment 的偏移量 (0x10 是 GDT 的第 3 個元素：Data Segment)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 載入 Code Segment 的偏移量 (0x08 是 GDT 的第 2 個元素：Code Segment)
    ; 我們必須使用 "Far Jump (遠跳躍)" 來強制 CPU 刷新執行管線並套用新的 cs 暫存器
    jmp 0x08:.flush

.flush:
    ret


global tss_flush

tss_flush:
    mov ax, 0x2B ; 0x28 (第 5 個 GDT) | 3 (Ring 3) = 0x2B
    ltr ax       ; Load Task Register
    ret
