[bits 32]
global switch_task
global task_return_stub  ; [新增] 暴露給 C 語言使用

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

; [新增] 新任務的第一個起點！
; 當 switch_task 執行 ret 後，新任務會跳來這裡
; 這裡的 Stack 狀態，被我們偽造得跟 ISR 準備結束時一模一樣！
task_return_stub:
    popa            ; 恢復 8 個通用暫存器
    add esp, 8      ; 跳過 Error Code 和 Int Number (各 4 bytes)
    iret            ; 完美降級至 Ring 3！
