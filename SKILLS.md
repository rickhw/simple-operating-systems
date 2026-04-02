# Operating System Development Skills

This document summarizes the core technical knowledge and skills required to develop and maintain the Simple OS project, focusing on C and x86 Assembly.

## C Programming for Systems

### 1. Freestanding Environment
- **Concept:** Developing without a standard library (libc).
- **Tooling:** Using `-ffreestanding` to tell GCC that the standard library might not exist.
- **Implementation:** Implementing own versions of `memcpy`, `memset`, `strlen`, and `printf` (as `kprintf`).

### 2. Low-Level Memory Management
- **Alignment:** Using `__attribute__((aligned(4096)))` to ensure data structures (like Page Directories) are aligned to page boundaries.
- **Pointers:** Direct manipulation of memory-mapped I/O (MMIO) and physical addresses.
- **Volatile:** Using the `volatile` keyword to prevent the compiler from optimizing out repeated reads/writes to hardware-mapped registers or shared task states.

### 3. Inline Assembly
- **Usage:** Integrating assembly instructions within C code for tasks C cannot perform (e.g., `hlt`, `cli`, `sti`, `invlpg`).
- **Syntax:** Understanding the GCC extended asm syntax: `__asm__ volatile ( "instruction" : output : input : clobber )`.

### 4. Data Structure Layout
- **Registers:** Defining structures like `registers_t` that exactly match the stack layout during an interrupt to allow C handlers to access CPU state.

## x86 Architecture & Assembly

### 1. CPU Initialization & Protection
- **GDT (Global Descriptor Table):** Defining segments for Kernel/User Code and Data. Transitioning from real mode (handled by GRUB) to protected mode.
- **IDT (Interrupt Descriptor Table):** Routing hardware interrupts and software exceptions to the correct handlers.
- **Privilege Levels:** Understanding Ring 0 (Kernel) vs. Ring 3 (User) and how to transition between them using `iret`.

### 2. Memory Management (Paging)
- **Control Registers:**
    - `CR0`: Enabling the Paging bit (PG).
    - `CR3`: Storing the physical address of the current Page Directory.
    - `CR2`: Containing the linear address that caused the last Page Fault.
- **Structures:** Implementing Page Directories (PD) and Page Tables (PT) to map virtual memory to physical memory.

### 3. Multitasking & Context Switching
- **Stack Manipulation:** Saving the state of a running task (registers) onto its kernel stack and restoring the state of the next task.
- **TSS (Task State Segment):** Used primarily to store the kernel stack pointer used during transitions from Ring 3 to Ring 0.

### 4. Hardware Communication (I/O)
- **Port I/O:** Using `in` and `out` instructions (`inb`, `outw`, `inl`, etc.) to communicate with legacy hardware like the Programmable Interrupt Controller (PIC), PS/2 Keyboard, and PCI configuration space.
- **PCI Scanning:** Iterating through the PCI bus to identify and initialize device drivers (like the RTL8139 network card).

## System Architecture Concepts

- **VFS (Virtual File System):** Abstracting different storage backends (like SimpleFS) behind a unified interface.
- **IPC (Inter-Process Communication):** Implementing message queues and shared memory between tasks.
- **GUI Engine:** Managing windows, Z-order (layering), and routing mouse/keyboard events to the correct process.
- **Networking Stack:** Handling Ethernet frames, ARP (Address Resolution Protocol), and ICMP (Ping) packets.
