幫我把 build-app.sh 整合到 Makefile 裡：

```build-app.sh
#!/bin/bash

rm -f app.o my_app.elf

# 1. 編譯獨立的 User App (加入解除武裝的參數)
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o

# 2. 連結 App
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
```

makefile
```Makefile
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace os-builder

# [新增] -Ilib 告訴 GCC：如果遇到 #include "xxx.h"，請去 lib/ 目錄下找！
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ilib

# 自動抓取所有的 .c 與 .S 檔案
C_SOURCES = $(wildcard lib/*.c)
C_OBJS = $(C_SOURCES:.c=.o)
ASM_SOURCES = $(wildcard asm/*.S)
ASM_OBJS = $(ASM_SOURCES:.S=.o)

# 把所有 .o 檔統整起來 (注意 boot.o 必須在前面，雖然 linker.ld 裡有指定 ENTRY，但放前面更保險)
OBJS = asm/boot.o $(filter-out asm/boot.o, $(ASM_OBJS)) $(C_OBJS)

all: build-env myos.iso

build-env:
	docker build -t os-builder .

myos.iso: myos.bin grub.cfg my_app.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin $(OBJS)

# --- 硬碟映像檔建置規則 ---
hdd.img:
	@echo "==> 建立 10MB 的空白虛擬硬碟..."
	dd if=/dev/zero of=hdd.img bs=1M count=10
	@echo "==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表..."
	docker run --rm -i --platform linux/amd64 -v $(CURDIR):/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
	@echo "==> 硬碟 hdd.img 建立與分區完成！"

clean-disk:
	rm -f hdd.img
	@echo "==> 硬碟已刪除，下次 make run 將會產生一顆全新的硬碟。"

%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

clean:
	rm -f hdd.img
	rm -f asm/*.o lib/*.o *.bin *.iso
	rm -rf isodir

run: myos.iso hdd.img my_app.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img my_app.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
```

然後增加一個 clean user app 的動作


---


把建置腳本完美收斂進 `Makefile` 絕對是明智之舉！這樣我們只要敲一次 `make`，不管是 OS 核心還是應用程式，都會一氣呵成地自動編譯和打包，再也不用兩邊切換跑腳本了。

因為 User App 的編譯參數（解除武裝的 `fno-pie` 等）和 Kernel 不太一樣，我們需要為 `app.o` 和 `my_app.elf` 獨立寫兩條規則，以免跟原本的 `%.o: %.c` (編譯 OS 用的) 撞在一起。

底下是幫你完美融合並加上 `clean-app` 標籤的全新 **Makefile**：

```makefile
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace os-builder

# [新增] -Ilib 告訴 GCC：如果遇到 #include "xxx.h"，請去 lib/ 目錄下找！
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ilib

# 自動抓取所有的 .c 與 .S 檔案
C_SOURCES = $(wildcard lib/*.c)
C_OBJS = $(C_SOURCES:.c=.o)
ASM_SOURCES = $(wildcard asm/*.S)
ASM_OBJS = $(ASM_SOURCES:.S=.o)

# 把所有 .o 檔統整起來 (注意 boot.o 必須在前面，雖然 linker.ld 裡有指定 ENTRY，但放前面更保險)
OBJS = asm/boot.o $(filter-out asm/boot.o, $(ASM_OBJS)) $(C_OBJS)

all: build-env myos.iso

build-env:
	docker build -t os-builder .

# --- User App 建置規則 (取代原本的 build-app.sh) ---
app.o: app.c
	@echo "==> 編譯 User App (app.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o

my_app.elf: app.o
	@echo "==> 連結 User App，指定 Entry Point 於 0x08048000..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf

# 專屬清理 User App 的指令
clean-app:
	rm -f app.o my_app.elf
	@echo "==> User App 已清除。"

# --- OS ISO 建置規則 ---
# 注意：這裡依賴了 my_app.elf，所以只要打 make myos.iso，它就會自動先去編譯 App
myos.iso: myos.bin grub.cfg my_app.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin $(OBJS)

# --- 硬碟映像檔建置規則 ---
hdd.img:
	@echo "==> 建立 10MB 的空白虛擬硬碟..."
	dd if=/dev/zero of=hdd.img bs=1M count=10
	@echo "==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表..."
	docker run --rm -i --platform linux/amd64 -v $(CURDIR):/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
	@echo "==> 硬碟 hdd.img 建立與分區完成！"

clean-disk:
	rm -f hdd.img
	@echo "==> 硬碟已刪除，下次 make run 將會產生一顆全新的硬碟。"

# 核心編譯規則
%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

# 總清理規則：順便觸發 clean-app
clean: clean-app
	rm -f hdd.img
	rm -f asm/*.o lib/*.o *.bin *.iso
	rm -rf isodir

run: myos.iso hdd.img my_app.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img my_app.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d

```

### 💡 這次改動帶來的效益：

1. **相依性自動追蹤：** 只要你改了 `app.c`，你打 `make run` 的時候，Makefile 會聰明地發現 `app.c` 更新了，自動幫你重編 `app.o` -> 重連 `my_app.elf` -> 重新打包 `myos.iso`，完全自動化！
2. **打掃得乾乾淨淨：** 原本的 `make clean` 我順便幫你掛上了 `clean-app` 的相依，現在只要敲一次 `make clean`，連同 OS 的中間檔、硬碟還有 App 都會一起掃得乾乾淨淨。
3. 原本的 `build-app.sh` 腳本就可以直接 `rm` 刪除，正式退役啦！

換上去之後可以隨便改個 `app.c` 裡面的 `sys_print` 字串，然後直接執行 `make run`，享受一下被自動化建置伺候的爽快感吧！準備好進入 Day 29 的動態載入環節隨時叫我！




---


```bash
❯ make
docker build -t os-builder .
[+] Building 1.4s (7/7) FINISHED                                                                                                      docker:orbstack
 => [internal] load build definition from Dockerfile                                                                                             0.0s
 => => transferring dockerfile: 288B                                                                                                             0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)                             0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                                               1.2s
 => [internal] load .dockerignore                                                                                                                0.0s
 => => transferring context: 2B                                                                                                                  0.0s
 => [1/3] FROM docker.io/library/debian:bullseye@sha256:550c53afe18c60e0dc975327dcb75a4ef8265e247e1190822bf8881a70825fce                         0.0s
 => CACHED [2/3] RUN apt-get update && apt-get install -y     nasm     build-essential     grub-pc-bin     grub-common     xorriso     && rm -r  0.0s
 => CACHED [3/3] WORKDIR /workspace                                                                                                              0.0s
 => exporting to image                                                                                                                           0.0s
 => => exporting layers                                                                                                                          0.0s
 => => writing image sha256:4ab04f6c1af7e382dc7a50ee3bab3a2c44aadf15db5f155e13c651c399af54ba                                                     0.0s
 => => naming to docker.io/library/os-builder                                                                                                    0.0s

 1 warning found (use docker --debug to expand):
 - FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
lib/kernel.c: In function 'kernel_main':
lib/kernel.c:64:40: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   64 |             uint32_t user_stack_phys = pmm_alloc_page();
      |                                        ^~~~~~~~~~~~~~
lib/kernel.c:19:27: warning: unused parameter 'magic' [-Wunused-parameter]
   19 | void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
      |                  ~~~~~~~~~^~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
lib/syscall.c: In function 'syscall_handler':
lib/syscall.c:16:31: warning: unused parameter 'edi' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                      ~~~~~~~~~^~~
lib/syscall.c:16:45: warning: unused parameter 'esi' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                    ~~~~~~~~~^~~
lib/syscall.c:16:59: warning: unused parameter 'ebp' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                                  ~~~~~~~~~^~~
lib/syscall.c:16:73: warning: unused parameter 'esp' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                                                ~~~~~~~~~^~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
==> 編譯 User App (app.c)...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o
==> 連結 User App，指定 Entry Point 於 0x08048000...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  279g free
Added to ISO image: directory '/'='/tmp/grub.VGr6Vi'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4660 sectors
Written to medium : 4660 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
❯ ls
app.c      app.o      asm        Dockerfile grub.cfg   lib        linker.ld  Makefile   my_app.elf myos.bin   myos.iso
```
