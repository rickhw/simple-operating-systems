; 1. 手動建立一個 Ring 3 的環境。
; 2. 確保平民有自己的堆疊（這就是為什麼你要 push edx 作為 ESP）。
; 3. 確保平民不能亂搞（透過 GDT 描述符設定 DPL=3）。
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
