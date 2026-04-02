

; [目的] 進程上下文切換 (Context Switch) 與位址空間切換
; [流程 - switch_task(old_esp, new_esp, next_cr3)]
;
;    Task A Stack (Kernel)             Task B Stack (Kernel)
;    +-------------------+             +-------------------+
;    | 保存 A 現場        |             |                   |
;    | (pusha, pushf)    |             |                   |
;    | [esp] -> old_esp  |             |                   |
;    +---------|---------+             +---------|---------+
;              |                                 A
;              |      [ 切換 ESP & CR3 ]         |
;              +---------------------------------+
;              |--> mov edx, [esp+48] (取得 next_cr3)
;              |--> mov esp, [next_esp] (換堆疊)
;              |--> mov cr3, edx (換位址空間)
;                                                |
;                                      +---------V---------+
;                                      | 恢復 B 現場        |
;                                      | (popf, popa)      |
;                                      +-------------------+
[bits 32]

global switch_task
global child_ret_stub

; extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
switch_task:
    pusha
    pushf

    ; =========================================================
    ; 【關鍵修復：拯救 CR3】
    ; 必須在修改 ESP「之前」，先從舊堆疊拿出第三個參數 next_cr3
    ; 我們把它暫存在 EDX 裡 (稍後 popa 會從新堆疊覆蓋掉 EDX，所以借放一下非常安全)
    ; =========================================================
    mov edx, [esp + 48]

    ; 保存 current_esp
    mov eax, [esp + 40]
    mov [eax], esp

    ; 載入 next_esp (這一步之後，我們就正式跨入新任務的堆疊了！)
    mov eax, [esp + 44]
    mov esp, [eax]

    ; 【跨越宇宙】堆疊切換完成，現在將剛才暫存的 CR3 寫入硬體！
    mov cr3, edx

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
