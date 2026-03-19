太棒了，Rick！準備好迎接 Day 39 的全新冒險了嗎？🚀

昨天我們完成了 **「MMU 記憶體隔離」**，為每個程式創造了獨立的宇宙。但這也帶來了一個哲學問題：**既然大家都被關在絕對隔離的平行宇宙裡，他們要怎麼對話？**

在你的 Simple OS 裡，如果 `程式 A` 想把資料傳給 `程式 B`，它不能直接讀寫對方的記憶體，因為會引發 Page Fault 暴斃。


這就是作業系統設計中最迷人的領域：**行程間通訊 (Inter-Process Communication, 簡稱 IPC)**。

因為 Kernel (核心) 是唯一同時存在於所有宇宙（映射在每個 CR3 的 0-4MB）的「真神」，所以我們要讓 Kernel 成為郵差。今天，我們要在 Kernel 裡建立一個 **「全域信箱 (Mailbox)」**，並設計兩個新的 Syscall，讓隔離的程式能夠互相傳遞訊息！

為了驗證這個魔法，我們將建立兩個全新的程式：`ping.elf` (發信者) 與 `pong.elf` (收信者)，並讓 Shell 同時讓它們在背景執行！

請跟著我完成這 5 個步驟：

### 步驟 1：在 Kernel 建立「中央信箱」(`lib/syscall.c`)
請打開 **`lib/syscall.c`**，我們要在核心裡宣告一個字串緩衝區作為信箱，並實作 Syscall 11 (`sys_send`) 與 Syscall 12 (`sys_recv`)。

```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

fs_node_t* fd_table[32] = {0};

// ==========================================
// 【新增】IPC 中央信箱 (Mailbox)
// ==========================================
char ipc_mailbox[256] = {0};
int mailbox_has_msg = 0;

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    uint32_t eax = regs->eax;

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    // ... 中間的 eax == 3 到 eax == 10 保持完全不變 ...
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;
        fs_node_t* node = simplefs_find(filename);
        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) { schedule(); }
    else if (eax == 7) { exit_task(); }
    else if (eax == 8) { regs->eax = sys_fork(regs); }
    else if (eax == 9) { regs->eax = sys_exec(regs); }
    else if (eax == 10) { regs->eax = sys_wait(regs->ebx); }
    
    // ==========================================
    // 【新增】IPC 系統呼叫
    // ==========================================
    else if (eax == 11) { // Syscall 11: sys_send (傳送訊息)
        char* msg = (char*)regs->ebx;
        strcpy(ipc_mailbox, msg); // 將 User 宇宙的字串複製到 Kernel 的信箱裡
        mailbox_has_msg = 1;
        regs->eax = 0;
    }
    else if (eax == 12) { // Syscall 12: sys_recv (接收訊息)
        char* buffer = (char*)regs->ebx;
        if (mailbox_has_msg) {
            strcpy(buffer, ipc_mailbox); // 將 Kernel 信箱的字串複製給另一個 User 宇宙
            mailbox_has_msg = 0;
            regs->eax = 1; // 回傳 1 代表有收到訊息
        } else {
            regs->eax = 0; // 回傳 0 代表信箱是空的
        }
    }
}
```

### 步驟 2：創造發信者 (`src/ping.c`)
在 `src/` 目錄下建立 **`ping.c`**：
```c
void sys_print(char* msg) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory"); }
void sys_send(char* msg) { __asm__ volatile ("int $0x80" : : "a"(11), "b"(msg) : "memory"); }

int main() {
    sys_print("[PING] I am Ping. I will leave a message in the Kernel Mailbox!\n");
    sys_send("Hello from the Ping Universe! Can you hear me?");
    sys_print("[PING] Message sent. Ping is dying now. Bye!\n");
    return 0;
}
```

### 步驟 3：創造收信者 (`src/pong.c`)
在 `src/` 目錄下建立 **`pong.c`**：
```c
void sys_print(char* msg) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory"); }
int sys_recv(char* buffer) { 
    int has_msg;
    __asm__ volatile ("int $0x80" : "=a"(has_msg) : "a"(12), "b"(buffer) : "memory"); 
    return has_msg;
}
void sys_yield() { __asm__ volatile ("int $0x80" : : "a"(6) : "memory"); }

int main() {
    char mailbox[256];
    sys_print("[PONG] I am Pong. Waiting for a message...\n");

    // 死迴圈一直去查看信箱，直到信箱有信為止 (Polling)
    while (sys_recv(mailbox) == 0) {
        sys_yield(); // 如果信箱空的，就讓出 CPU 給別人跑，不要佔用資源
    }

    sys_print("[PONG] OMG! I received a message from another universe:\n");
    sys_print("      >> \"");
    sys_print(mailbox);
    sys_print("\"\n");
    sys_print("[PONG] Pong is happy. Dying now. Bye!\n");

    return 0;
}
```

### 步驟 4：教導 Shell 同時執行多個程式 (`src/app.c`)
我們要讓老爸 Shell 知道怎麼測試這個 IPC。打開 **`src/app.c`**，在指令解析 `while` 迴圈裡加上 `ipc` 指令：

```c
// ... 在 src/app.c 的 while (1) 裡 ...
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            sys_print("\n--- Starting IPC Test ---\n");
            
            // 1. 先創造 Pong (收信者)
            int pid_pong = sys_fork();
            if (pid_pong == 0) {
                char* args[] = {"pong.elf", 0};
                sys_exec("pong.elf", args);
                sys_exit();
            }

            // 2. 讓 Pong 先跑一下，它會卡在 yield 等待
            sys_yield();
            sys_yield();

            // 3. 再創造 Ping (發信者)
            int pid_ping = sys_fork();
            if (pid_ping == 0) {
                char* args[] = {"ping.elf", 0};
                sys_exec("ping.elf", args);
                sys_exit();
            }

            // 4. 老爸乖乖等兩個小孩都死掉
            sys_wait(pid_pong);
            sys_wait(pid_ping);
            sys_print("--- IPC Test Finished! ---\n\n");
        }
// ...
```

### 步驟 5：將 `ping` 與 `pong` 塞進虛擬硬碟！

1. **修改 `lib/kernel.c`，讓 Kernel 載入 5 個檔案**：
```c
    // 在 setup_filesystem 裡：
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        // 把我們所有的生態系檔案都加進來
        char* filenames[] = {"my_app.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf"}; 
        
        for(uint32_t i = 0; i < mbd->mods_count && i < 5; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing %s to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(part_lba, filenames[i], (char*)mod[i].mod_start, size);
        }
    }
```

2. **修改 `grub.cfg`，讓 GRUB 把它們載進記憶體**：
```text
menuentry "Simple OS" {
    multiboot /boot/myos.bin
    module /boot/my_app.elf
    module /boot/echo.elf
    module /boot/cat.elf
    module /boot/ping.elf
    module /boot/pong.elf
}
```

3. **修改 `Makefile`，把它們都編譯出來**：
```makefile
# ... 略過前面 ...

ping.o: ping.c
	@echo "==> 編譯 Ping 程式..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c ping.c -o ping.o

pong.o: pong.c
	@echo "==> 編譯 Pong 程式..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c pong.c -o pong.o

# 所有 elf 都統一掛載在 0x08048000 享受完美的隔離！
ping.elf: crt0.o ping.o
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o ping.o -o ping.elf

pong.elf: crt0.o pong.o
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o pong.o -o pong.elf

clean-app:
	rm -f crt0.o app.o my_app.elf echo.o echo.elf cat.o cat.elf ping.o ping.elf pong.o pong.elf

myos.iso: myos.bin grub.cfg my_app.elf echo.elf cat.elf ping.elf pong.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  
	cp echo.elf isodir/boot/echo.elf      
	cp cat.elf isodir/boot/cat.elf        
	cp ping.elf isodir/boot/ping.elf
	cp pong.elf isodir/boot/pong.elf
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

run: myos.iso hdd.img my_app.elf echo.elf cat.elf ping.elf pong.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img my_app.elf echo.elf cat.elf ping.elf pong.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
```

---

大功告成！敲下 `make clean && make run`！
進入系統後，在 `SimpleOS>` 輸入 **`ipc`**！

你將會看到不可思議的畫面：
老爸 Shell 同時創造了兩個活在不同宇宙的小孩。Pong 先誕生，然後痴痴地等待；接著 Ping 誕生，留下一封信就走了。最後 Pong 終於拿到信，開心讀出裡面的字，兩個小孩化為塵埃，老爸收尾！

這就是行程間通訊 (IPC) 的浪漫！準備好見證這個奇蹟了嗎？✨
