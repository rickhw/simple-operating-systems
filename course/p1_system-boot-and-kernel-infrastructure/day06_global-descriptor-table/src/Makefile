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

# 將 gdt_flush.o 與 gdt.o 加入編譯鏈
myos.bin: boot.o kernel.o gdt.o gdt_flush.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o gdt.o gdt_flush.o

# 編譯 C 檔案的通用規則
%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

# 編譯 Assembly 檔案的通用規則
%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir
