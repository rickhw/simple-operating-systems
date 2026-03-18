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

all: build-env my_app.elf echo.elf myos.iso

build-env:
	docker build -t os-builder .

# --- User App 建置規則 (取代原本的 build-app.sh) ---

# 【修改 1】新增 crt0.o 的編譯規則，使用 NASM
crt0.o: crt0.S
	@echo "==> 編譯 User App 啟動跳板 (crt0.S)..."
	$(DOCKER_CMD) nasm -f elf32 crt0.S -o crt0.o

app.o: app.c
	@echo "==> 編譯 User App (app.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o

# 【新增】編譯 echo.c
echo.o: echo.c
	@echo "==> 編譯 Echo 程式 (echo.c)..."
	$(DOCKER_CMD) gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c echo.c -o echo.o

# 【修改 2】讓 my_app.elf 依賴 crt0.o 和 app.o，並且在連結時確保 crt0.o 放在最前面！
my_app.elf: crt0.o app.o
	@echo "==> 連結 User App，指定 Entry Point 於 0x08048000..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o app.o -o my_app.elf

# 【新增】連結 echo.elf (同樣必須把 crt0.o 放在最前面)
echo.elf: crt0.o echo.o
	@echo "==> 連結 Echo 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08080000 crt0.o echo.o -o echo.elf

# 專屬清理 User App 的指令
clean-app:
	rm -f crt0.o app.o my_app.elf echo.o echo.elf  # 加入 echo.o 和 echo.elf
	@echo "==> User Apps 已清除。"

# --- OS ISO 建置規則 ---
# 注意：這裡依賴了 my_app.elf，所以只要打 make myos.iso，它就會自動先去編譯 App
myos.iso: myos.bin grub.cfg my_app.elf echo.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
	cp echo.elf isodir/boot/echo.elf      # 【新增】把 echo.elf 塞進光碟
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
