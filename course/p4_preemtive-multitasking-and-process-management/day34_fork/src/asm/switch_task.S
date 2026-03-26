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

; 【終極降落傘】所有 Ring 3 任務（包含初始 Shell 與 Fork 分身）都從這裡降落
child_ret_stub:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 精準彈出我們在 C 語言中手工對齊的 4 個暫存器
    pop edi
    pop esi
    pop ebx
    pop ebp

    mov eax, 0 ; 【核心魔法】小孩拿到回傳值 0！

    iret       ; 彈出 IRET 幀，完美降落
