DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace os-builder

# 將最終目標改成 myos.iso
all: build-env myos.iso

build-env:
	docker build -t os-builder .

# 新增打包 ISO 的步驟
myos.iso: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

myos.bin: boot.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o

boot.o: boot.S
	$(DOCKER_CMD) nasm -f elf32 boot.S -o boot.o

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir

run: myos.iso
	qemu-system-i386 -cdrom myos.iso -monitor stdio -boot d
