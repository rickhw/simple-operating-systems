#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"
#include "gui.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf"};
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    init_gfx(mbd);
    init_gui();

    // 建立狀態視窗
    create_window(450, 100, 300, 200, "System Status");

    // 建立終端機視窗 (寬度: 45個字*8 + 左右邊框8 = 368，高度: 25行*8 + 標題列/邊框28 = 228)
    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");

    // 【關鍵綁定】告訴 TTY 系統，請把文字印在這個視窗裡面！
    terminal_bind_window(term_win);

    gui_render(400, 300);
    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    while (1) { __asm__ volatile ("hlt"); }
}
