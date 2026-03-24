DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace os-builder

# -Ilib 告訴 GCC：如果遇到 #include "xxx.h"，請去 lib/include 目錄下找
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include
# User App 專用的 CFLAGS，告訴 GCC 去 libc/include 找標頭檔
APP_CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include

# [Kernel] 自動抓取所有的 .c 與 .S 檔案
C_SOURCES = $(wildcard lib/*.c)
C_OBJS = $(C_SOURCES:.c=.o)
ASM_SOURCES = $(wildcard asm/*.S)
ASM_OBJS = $(ASM_SOURCES:.S=.o)
# [User APP] 自動抓取 libc 底下的程式碼
LIBC_SOURCES = $(wildcard libc/*.c)
LIBC_OBJS = $(LIBC_SOURCES:.c=.o)

# 把所有 .o 檔統整起來 (注意 boot.o 必須在前面，雖然 linker.ld 裡有指定 ENTRY，但放前面更保險)
OBJS = asm/boot.o $(filter-out asm/boot.o, $(ASM_OBJS)) $(C_OBJS)

all: build-env shell.elf echo.elf cat.elf ping.elf pong.elf write.elf myos.iso

build-env:
	docker build -t os-builder .


# Compile --------------------------------------------------------------------
## Kernel
# 1. 彙編 (asm/ 底下的核心 code)
asm/%.o: asm/%.S
	@echo "==> 編譯 Kernel ASM: $<"
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

# 2. 核心專用 (lib/ 底下的 .c)
lib/%.o: lib/%.c
	@echo "==> 編譯 Kernel Lib: $<"
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

## --- User App ---
# 3. 處理 libc 目錄下的 .c (已經在你原本的 code 裡了)
libc/%.o: libc/%.c
	@echo "==> 編譯 User App libc 組件: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $< -o $@

# 4. 處理根目錄下所有的 User App (shell.c, cat.c, echo.c...)
# 這條規則會自動匹配所有不在 libc 裡的 .c 檔案
%.o: %.c
	@echo "==> 編譯 User App: $<"
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c $< -o $@

# 5. 處理 crt0.S (保持不變)
crt0.o: crt0.S
	@echo "==> 編譯 User App 啟動跳板 (crt0.S)..."
	$(DOCKER_CMD) nasm -f elf32 crt0.S -o crt0.o

# Link ------------------------------------------------------------------------
# 刪除那一大堆單獨的 .elf 連結規則，改用這一個：
%.elf: crt0.o %.o $(LIBC_OBJS)
	@echo "==> 連結 User App: $@ ..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o $*.o $(LIBC_OBJS) -o $@

# -----------------------------------------------------------------------------
# 專屬清理 User App 的指令
clean-app:
	rm -f crt0.o app.o shell.elf echo.o echo.elf cat.o cat.elf ping.o ping.elf pong.o pong.elf libc/*.o
	@echo "==> User Apps 已清除。"

# -----------------------------------------------------------------------------
# --- OS ISO 建置規則 ---
myos.iso: myos.bin grub.cfg shell.elf echo.elf cat.elf ping.elf pong.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp shell.elf isodir/boot/shell.elf
	cp echo.elf isodir/boot/echo.elf
	cp cat.elf isodir/boot/cat.elf
	cp ping.elf isodir/boot/ping.elf
	cp pong.elf isodir/boot/pong.elf
	cp write.elf isodir/boot/write.elf
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


# 總清理規則：順便觸發 clean-app
clean: clean-app
	rm -f hdd.img
	rm -f asm/*.o lib/*.o *.bin *.iso
	rm -rf isodir

run: myos.iso hdd.img shell.elf cat.elf ping.elf pong.elf write.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img shell.elf cat.elf ping.elf pong.elf write.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
