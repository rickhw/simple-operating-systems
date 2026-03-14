global switch_task

; 函式簽名：void switch_task(uint32_t* old_esp_ptr, uint32_t new_esp);
switch_task:
    ; 1. 儲存 C 語言會用到的、但呼叫慣例規定不能弄髒的暫存器 (Callee-saved registers)
    push ebx
    push esi
    push edi
    push ebp

    ; 2. 取出參數
    ; [esp + 20] 是第一個參數 (old_esp_ptr 的位址) (因為前面 push 了 4 個暫存器 + 返回位址 = 20 bytes)
    ; [esp + 24] 是第二個參數 (new_esp 的值)
    mov eax, [esp + 20]

    ; 3. 把現在的 esp 存進第一個參數指到的記憶體 (也就是 task_A->esp)
    mov [eax], esp

    ; ==========================================
    ; 🎩 魔術發生：偷換堆疊！
    ; ==========================================
    mov eax, [esp + 24]
    mov esp, eax        ; 現在開始，我們在 Task B 的世界了！
    ; ==========================================

    ; 4. 從 Task B 的堆疊中，把當初存起來的暫存器彈出來
    pop ebp
    pop edi
    pop esi
    pop ebx

    ; 5. 返回！但注意，因為我們換了堆疊，這個 ret 會彈出 Task B 當初呼叫 switch_task 時的返回位址！
    ret
