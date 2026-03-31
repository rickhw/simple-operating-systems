; [目的] 核心進入點與環境初始化 (支援 VESA 圖形模式)
; [來源] 符合 Multiboot 1 規範，讓 GRUB 載入並啟動
; [流程]
;  GRUB 載入器 -> 檢查 Multiboot Header (含 VBE 設定) -> 跳轉至 _start
;                                                         |
;    +----------------------------------------------------+
;    V
; [ _start ]
;    |--> [ 設定 ESP ] (指向 16KB 堆疊頂部：stack_top)
;    |--> [ 壓入參數 ] (eax=Magic, ebx=MBI 指標)
;    |          |----> ebx 內含 VBE 控制資訊與 Framebuffer 位址
;    |--> [ call kernel_main ] (切換至 C 語言環境)
;    |--> [ 核心停止 ] (cli/hlt 無限迴圈，防止 CPU 過熱)

; ==========================================
; Multiboot Header 升級為圖形模式
; ==========================================
MAGIC       equ 0x1BADB002
; Flag Bit 0: Align (對齊)
; Flag Bit 1: MemInfo (記憶體資訊)
; Flag Bit 2: VideoMode (影片模式)  <-- 【新增】我們需要這個！
FLAGS       equ 0x00000007
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    ; AOUT kludge 位址欄位 (不使用，但因為啟用了 Bit 2，必須填 5 個 0 佔位)
    dd 0, 0, 0, 0, 0

    ; 【新增】VBE (VESA BIOS Extensions) 圖形模式設定
    dd 0    ; mode_type: 0 代表線性圖形模式 (Linear Graphics)
    dd 800  ; width: 寬度 800 像素
    dd 600  ; height: 高度 600 像素
    dd 32   ; depth: 色彩深度 32-bit (ARGB)


; === 定義 Multiboot 1 的常數 ===
; MBALIGN  equ  1 << 0            ; 請求 GRUB 將載入的模組對齊 4KB 邊界
; MEMINFO  equ  1 << 1            ; 請求 GRUB 提供記憶體資訊 (包含模組位址)
; MAGIC    equ  0x1BADB002        ; [關鍵] Multiboot 1 的專屬魔法數字
; FLAGS    equ  MBALIGN | MEMINFO ; 組合旗標 (通常是 3)
; CHECKSUM equ -(MAGIC + FLAGS)   ; 檢查碼，證明這是一個合法的標頭

; section .multiboot
; align 4
;     dd MAGIC
;     dd FLAGS
;     dd CHECKSUM

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
    cli             ; Clear Interrupts
.hang:
    hlt             ; https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    jmp .hang
