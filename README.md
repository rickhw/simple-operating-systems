
# Simple Operating System

A 32-bit x86 operating system built from scratch in C and NASM assembly — bootloader to GUI to TCP/IP stack.

## Blog & Demo

- [自幹作業系統 - Simple OS](https://rickhw.github.io/2026/03/27/ComputerScience/Simple-OS/)
- [自幹作業系統 - Networking Fundamentals](https://rickhw.github.io/2026/04/09/ComputerScience/Simple-OS-Networking/)
- [自幹作業系統 - 初探 Timer 原理](https://rickhw.github.io/2026/04/12/ComputerScience/Simple-OS-Timer/)
- [自幹作業系統 - Task and Scheduler](https://rickhw.github.io/2026/04/17/ComputerScience/Simple-OS-Task-and-Scheduler/)
- [Demo / 錄影](https://www.youtube.com/playlist?list=PL63J1r2PBvojs-N7OLM6jo6IYKEIizK1P)


## What's Implemented

| Area | Details |
|------|---------|
| Boot | GRUB → boot.S → main.c, GDT/IDT, IRQ handling |
| Memory | Physical memory manager, kernel heap, MMU paging |
| Multitasking | PIT timer, context switch, round-robin scheduler |
| Processes | `fork` / `exec` / `wait` / `exit`, Ring 0↔3 transition, IPC, Mutex |
| File System | ATA driver, MBR parser, SimpleFS, VFS, ELF loader |
| Shell & CLI | `shell`, `ls`, `cat`, `touch`, `mkdir`, `rm`, `ps`, `top`, `kill`, `ping` |
| GUI | VESA framebuffer, window manager, double buffering, PS/2 mouse/keyboard |
| Window Apps | clock, notepad, paint, image viewer, file explorer, task manager, tic-tac-toe |
| Networking | RTL8139 driver, Ethernet, ARP, IPv4, ICMP, UDP, TCP, DNS, HTTP, `wget` |
| User libc | `malloc` / `free` / `printf`, syscall wrappers, `crt0.S` |

The final kernel binary (`myos.bin`) is ~62 KB.


## Quick Start

Requires Docker (macOS: [OrbStack](https://orbstack.dev)) and QEMU.

```bash
make build-env   # Build the Docker cross-compile image (one-time)
make all         # Compile kernel + user apps → myos.iso
make run         # Boot in QEMU
make debug       # Boot with interrupt logging (-d int -no-reboot)
make clean       # Remove build artifacts
```


## Architecture

**Boot sequence:** GRUB → `src/kernel/arch/x86/boot.S` → `src/kernel/init/main.c`

```
src/
├── kernel/
│   ├── arch/x86/    # Boot, GDT/IDT, paging, PIT, context switch
│   ├── mm/          # Physical memory (pmm.c), kernel heap (kheap.c)
│   ├── kernel/      # Scheduler, task lifecycle, syscall dispatch, GUI/WM
│   ├── fs/          # VFS, SimpleFS, ELF loader, ATA, MBR
│   ├── drivers/     # VESA, TTY, PS/2 keyboard/mouse, RTC, PCI, RTL8139
│   ├── net/         # Ethernet → ARP → IPv4 → ICMP/UDP/TCP
│   ├── lib/         # Kernel printf, string/memory utilities
│   └── include/     # All kernel headers
└── user/
    ├── bin/         # ~29 standalone ELF applications
    ├── lib/         # Minimal libc
    └── asm/         # crt0.S (user-space C runtime entry)
```

- Kernel linked at physical base `0x100000` via `scripts/linker.ld`
- Syscalls via `int 0x80`, dispatched in `src/kernel/kernel/syscall.c`
- User processes run in Ring 3; each `.c` in `src/user/bin/` becomes a standalone ELF


## Prerequisites

- Basic C and x86 assembly
- Familiarity with computer organization concepts
- macOS or Ubuntu 24.04
- Docker (OrbStack recommended on macOS) + QEMU
