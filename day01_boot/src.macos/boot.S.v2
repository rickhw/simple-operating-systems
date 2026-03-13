section .multiboot
align 8
header_start:
    dd 0xe85250d6                ; Multiboot2 魔法數字
    dd 0                         ; 架構 0 (代表 i386)
    dd header_end - header_start ; 標頭總長度
    ; 檢查碼 (Checksum) = -(魔法數字 + 架構 + 長度)
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; === 這裡是新增的結束標籤 (End tag) ===
    align 8
    dw 0    ; type = 0
    dw 0    ; flags = 0
    dd 8    ; size = 8
header_end:

section .text
global _start
_start:
    ; VGA 文字模式的記憶體起始位置是 0xb8000
    mov dword [0xb8000], 0x2f4b2f4f

    ; 關閉硬體中斷
    cli

.hang:
    ; 讓 CPU 進入休眠狀態
    hlt
    jmp .hang
