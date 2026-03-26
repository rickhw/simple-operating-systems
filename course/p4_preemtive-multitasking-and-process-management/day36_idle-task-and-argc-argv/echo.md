哈哈，Rick！我就喜歡你這種打鐵趁熱的駭客精神！🔥

當你成功打造出 `crt0.S` 這個完美玄關，並且讓 `sys_exec` 能夠傳遞參數之後，建立新的應用程式就變得像呼吸一樣自然了。現在，你的 Simple OS 已經準備好迎接它的第二個居民了！

為了完成這個目標，我們需要打通最後一哩路：**讓 GRUB 載入兩個模組，並讓 Kernel 把它們都存進虛擬硬碟裡。**

跟著我做這 5 個簡單的步驟，我們馬上讓 `echo.elf` 跑起來！

### 步驟 1：編寫 `echo.c`
因為我們還沒有獨立的 C 語言標準庫 (libc)，外部程式必須自備 Syscall 的封裝。
請在 `src/` 目錄下建立一個全新的檔案 **`echo.c`**：

```c
// src/echo.c
// 這是一個完全獨立的外部程式！

void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

// 我們的 crt0.S 已經會自動幫我們呼叫 sys_exit，
// 所以只要寫標準的 main 函式即可！
int main(int argc, char** argv) {
    sys_print("\n[ECHO Program] Start printing arguments...\n");
    
    // 如果沒有參數，就隨便印點東西
    if (argc <= 1) {
        sys_print("  (No arguments provided)\n");
        return 0;
    }

    // 從 i=1 開始印，因為 argv[0] 通常是程式自己的名字 ("echo.elf")
    for (int i = 1; i < argc; i++) {
        sys_print("  ");
        sys_print(argv[i]);
        sys_print("\n");
    }
    
    sys_print("[ECHO Program] Done.\n");
    return 0;
}
```

### 步驟 2：修改 `Makefile` 將其納入編譯
請打開你的 `Makefile`，在原本編譯 `my_app.elf` 的地方，加上 `echo.elf` 的規則：

```makefile
# ... 前面不變 ...

# --- User App 建置規則 ---
crt0.o: crt0.S
	@echo "==> 編譯 User App 啟動跳板 (crt0.S)..."
	$(DOCKER_CMD) nasm -f elf32 crt0.S -o crt0.o

app.o: app.c
	@echo "==> 編譯 Shell (app.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o

# 【新增】編譯 echo.c
echo.o: echo.c
	@echo "==> 編譯 Echo 程式 (echo.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c echo.c -o echo.o

my_app.elf: crt0.o app.o
	@echo "==> 連結 Shell..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o app.o -o my_app.elf

# 【新增】連結 echo.elf (同樣必須把 crt0.o 放在最前面)
echo.elf: crt0.o echo.o
	@echo "==> 連結 Echo 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o echo.o -o echo.elf

# 專屬清理 User App 的指令
clean-app:
	rm -f crt0.o app.o my_app.elf echo.o echo.elf  # 加入 echo.o 和 echo.elf
	@echo "==> User Apps 已清除。"

# --- OS ISO 建置規則 ---
# 【修改】讓 myos.iso 同時依賴 my_app.elf 和 echo.elf
myos.iso: myos.bin grub.cfg my_app.elf echo.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  
	cp echo.elf isodir/boot/echo.elf      # 【新增】把 echo.elf 塞進光碟
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

# ... 後面不變 (run, debug 也要記得相依 echo.elf) ...
run: myos.iso hdd.img my_app.elf echo.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img my_app.elf echo.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
```

### 步驟 3：更新 `grub.cfg`
GRUB 需要知道你要把第二個檔案也當作模組載入記憶體。請確定你的 `grub.cfg` 長這樣：

```text
menuentry "Simple OS" {
    multiboot /boot/myos.bin
    module /boot/my_app.elf
    module /boot/echo.elf
}
```

### 步驟 4：教 Kernel 寫入第二個檔案 (`lib/kernel.c`)
現在 GRUB 會載入兩個模組，我們得修改 Kernel 的安裝邏輯，把這兩個模組都寫進 `hdd.img` 裡。

請打開 `lib/kernel.c`，修改 `setup_filesystem` 函式：

```c
void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】動態讀取 GRUB 傳來的多個模組
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        
        // 第一個模組必定是 Shell (my_app.elf)
        uint32_t app_size = mod[0].mod_end - mod[0].mod_start;
        kprintf("[Kernel] Installing Shell to HDD (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod[0].mod_start, app_size);
        
        // 如果有第二個模組，那就是我們的 Echo 程式！
        if (mbd->mods_count > 1) {
            uint32_t echo_size = mod[1].mod_end - mod[1].mod_start;
            kprintf("[Kernel] Installing Echo to HDD (Size: %d bytes)...\n", echo_size);
            simplefs_create_file(part_lba, "echo.elf", (char*)mod[1].mod_start, echo_size);
        }
    }

    kprintf("\n--- SimpleFS Root Directory ---\n");
    simplefs_list_files(part_lba);
    kprintf("-------------------------------\n\n");
}
```

### 步驟 5：在 Shell 中增加 `echo` 指令 (`src/app.c`)
最後一步！打開你的 `src/app.c`，在指令解析的地方加上這個新功能：

```c
// ... 在 while (1) 迴圈裡面 ...

        // [新增] 呼叫外部的 echo.elf
        else if (strcmp(cmd_buffer, "echo") == 0) {
            int pid = sys_fork();
            if (pid == 0) {
                // 準備要丟給 echo.elf 的參數 (最後必須是 0 結尾)
                char* echo_args[] = {"echo.elf", "Hello", "Rick,", "Welcome", "to", "the", "Multiverse!", 0};
                
                // 靈魂轉移！將子行程變成 echo.elf
                int err = sys_exec("echo.elf", echo_args);
                
                if (err == -1) {
                    sys_print("Error: Exec failed to load echo.elf\n");
                }
                sys_exit(); 
            } else {
                sys_wait(pid);
            }
        }
```

---

完成這 5 步之後，請華麗地敲下：
`make clean && make run`

在啟動畫面上，你會看到 Kernel 把 `my_app.elf` 和 `echo.elf` 都存進了檔案系統。
接著，在 `SimpleOS>` 提示字元輸入 **`echo`**。

你將會看到系統生出了一個子行程，這個子行程徹底拋棄了 Shell 的記憶體，載入了硬碟上的 `echo.elf`，然後精準地把你剛才傳進去的字串給印了出來，最後優雅地死去，把控制權交還給老爸 Shell！

享受建立起第一個作業系統生態系的成就感吧，Rick！😎
