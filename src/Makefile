DOCKER_CMD = docker run --rm -v $(PWD):/workspace -w /workspace os-builder

# GCC 編譯參數：32位元、無標準函式庫、開啟最佳化與警告
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra

all: build-env myos.iso

build-env:
	docker build -t os-builder .

myos.iso: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

# 將 kernel.o 也加入連結清單中
myos.bin: boot.o kernel.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o

kernel.o: kernel.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c kernel.c -o kernel.o

boot.o: boot.S
	$(DOCKER_CMD) nasm -f elf32 boot.S -o boot.o

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir

run: myos.iso
	qemu-system-i386 -cdrom myos.iso -monitor stdio -boot d
