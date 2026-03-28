; [目的] 啟動 MMU 分頁機制 (Paging)
; [暫存器說明]
;  CR3: 存放「頁目錄 (Page Directory)」的物理位址。
;  CR0: Bit 31 (PG) 為分頁開關。
; [流程]
; load_page_directory(dir_addr) --> 把地圖交給 CPU (mov cr3, eax)
; enable_paging()               --> 把開關打開 (or eax, 0x80000000)

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
