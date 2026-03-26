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

#myos.iso: myos.bin grub.cfg my_app.elf
myos.iso: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	# cp my_app.elf isodir/boot/my_app.elf  # [新增這行！] 把執行檔塞進虛擬光碟
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin $(OBJS)

%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

clean:
	rm -f asm/*.o lib/*.o *.bin *.iso
	rm -rf isodir

run:
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img

debug:
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img
