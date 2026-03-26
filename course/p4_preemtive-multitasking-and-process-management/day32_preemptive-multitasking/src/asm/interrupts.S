; interrupts.S
global idt_flush

global isr0
extern isr0_handler ; 告訴組譯器，這個函式在 C 語言裡

global isr32
extern timer_handler

global isr33
extern keyboard_handler

global isr128
extern syscall_handler


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


; 第 32 號中斷 (Timer IRQ 0)
isr32:
    pusha
    call timer_handler
    popa
    iret

; 第 33 號中斷 (鍵盤)
isr33:
    ; 備份所有通用暫存器 (這是一個好習慣，避免中斷處理程式弄髒了原本正在執行的程式的狀態)
    pusha

    call keyboard_handler

    ; 恢復所有通用暫存器
    popa

    iret ; 中斷返回


; 第 128 號中斷 (System Call 窗口)
isr128:
    ; 按照 C 語言 (cdecl) 的呼叫慣例，參數是【從右到左】 push 進堆疊的。
    ; 我們的 C 語言函式長這樣：
    ; void syscall_handler(edi, esi, ebp, esp, ebx, edx, ecx, eax);

    push eax
    push ecx
    push edx
    push ebx
    push esp
    push ebp
    push esi
    push edi

    ; 呼叫 C 語言的處理中心
    call syscall_handler

    ; 清理堆疊！我們剛才 push 了 8 個 32-bit (4 bytes) 的暫存器
    ; 8 * 4 = 32 bytes。所以把 esp 加上 32，丟棄剛才 push 的參數
    add esp, 32

    ; ！！！關鍵魔法！！！
    ; 這裡我們【絕對不能】使用 popa 來恢復暫存器！
    iret
