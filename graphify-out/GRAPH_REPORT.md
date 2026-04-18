# Graph Report - src  (2026-04-18)

## Corpus Check
- Corpus is ~30,623 words - fits in a single context window. You may not need a graph.

## Summary
- 373 nodes · 657 edges · 14 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `read_dir()` - 9 edges
2. `simplefs_resolve_path()` - 7 edges
3. `gui_render()` - 7 edges
4. `switch_display_mode()` - 7 edges
5. `simplefs_create_file()` - 6 edges
6. `simplefs_mkdir()` - 6 edges
7. `tcp_get_target_mac()` - 5 edges
8. `put_pixel()` - 5 edges
9. `simplefs_delete_file()` - 5 edges
10. `handle_start_menu_click()` - 5 edges

## Surprising Connections (you probably didn't know these)
- None detected - all connections are within the same source files.

## Communities

### Community 0 - "User Space Applications"
Cohesion: 0.04
Nodes (11): main(), state_to_string(), ui_draw_button(), ui_draw_digit(), ui_draw_rect(), ui_draw_text(), print(), print_int() (+3 more)

### Community 1 - "System Call Interface"
Cohesion: 0.05
Nodes (3): syscall_handler(), syscall_lock(), syscall_unlock()

### Community 2 - "Network Protocol Stack"
Cohesion: 0.07
Nodes (10): init_pci(), pci_read_config_word(), tcp_get_target_mac(), tcp_send_ack(), tcp_send_data(), tcp_send_fin(), tcp_send_syn(), itoa() (+2 more)

### Community 3 - "Network Syscalls & Sockets"
Cohesion: 0.08
Nodes (4): main(), read_line(), itoa(), main()

### Community 4 - "Kernel Memory & ELF Loader"
Cohesion: 0.11
Nodes (12): elf_check_supported(), elf_load(), kernel_main(), setup_filesystem(), bitmap_clear(), bitmap_find_first_free(), bitmap_set(), bitmap_test() (+4 more)

### Community 5 - "GUI & Window Manager"
Cohesion: 0.14
Nodes (21): close_window(), create_window(), draw_desktop_background(), draw_desktop_icon(), draw_desktop_icons(), draw_start_menu(), draw_taskbar(), draw_window_internal() (+13 more)

### Community 6 - "SimpleFS Filesystem"
Cohesion: 0.2
Nodes (18): read_dir(), shallow_get_dir_lba(), simplefs_alloc_blocks(), simplefs_create_file(), simplefs_delete_file(), simplefs_find(), simplefs_get_dir_lba(), simplefs_getcwd() (+10 more)

### Community 7 - "Interrupt & I/O Handling"
Cohesion: 0.14
Nodes (7): idt_set_gate(), init_idt(), pic_remap(), bcd_to_binary(), get_RTC_register(), get_update_in_progress_flag(), read_time()

### Community 8 - "Graphics Primitives"
Cohesion: 0.19
Nodes (10): draw_char(), draw_char_transparent(), draw_cursor(), draw_rect(), draw_string(), draw_window(), put_pixel(), init_mouse() (+2 more)

### Community 9 - "GDT & Task Management"
Cohesion: 0.17
Nodes (9): gdt_set_gate(), init_gdt(), write_tss(), exit_task(), get_task_by_pid(), schedule(), sys_kill(), sys_wait() (+1 more)

### Community 10 - "Keyboard & Terminal"
Cohesion: 0.17
Nodes (5): terminal_initialize(), terminal_putchar(), terminal_set_mode(), terminal_write(), terminal_writestring()

### Community 11 - "Standard Library & Echo"
Cohesion: 0.4
Nodes (0): 

### Community 12 - "DNS Resolution"
Cohesion: 0.47
Nodes (4): htons(), ntohs(), format_dns_name(), main()

### Community 13 - "ATA Disk Driver"
Cohesion: 0.8
Nodes (5): ata_delay(), ata_read_sector(), ata_wait_bsy(), ata_wait_drq(), ata_write_sector()

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Should `User Space Applications` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `System Call Interface` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Network Protocol Stack` be split into smaller, more focused modules?**
  _Cohesion score 0.07 - nodes in this community are weakly interconnected._
- **Should `Network Syscalls & Sockets` be split into smaller, more focused modules?**
  _Cohesion score 0.08 - nodes in this community are weakly interconnected._
- **Should `Kernel Memory & ELF Loader` be split into smaller, more focused modules?**
  _Cohesion score 0.11 - nodes in this community are weakly interconnected._
- **Should `GUI & Window Manager` be split into smaller, more focused modules?**
  _Cohesion score 0.14 - nodes in this community are weakly interconnected._
- **Should `Interrupt & I/O Handling` be split into smaller, more focused modules?**
  _Cohesion score 0.14 - nodes in this community are weakly interconnected._