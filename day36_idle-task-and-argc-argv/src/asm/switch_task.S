; [目的] 進程上下文切換 (Context Switch)
; [流程 - switch_task(old_esp, new_esp)]
;    Current Stack (Task A)         Next Stack (Task B)
;    +-------------------+          +-------------------+
;    | 保存 Task A 現場   |          |                   |
;    | (pusha, pushf)    |          |                   |
;    | [esp] -> old_esp  |          |                   |
;    +---------|---------+          +---------|---------+
;              |                              V
;              +------------------------> [ 載入 new_esp ]
;                                             |
;                                   +---------V---------+
;                                   | 恢復 Task B 現場  |
;                                   | (popf, popa)      |
;                                   +-------------------+

[bits 32]

global switch_task
global child_ret_stub

switch_task:
    pusha
    pushf
    mov eax, [esp + 40]
    mov [eax], esp
    mov eax, [esp + 44]
    mov esp, [eax]
    popf
    popa
    ret

; ==========================================
; 【終極降落傘】純組合語言實作，保證絕不弄髒暫存器！
; ==========================================
child_ret_stub:
    ; 剛從 switch_task 的 ret 跳過來。
    ; 此時 ESP 完美指向我們在 C 語言壓入的 edi！

    ; 1. 精準彈出我們手工對齊的 4 個暫存器
    pop edi
    pop esi
    pop ebx
    pop ebp       ; 這裡拿回了完美修正過的 EBP！

    ; 2. 換上平民服裝 (User Data Segment = 0x23)
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    mov eax, 0    ; 【核心魔法】子行程拿到回傳值 0！

    ; 3. 完美降落！
    ; 此刻 ESP 剛好指著我們手工準備的 IRET 幀 (EIP, CS, EFLAGS, ESP, SS)
    iret
