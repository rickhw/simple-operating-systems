[bits 32]
global switch_task

; void switch_task(uint32_t *current_esp, uint32_t *next_esp);
switch_task:
    pusha
    pushf

    ; 【修復 3】精準計算 Stack 偏移量
    ; pusha (32) + pushf (4) + 呼叫時的 ret EIP (4) = 40
    mov eax, [esp + 40]
    mov [eax], esp

    mov eax, [esp + 44]
    mov esp, [eax]

    popf
    popa
    ret
