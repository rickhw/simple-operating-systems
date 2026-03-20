; [目的] 設定分頁目錄位址並啟動分頁保護模式
; [流程]
; 1. load_page_directory: 將 Page Dir 位址寫入 CR3 暫存器
; 2. enable_paging:
;    [ 讀取 CR0 ] -> [ bit 31 設為 1 ] -> [ 寫回 CR0 ]
;                                            |
;                                            V
;                                     [ 虛擬記憶體啟動 ]

global load_page_directory
global enable_paging

; void load_page_directory(uint32_t* dir);
load_page_directory:
    mov eax, [esp+4]  ; 取得 C 語言傳進來的 page_directory 陣列位址
    mov cr3, eax      ; 將位址存入 CR3 暫存器
    ret

; void enable_paging(void);
enable_paging:
    mov eax, cr0      ; 讀取目前的 CR0 狀態
    or eax, 0x80000000; 使用 OR 運算，把最高位 (PG bit 31) 強制設為 1
    mov cr0, eax      ; 寫回 CR0，虛擬記憶體機制瞬間啟動！
    ret
