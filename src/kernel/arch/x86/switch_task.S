; [目的] 進程上下文切換 (Context Switch) 與位址空間切換
; [流程 - switch_task(old_esp, new_esp, next_cr3)]
;
;    Task A Stack (Kernel)             Task B Stack (Kernel)
;    +-------------------+             +-------------------+
;    | 保存 A 現場        |             |                   |
;    | (pusha, pushf)    |             |                   |
;    | [esp] -> old_esp  |             |                   |
;    +---------|---------+             +---------|---------+
;              |                                           A
;              |      [ 切換 ESP & CR3 ]                   |
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

; =========================================================
; 堆疊偏移量計算：
;     pusha (32 bytes) + pushf (4 bytes) + Return EIP (4 bytes) = 40 bytes
;
; 因此參數在堆疊中的偏移為：
;   +40 = 第一個參數 (current_esp)
;   +44 = 第二個參數 (next_esp)
;   +48 = 第三個參數 (next_cr3)
; =========================================================
OFFSET_CURRENT_ESP equ 40
OFFSET_NEXT_ESP    equ 44
OFFSET_NEXT_CR3    equ 48
USER_DATA_SEG      equ 0x23

; extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
switch_task:
    pusha           ; 一次壓入 8 個通用暫存器 (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    pushf           ; 壓入 EFLAGS (包含中斷開關狀態 IF)

    ; 在換掉 ESP 之前，先把第三個參數 (next_cr3) 存到 EDX
    ; 換掉 ESP 之後就再也找不到舊堆疊上的參數了！
    mov edx, [esp + OFFSET_NEXT_CR3]        ; OFFSET = 40(pusha) + 4(pushf) + 4(return EIP) = 48

    ; 把當前 ESP 值存入 prev->esp (第一個參數是指向 prev->esp 的指標)
    mov eax, [esp + OFFSET_CURRENT_ESP]
    mov [eax], esp

    ; 載入 next->esp (切換到新行程的 Kernel Stack)
    mov eax, [esp + OFFSET_NEXT_ESP]
    mov esp, [eax]                          ; ← 這一行過後，我們已經在新行程的堆疊上了

    ; 現在把暫存的 next_cr3 寫入 CR3，切換虛擬位址空間
    mov cr3, edx                            ; ← TLB 全部失效，位址空間正式切換

    popf            ; 恢復新行程的 EFLAGS
    popa            ; 恢復新行程的 8 個通用暫存器

    ret             ; 返回到新行程上次被中斷的位置
                    ; 從堆疊彈出返回位址 (EIP) ，而這個 EIP 是新行程上次被 switch_task 呼叫時壓入的
                    ; ，所以 CPU 就這樣神不知鬼不覺地回到了新行程「上次停下來的地方」，繼續執行。


; ==========================================
; 【終極降落傘】純組合語言實作，不弄髒暫存器！
; ==========================================
child_ret_stub:
    ; 剛從 switch_task 的 ret 跳過來。
    ; 此時 ESP 完美指向我們在 C 語言壓入的 edi！

    ; 1. 精準彈出我們手工對齊的 4 個暫存器
    pop edi       ; 彈出手工放的暫存器初始值
    pop esi
    pop ebx
    pop ebp       ; 這裡拿回了完美修正過的 EBP！

    ; 2. 換上平民服裝 (User Data Segment = 0x23), Ring 3 的資料段
    mov cx, USER_DATA_SEG
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    mov eax, 0    ; [關鍵] 子行程拿到回傳值 0！

    ; 3. 完美降落！
    ; iret：從 Ring 0 跳回 Ring 3
    ; CPU 從 Stack 取出 EIP, CS, EFLAGS, ESP, SS
    ; 完成 Kernel → User Space 的特權降級
    iret
