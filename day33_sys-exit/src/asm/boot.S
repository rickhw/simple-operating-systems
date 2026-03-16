; === 定義 Multiboot 1 的常數 ===
MBALIGN  equ  1 << 0            ; 請求 GRUB 將載入的模組對齊 4KB 邊界
MEMINFO  equ  1 << 1            ; 請求 GRUB 提供記憶體資訊 (包含模組位址)
MAGIC    equ  0x1BADB002        ; [關鍵] Multiboot 1 的專屬魔法數字
FLAGS    equ  MBALIGN | MEMINFO ; 組合旗標 (通常是 3)
CHECKSUM equ -(MAGIC + FLAGS)   ; 檢查碼，證明這是一個合法的標頭

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; === 定義未初始化的資料區段 (BSS) 來存放堆疊 ===
section .bss
align 16
stack_bottom:
    resb 16384 ; 保留 16 KB 的空間 (16 * 1024 bytes)
stack_top:

; === 系統進入點 ===
section .text
extern kernel_main

global _start
_start:
    ; 1. 設定堆疊指標
    mov esp, stack_top

    ; 2. 將 GRUB 傳遞的參數壓入堆疊 (Multiboot 1 規範)
    ; eax 會包含 0x2BADB002，ebx 會包含 MBI 結構的位址
    push ebx    ; 參數 2: multiboot_info_t* (MBI 結構指標)
    push eax    ; 參數 1: magic number (魔法數字)

    ; 3. 呼叫 C 語言的進入點
    call kernel_main

    ; 4. 清理堆疊 (兩個參數共 8 bytes)
    add esp, 8

    ; 5. 安全降落區：如果 Kernel 意外結束，就在這裡無限休眠
    cli
.hang:
    hlt
    jmp .hang
