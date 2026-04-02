

; [Purpose] Kernel Entry Point and environment initialization (VESA support)
; [Flow] GRUB -> _start -> kernel_main

; --- Multiboot 1 Constants ---
MB_MAGIC       equ 0x1BADB002
MB_ALIGN       equ 1 << 0
MB_MEMINFO     equ 1 << 1
MB_VIDEOMODE   equ 1 << 2
MB_FLAGS       equ MB_ALIGN | MB_MEMINFO | MB_VIDEOMODE
MB_CHECKSUM    equ -(MB_MAGIC + MB_FLAGS)

; --- Graphics Settings ---
VBE_MODE_LINEAR equ 0
VBE_WIDTH       equ 800
VBE_HEIGHT      equ 600
VBE_DEPTH       equ 32

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    ; AOUT kludge fields (unused)
    dd 0, 0, 0, 0, 0

    ; VBE (VESA BIOS Extensions) Settings
    dd VBE_MODE_LINEAR
    dd VBE_WIDTH
    dd VBE_HEIGHT
    dd VBE_DEPTH

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB stack
stack_top:

section .text
extern kernel_main
global _start

_start:
    ; 1. Setup stack
    mov esp, stack_top

    ; 2. Push Multiboot parameters (EAX=Magic, EBX=MBI pointer)
    push ebx
    push eax

    ; 3. Call Kernel Entry Point
    call kernel_main

    ; 4. If kernel returns, hang
    cli
.hang:
    hlt
    jmp .hang
