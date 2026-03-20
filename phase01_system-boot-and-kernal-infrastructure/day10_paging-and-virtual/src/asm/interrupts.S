; interrupts.S
global idt_flush
global isr0
extern isr0_handler ; 告訴組譯器，這個函式在 C 語言裡
global isr33
extern keyboard_handler

; 載入 IDT (跟昨天的 lgdt 幾乎一樣)
idt_flush:
    mov eax, [esp+4]  ; 取出 C 語言傳進來的 idt_ptr 位址
    lidt [eax]        ; Load IDT
    ret

; 第 0 號中斷的跳板程式 (除以零)
isr0:
    ; 關閉中斷，避免在處理錯誤時又被其他中斷打擾
    cli

    ; 呼叫我們在 idt.c 裡寫的 C 語言處理函式
    call isr0_handler

    ; 恢復中斷並返回 (但因為我們在 handler 裡寫了 hlt，其實不會走到這裡)
    sti
    iret

; 第 33 號中斷 (鍵盤)
isr33:
    ; 備份所有通用暫存器 (這是一個好習慣，避免中斷處理程式弄髒了原本正在執行的程式的狀態)
    pusha

    call keyboard_handler

    ; 恢復所有通用暫存器
    popa

    iret ; 中斷返回
