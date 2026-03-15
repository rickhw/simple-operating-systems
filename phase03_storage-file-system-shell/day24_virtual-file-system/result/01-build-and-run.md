
```bash
❯ make clean
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
❯ make
docker build -t os-builder .
[+] Building 1.7s (7/7) FINISHED                                                                                                      docker:orbstack
 => [internal] load build definition from Dockerfile                                                                                             0.0s
 => => transferring dockerfile: 288B                                                                                                             0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)                             0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                                               1.5s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
lib/elf.c: In function 'elf_load':
lib/elf.c:20:13: warning: implicit declaration of function 'kprintf' [-Wimplicit-function-declaration]
   20 |             kprintf("[Loader] Mapping Section: Virt 0x%x -> Phys 0x%x\n", virt_addr, phys_addr);
      |             ^~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
lib/gdt.c: In function 'write_tss':
lib/gdt.c:32:5: warning: implicit declaration of function 'memset' [-Wimplicit-function-declaration]
   32 |     memset(&tss_entry, 0, sizeof(tss_entry_t));
      |     ^~~~~~
lib/gdt.c:3:1: note: 'memset' is defined in header '<string.h>'; did you forget to '#include <string.h>'?
    2 | #include "gdt.h"
  +++ |+#include <string.h>
    3 |
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
lib/kernel.c: In function 'dummy_read_func':
lib/kernel.c:16:69: warning: unused parameter 'size' [-Wunused-parameter]
   16 | uint32_t dummy_read_func(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
      |                                                            ~~~~~~~~~^~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
lib/mbr.c: In function 'parse_mbr':
lib/mbr.c:7:5: warning: implicit declaration of function 'kprintf' [-Wimplicit-function-declaration]
    7 |     kprintf("--- Reading Master Boot Record (LBA 0) ---\n");
      |     ^~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
lib/vfs.c: In function 'vfs_read':
lib/vfs.c:13:9: warning: implicit declaration of function 'kprintf' [-Wimplicit-function-declaration]
   13 |         kprintf("[VFS] Error: Node '%s' does not support read operation.\n", node->name);
      |         ^~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/syscall.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day24_virtual-file-system/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  279g free
Added to ISO image: directory '/'='/tmp/grub.5BoULi'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     339 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4653 sectors
Written to medium : 4653 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
❯ make run
qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d
WARNING: Image format was not specified for 'hdd.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
QEMU 10.2.1 monitor - type 'help' for more information
(qemu) qemu-system-i386: terminating on signal 2 from pid 2889 (<unknown process>)
```
