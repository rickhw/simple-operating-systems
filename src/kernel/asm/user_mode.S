; [目的] 執行「降權」操作，從 Ring 0 進入 Ring 3
; [流程]
; 1. 設定平民段選擇子 (DS/ES/FS/GS = 0x23)
; 2. 準備 IRET 堆疊幀 (SS, ESP, EFLAGS, CS, EIP)
;    |--> EFLAGS 特別處理：or 0x200 確保進入 User Mode 後「中斷是開啟的」
; 3. 執行 iret：CPU 自動載入暫存器，並跳轉至 entry_point。

global enter_user_mode
enter_user_mode:
    ; 標準的 C 語言函式開頭
    push ebp
    mov ebp, esp

    ; 因為 push 了 ebp，參數的位移要 +4
    mov eax, [ebp+8]  ; 參數 1: entry_point
    mov edx, [ebp+12] ; 參數 2: user_stack_ptr

    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23         ; SS (User Data Segment)
    push edx          ; ESP (給平民專用的堆疊！)
    pushfd            ; EFLAGS Register (Extended Flags)
    pop ebx
    or ebx, 0x200     ; 開啟中斷
    push ebx

    ; 0x1b 的二進位是 ...00011 0 11，最後那兩個 11 就是在告訴 CPU：「等一下 iret 回去後，請進入 Ring 3。」
    push 0x1b         ; CS (User Code Segment)

    push eax          ; EIP
    iret              ; Interrupt Return
