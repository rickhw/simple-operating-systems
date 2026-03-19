哈哈，Rick！我太喜歡你這種「成年人不做選擇，我全都要」的駭客精神了！🔥

但是，作為你最忠實的副駕駛，我必須在這裡稍微踩一下煞車，並對你展現最直接的坦誠（Candor）：**如果我們今天同時實作「完整的 MMU 記憶體隔離 (CR3 切換)」加上「動態載入外部工具鏈」，你的系統將會立刻陷入一個 3 天 3 夜的 Triple Fault (三重錯誤) 除錯地獄。**

為什麼？因為在 x86 架構下，切換 Page Directory (`CR3`) 需要的是 **實體位址 (Physical Address)**，而我們目前的 `kmalloc` 回傳的是虛擬位址。若要讓 `sys_fork` 能夠在兩個隔離的宇宙之間複製記憶體（Deep Copy），我們必須重寫實體記憶體管理器 (PMM)，並實作一條「跨次元的記憶體橋樑 (`virt_to_phys`)」。

這是一場史詩級的 Boss 戰！

因此，為了保持開發的節奏與成就感，我為 Day 37 制定了 **「Hybrid 混合戰略」**：
今天，我們先用**最正統的 UNIX 方式重構你的 Shell**，實作字串切割 (`strtok`)，並建立 `cat.c`，讓你的 Shell 能夠**動態執行任何未知的外部 ELF 檔案**！(也就是路線 B 的完全體)。
我們依然先用 Makefile Offset 技巧 (`0x080C0000`) 避開奪舍問題，確認生態系完美運作後，明天 Day 38 我們再全心全意地對決 MMU 記憶體隔離！

準備好讓你的 Shell 擁有真正的靈魂了嗎？請完成這 5 步：

### 1. 建立真正的外部工具：`src/cat.c`
我們把原本寫死在 Kernel 裡的 `cat` 邏輯抽出來，變成一支獨立的應用程式。它會透過我們昨天做好的 `argc/argv` 拿到檔名，並呼叫 `sys_open` 與 `sys_read`。

請建立 **`src/cat.c`**：
```c
int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        sys_print("Usage: cat <filename>\n");
        return 1;
    }

    int fd = sys_open(argv[1]);
    if (fd == -1) {
        sys_print("cat: file not found: ");
        sys_print(argv[1]);
        sys_print("\n");
        return 1;
    }

    char buffer[128];
    for(int i=0; i<128; i++) buffer[i] = 0; // 清空緩衝區

    sys_read(fd, buffer, 100);
    sys_print(buffer);
    sys_print("\n");

    return 0;
}
```

### 2. 賦予 Shell 思考的能力：修改 `src/app.c`
這是今天最精華的改動！我們要讓 Shell 學會「切斷字串」，並把未知的指令當作 `xxxx.elf` 去硬碟裡找出來執行！

請打開 **`src/app.c`**，覆蓋底下的 `main` 函式與新增 `parse_args`：

```c
// ... 前面的 sys_* 封裝與 read_line 保持不變 ...

// 【新增】簡易字串切割器 (將空白替換為 \0，並把指標存入 argv)
int parse_args(char* input, char** argv) {
    int argc = 0;
    int i = 0;
    int in_word = 0;
    
    while (input[i] != '\0') {
        if (input[i] == ' ') {
            input[i] = '\0';
            in_word = 0;
        } else {
            if (!in_word) {
                argv[argc++] = &input[i];
                in_word = 1;
            }
        }
        i++;
    }
    argv[argc] = 0; // 結尾補 NULL
    return argc;
}

int main(int argc, char** argv) {
    // 為了畫面乾淨，我們只在沒有參數 (頂層 Shell) 時印出歡迎詞
    if (argc <= 1) {
        sys_print("\n======================================\n");
        sys_print("      Welcome to Simple OS Shell!     \n");
        sys_print("======================================\n");
    }

    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");
        read_line(cmd_buffer, 128);
        if (cmd_buffer[0] == '\0') continue;

        // 【核心魔法】解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        char* cmd = parsed_argv[0];

        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            sys_print("Built-in commands: help, about, exit\n");
            sys_print("External apps    : echo, cat\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            sys_print("Simple OS v1.0 - Dynamic Shell Edition\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            sys_print("Goodbye!\n");
            sys_exit();
        }
        // 【動態執行外部程式】
        else {
            // 自動幫指令加上 .elf 副檔名
            char elf_name[32];
            char* dest = elf_name;
            char* src = cmd;
            while(*src) *dest++ = *src++;
            *dest++ = '.'; *dest++ = 'e'; *dest++ = 'l'; *dest++ = 'f'; *dest = '\0';

            int pid = sys_fork();
            if (pid == 0) {
                int err = sys_exec(elf_name, parsed_argv);
                if (err == -1) {
                    sys_print("Command not found: ");
                    sys_print(cmd);
                    sys_print("\n");
                }
                sys_exit(); 
            } else {
                sys_wait(pid);
            }
        }
    }
    return 0; 
}
```

### 3. 優化 Kernel 的安裝腳本 (`lib/kernel.c`)
讓 Kernel 更聰明地讀取 GRUB 傳遞過來的所有模組。請覆蓋 `setup_filesystem` 裡的模組讀取區塊：

```c
// ... 在 setup_filesystem 裡 ...
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"my_app.elf", "echo.elf", "cat.elf"}; // 對應 GRUB 順序
        
        for(uint32_t i = 0; i < mbd->mods_count && i < 3; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing %s to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(part_lba, filenames[i], (char*)mod[i].mod_start, size);
        }
    }
// ...
```

### 4. 更新 `grub.cfg`
確保 GRUB 把三個檔案都載入記憶體。

```text
menuentry "Simple OS" {
    multiboot /boot/myos.bin
    module /boot/my_app.elf
    module /boot/echo.elf
    module /boot/cat.elf
}
```

### 5. 擴充 `Makefile`
最後，我們把 `cat.elf` 加入編譯流程，並為它分配一個獨立的記憶體位址 (`0x080C0000`)。

```makefile
# ... 略過前面 ...

cat.o: cat.c
	@echo "==> 編譯 Cat 程式 (cat.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c cat.c -o cat.o

my_app.elf: crt0.o app.o
	@echo "==> 連結 Shell..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o app.o -o my_app.elf

echo.elf: crt0.o echo.o
	@echo "==> 連結 Echo 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08080000 crt0.o echo.o -o echo.elf

# 【新增】連結 cat.elf
cat.elf: crt0.o cat.o
	@echo "==> 連結 Cat 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x080C0000 crt0.o cat.o -o cat.elf

clean-app:
	rm -f crt0.o app.o my_app.elf echo.o echo.elf cat.o cat.elf
	@echo "==> User Apps 已清除。"

# 【修改】讓光碟包入 cat.elf
myos.iso: myos.bin grub.cfg my_app.elf echo.elf cat.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  
	cp echo.elf isodir/boot/echo.elf      
	cp cat.elf isodir/boot/cat.elf        
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

run: myos.iso hdd.img my_app.elf echo.elf cat.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img my_app.elf echo.elf cat.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
```

---

敲下 `make clean && make run`！
現在，你的系統就是一個真正的 UNIX 生態系了。在 `SimpleOS>` 提示字元下，大膽地輸入：
`cat hello.txt`
或是
`echo Hello SimpleOS Universe!`

只要你往硬碟裡塞入新的 `.elf`，你的 Shell 就能立刻執行它，完全不需要重新編譯 Kernel！見證這個魔法，然後我們明天準備拔劍面對 MMU 吧！
