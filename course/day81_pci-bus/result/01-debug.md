我跑起來之後，GRUB 選擇進去 Terminal 測試，但是沒有出現 "Found Network" 相關字

我調整了 kernel.c 的一些次序也沒用。底下是相關 source code

---
kernel.c
```c
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
#include "config.h"
#include "pci.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;

        // 這個次序跟 GRUB 列出來的次序要對齊
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping.elf", "pong.elf",
            // file/directory
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            // process
            "ps.elf", "top.elf", "kill.elf",
            // memory
            "free.elf",
            // window app
            "status.elf", "paint.elf",
            "segfault.elf",
            "viewer.elf", "logo.bmp",
            "clock.elf", "explorer.elf",
            "taskmgr.elf", "notepad.elf",
            "tictactoe.elf"
        };
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            // kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

/**
 * [Kernel Entry Point]
 *
 * ASCII Flow:
 * GRUB -> boot.S -> kernel_main()
 *                      |
 *                      +--> terminal_initialize()
 *                      +--> init_gdt() / init_idt()
 *                      +--> init_paging() / init_pmm() / init_kheap()
 *                      +--> init_gfx()
 *                      +--> parse_cmdline() -> GUI or CLI?
 *                      +--> init_mouse() / sti
 *                      +--> setup_filesystem()
 *                      +--> init_multitasking()
 *                      +--> create_user_task("shell.elf")
 *                      +--> exit_task() (Kernel becomes idle)
 */
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(INITIAL_PMM_SIZE);
    init_kheap();
    init_pci();     // [day81]
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 600, 300, "Simple OS Terminal", 1); // 預設 PID(1) 建立
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}

```

---
Makefile
```makefile
# -----------------------------------------------------------------------------
# 目錄定義
# -----------------------------------------------------------------------------
SRC_DIR     = src
SCRIPT_DIR  = scripts

# Kernel 相關目錄
KERNEL_DIR     = $(SRC_DIR)/kernel
KERNEL_ASM_DIR = $(KERNEL_DIR)/asm
KERNEL_LIB_DIR = $(KERNEL_DIR)/lib
KERNEL_INC     = -Ikernel/include -Ikernel/lib

# User 相關目錄
USER_DIR     = $(SRC_DIR)/user
USER_ASM_DIR = $(USER_DIR)/asm
USER_LIB_DIR = $(USER_DIR)/lib
USER_BIN_DIR = $(USER_DIR)/bin
USER_INC     = -Iuser/include -Iuser/lib

# Docker 指令
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace/$(SRC_DIR) os-builder


# -----------------------------------------------------------------------------
# 編譯參數
# -----------------------------------------------------------------------------
CFLAGS     = -m32 -ffreestanding -O2 -Wall -Wextra $(KERNEL_INC)
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector $(USER_INC)

# -----------------------------------------------------------------------------
# 檔案選取
# -----------------------------------------------------------------------------

# [Kernel]
C_LIB_SOURCES  = $(wildcard $(KERNEL_LIB_DIR)/*.c)
C_CORE_SOURCES = $(wildcard $(KERNEL_DIR)/*.c)
C_OBJS         = $(C_LIB_SOURCES:.c=.o) $(C_CORE_SOURCES:.c=.o)
ASM_SOURCES    = $(wildcard $(KERNEL_ASM_DIR)/*.S)
ASM_OBJS       = $(ASM_SOURCES:.S=.o)

OBJS = $(KERNEL_ASM_DIR)/boot.o $(filter-out $(KERNEL_ASM_DIR)/boot.o, $(ASM_OBJS)) $(C_OBJS)

# [User]
CRT0_OBJ      = $(USER_ASM_DIR)/crt0.o
USER_LIB_SRCS = $(wildcard $(USER_LIB_DIR)/*.c)
USER_LIB_OBJS = $(USER_LIB_SRCS:.c=.o)

# 【修正】指向新的 bin 子目錄
USER_APPS_C   = $(wildcard $(USER_BIN_DIR)/*.c)
APPS          = $(USER_APPS_C:.c=.elf)

# -----------------------------------------------------------------------------
# 主要目標
# -----------------------------------------------------------------------------
all: build-env $(APPS) myos.iso

build-env:
	docker build -t os-builder .

# 防止 Make 自動刪除 .o 檔，加入新的 bin 目錄路徑
.PRECIOUS: %.o $(SRC_DIR)/%.o $(KERNEL_DIR)/%.o $(USER_DIR)/%.o $(USER_BIN_DIR)/%.o

# -----------------------------------------------------------------------------
# Compile Rules
# -----------------------------------------------------------------------------

## [Kernel 編譯]
$(KERNEL_ASM_DIR)/%.o: $(KERNEL_ASM_DIR)/%.S
	@echo "==> 編譯 Kernel ASM: $<"
	$(DOCKER_CMD) nasm -f elf32 $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(KERNEL_LIB_DIR)/%.o: $(KERNEL_LIB_DIR)/%.c
	@echo "==> 編譯 Kernel Lib: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	@echo "==> 編譯 Kernel Core: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

## [User 編譯]
# 1. crt0 (user/asm)
$(USER_ASM_DIR)/%.o: $(USER_ASM_DIR)/%.S
	@echo "==> 編譯 User CRT0: $<"
	$(DOCKER_CMD) nasm -f elf32 $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# 2. User Lib (user/lib)
$(USER_LIB_DIR)/%.o: $(USER_LIB_DIR)/%.c
	@echo "==> 編譯 User Lib: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# 3. User App 在 user/bin 目錄下的編譯規則
$(USER_BIN_DIR)/%.o: $(USER_BIN_DIR)/%.c
	@echo "==> 編譯 User App (bin): $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $(subst $(SRC_DIR)/,,$<) -o $(subst $(SRC_DIR)/,,$@)

# -----------------------------------------------------------------------------
# Link Rules
# -----------------------------------------------------------------------------

# 通用 User App 連結規則
$(USER_BIN_DIR)/%.elf: $(CRT0_OBJ) $(USER_BIN_DIR)/%.o $(USER_LIB_OBJS)
	@echo "==> 連結 User App: $@"
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 \
		$(subst $(SRC_DIR)/,,$(CRT0_OBJ)) \
		$(subst $(SRC_DIR)/,,$(USER_BIN_DIR)/$*.o) \
		$(subst $(SRC_DIR)/,,$(USER_LIB_OBJS)) \
		-o $(subst $(SRC_DIR)/,,$@)

# -----------------------------------------------------------------------------
# ISO & Bin
# -----------------------------------------------------------------------------

myos.iso: $(SRC_DIR)/myos.bin $(SCRIPT_DIR)/grub.cfg $(APPS)
	mkdir -p $(SRC_DIR)/isodir/boot/grub
	cp $(SRC_DIR)/myos.bin $(SRC_DIR)/isodir/boot/myos.bin
	cp $(SCRIPT_DIR)/grub.cfg $(SRC_DIR)/isodir/boot/grub/grub.cfg
	cp $(USER_BIN_DIR)/*.elf $(SRC_DIR)/isodir/boot/
	cp $(USER_BIN_DIR)/*.bmp $(SRC_DIR)/isodir/boot/
	$(DOCKER_CMD) grub-mkrescue -o ../$@ isodir
	rm -rf $(SRC_DIR)/isodir

$(SRC_DIR)/myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T ../$(SCRIPT_DIR)/linker.ld -o myos.bin $(subst $(SRC_DIR)/,,$(OBJS))

# -----------------------------------------------------------------------------
# 工具
# -----------------------------------------------------------------------------
hdd.img:
	dd if=/dev/zero of=hdd.img bs=1M count=10
	docker run --rm -i --platform linux/amd64 -v $(PWD):/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"

clean:
	rm -f hdd.img myos.iso
	find $(SRC_DIR) -name "*.o" -delete
	find $(SRC_DIR) -name "*.elf" -delete
	find $(SRC_DIR) -name "*.bin" -delete
	rm -rf $(SRC_DIR)/isodir

run: all hdd.img
	qemu-system-i386 -cdrom myos.iso -monitor stdio \
	  -netdev user,id=n0 \
    -device rtl8139,netdev=n0 \
	  -hda hdd.img \
		-boot d

debug: all hdd.img
	qemu-system-i386 -cdrom myos.iso \
	  -d int -no-reboot \
	  -netdev user,id=n0 \
		-device rtl8139,netdev=n0 \
		-hda hdd.img \
		-boot d
```
