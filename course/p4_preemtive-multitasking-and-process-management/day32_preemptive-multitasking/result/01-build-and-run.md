
```bash
❯ make
docker build -t os-builder .
[+] Building 1.8s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.6s
 => [internal] load .dockerignore                                                                                       0.0s
 => => transferring context: 2B                                                                                         0.0s
 => [1/3] FROM docker.io/library/debian:bullseye@sha256:550c53afe18c60e0dc975327dcb75a4ef8265e247e1190822bf8881a70825f  0.0s
 => CACHED [2/3] RUN apt-get update && apt-get install -y     nasm     build-essential     grub-pc-bin     grub-common  0.0s
 => CACHED [3/3] WORKDIR /workspace                                                                                     0.0s
 => exporting to image                                                                                                  0.0s
 => => exporting layers                                                                                                 0.0s
 => => writing image sha256:4ab04f6c1af7e382dc7a50ee3bab3a2c44aadf15db5f155e13c651c399af54ba                            0.0s
 => => naming to docker.io/library/os-builder                                                                           0.0s

 1 warning found (use docker --debug to expand):
 - FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day32_preemptive-multitasking/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
lib/kernel.c: In function 'kernel_main':
lib/kernel.c:84:37: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   84 |             uint32_t ustack1_phys = pmm_alloc_page();
      |                                     ^~~~~~~~~~~~~~
lib/kernel.c:88:37: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   88 |             uint32_t ustack2_phys = pmm_alloc_page();
      |                                     ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day32_preemptive-multitasking/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day32_preemptive-multitasking/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  274g free
Added to ISO image: directory '/'='/tmp/grub.BDhRwf'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4660 sectors
Written to medium : 4660 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
```
