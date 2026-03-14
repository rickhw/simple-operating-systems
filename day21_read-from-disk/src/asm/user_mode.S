global enter_user_mode

; void enter_user_mode(uint32_t user_function_ptr);
enter_user_mode:
    cli                   ; 關閉中斷，避免切換到一半被打斷
    mov ecx, [esp+4]      ; 取得 C 語言傳進來的目標程式位址

    ; 設定 Ring 3 的 Data Segment (第 4 個 GDT: 0x20 | 3 = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; === 偽造 iret 所需的 Stack ===
    mov eax, esp          ; [偷懶設計] 我們直接借用目前的 Stack 指標當作 User Stack

    push 0x23             ; 1. 壓入 SS (User Data Segment)
    push eax              ; 2. 壓入 ESP (User Stack Pointer)
    pushf                 ; 3. 壓入 EFLAGS

    ; 我們必須把 EFLAGS 裡面的 IF (Interrupt Flag) 設為 1，
    ; 這樣回到 User Mode 時才會繼續接聽 Timer 和鍵盤的中斷！
    pop eax
    or eax, 0x200         ; IF flag 是第 9 個 bit (0x200)
    push eax

    push 0x1B             ; 4. 壓入 CS (User Code Segment: 第 3 個 GDT 0x18 | 3 = 0x1B)
    push ecx              ; 5. 壓入 EIP (要跳去執行的函式位址)

    ; 魔術發生的瞬間：CPU 會從堆疊彈出這 5 個值，自動切換到 Ring 3 並跳轉到 ecx！
    iret
