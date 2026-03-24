
可以正常編譯，不過啟動的時候出現 Kernel Heap Out of Memory 的問題，他說 shell.elf 找不到。我懷疑是 kheap.c 有問題。

我在 Day45 的時候把 app.c 改成 shell.c，然後重構了 Makefile。

底下是相關 Source Code: Makefile, lib/kheap.c, lib/syscall.c, lib/task.c

```bash
❯ make clean; make; make run
rm -f crt0.o *.o *.elf libc/*.o
==> User Apps 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.4s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.2s
 => [internal] load .dockerignore                                                                                       0.0s
 => => transferring context: 2B                                                                                         0.0s
 => [1/3] FROM docker.io/library/debian:bullseye@sha256:943d97fa707482c24e1bc2bdd0b0adc45f75eb345c61dc4272c4157f9a2cc9  0.0s
 => CACHED [2/3] RUN apt-get update && apt-get install -y     nasm     build-essential     grub-pc-bin     grub-common  0.0s
 => CACHED [3/3] WORKDIR /workspace                                                                                     0.0s
 => exporting to image                                                                                                  0.0s
 => => exporting layers                                                                                                 0.0s
 => => writing image sha256:2e718a84f2e8302d405984c3c30d974040605b9ac63e0ac05e677ea5eefbc551                            0.0s
 => => naming to docker.io/library/os-builder                                                                           0.0s

 1 warning found (use docker --debug to expand):
 - FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)
==> 編譯 User App 啟動跳板 (crt0.S)...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 crt0.S -o crt0.o
==> 編譯 User App: shell.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c shell.c -o shell.o
==> 編譯 User App libc 組件: libc/stdio.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/stdio.c -o libc/stdio.o
==> 編譯 User App libc 組件: libc/stdlib.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/stdlib.c -o libc/stdlib.o
==> 編譯 User App libc 組件: libc/syscall.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/syscall.c -o libc/syscall.o
==> 編譯 User App libc 組件: libc/unistd.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/unistd.c -o libc/unistd.o
==> 連結 User App: shell.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o shell.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o shell.elf
==> 編譯 User App: echo.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c echo.c -o echo.o
==> 連結 User App: echo.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o echo.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o echo.elf
==> 編譯 User App: cat.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c cat.c -o cat.o
==> 連結 User App: cat.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o cat.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o cat.elf
==> 編譯 User App: ping.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c ping.c -o ping.o
==> 連結 User App: ping.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o ping.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o ping.elf
==> 編譯 User App: pong.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c pong.c -o pong.o
==> 連結 User App: pong.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o pong.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o pong.elf
==> 編譯 User App: write.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c write.c -o write.o
==> 連結 User App: write.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o write.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o write.elf
==> 編譯 User App: ls.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c ls.c -o ls.o
==> 連結 User App: ls.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o ls.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o ls.elf
==> 編譯 User App: rm.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c rm.c -o rm.o
==> 連結 User App: rm.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o rm.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o rm.elf
==> 編譯 User App: mkdir.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c mkdir.c -o mkdir.o
==> 連結 User App: mkdir.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o mkdir.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o mkdir.elf
==> 編譯 Kernel ASM: asm/boot.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
==> 編譯 Kernel ASM: asm/gdt_flush.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
==> 編譯 Kernel ASM: asm/interrupts.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
==> 編譯 Kernel ASM: asm/paging.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging.S -o asm/paging.o
==> 編譯 Kernel ASM: asm/switch_task.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
==> 編譯 Kernel ASM: asm/user_mode.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
==> 編譯 Kernel Lib: lib/ata.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/ata.c -o lib/ata.o
==> 編譯 Kernel Lib: lib/elf.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/elf.c -o lib/elf.o
==> 編譯 Kernel Lib: lib/gdt.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/gdt.c -o lib/gdt.o
==> 編譯 Kernel Lib: lib/idt.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/idt.c -o lib/idt.o
==> 編譯 Kernel Lib: lib/kernel.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/kernel.c -o lib/kernel.o
In file included from lib/kernel.c:12:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/kernel.c:11:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
==> 編譯 Kernel Lib: lib/keyboard.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/keyboard.c -o lib/keyboard.o
==> 編譯 Kernel Lib: lib/kheap.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:16:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   16 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
==> 編譯 Kernel Lib: lib/mbr.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/mbr.c -o lib/mbr.o
==> 編譯 Kernel Lib: lib/paging.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/paging.c -o lib/paging.o
==> 編譯 Kernel Lib: lib/pmm.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/pmm.c -o lib/pmm.o
==> 編譯 Kernel Lib: lib/simplefs.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/simplefs.c -o lib/simplefs.o
In file included from lib/simplefs.c:1:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/include/simplefs.h:6,
                 from lib/simplefs.c:1:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
lib/simplefs.c: In function 'simplefs_readdir':
lib/simplefs.c:207:74: warning: iteration 24 invokes undefined behavior [-Waggressive-loop-optimizations]
  207 |                 for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
      |                                                       ~~~~~~~~~~~~~~~~~~~^~~
lib/simplefs.c:207:17: note: within this loop
  207 |                 for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
      |                 ^~~
==> 編譯 Kernel Lib: lib/syscall.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/syscall.c -o lib/syscall.o
In file included from lib/syscall.c:4:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/include/simplefs.h:6,
                 from lib/syscall.c:4:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
==> 編譯 Kernel Lib: lib/task.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/task.c -o lib/task.o
In file included from lib/task.c:9:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/include/simplefs.h:6,
                 from lib/task.c:9:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
lib/task.c: In function 'exit_task':
lib/task.c:70:9: warning: implicit declaration of function 'free_page_directory' [-Wimplicit-function-declaration]
   70 |         free_page_directory(current_task->page_directory);
      |         ^~~~~~~~~~~~~~~~~~~
lib/task.c: In function 'schedule':
lib/task.c:111:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
  111 |         switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:20:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   20 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3); // [Day38][change] 加上第三個參數 cr3
      |                                                ~~~~~~~~~~^~~~~~~~
lib/task.c: In function 'create_user_task':
lib/task.c:126:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  126 |         uint32_t heap_phys = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
lib/task.c: In function 'sys_fork':
lib/task.c:182:34: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  182 |     uint32_t child_ustack_phys = pmm_alloc_page();
      |                                  ^~~~~~~~~~~~~~
lib/task.c: In function 'sys_exec':
lib/task.c:266:24: warning: implicit declaration of function 'create_page_directory' [-Wimplicit-function-declaration]
  266 |     uint32_t new_cr3 = create_page_directory();
      |                        ^~~~~~~~~~~~~~~~~~~~~
lib/task.c:286:28: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  286 |     uint32_t ustack_phys = pmm_alloc_page();
      |                            ^~~~~~~~~~~~~~
lib/task.c:293:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  293 |         uint32_t heap_phys = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
==> 編譯 Kernel Lib: lib/timer.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/timer.c -o lib/timer.o
==> 編譯 Kernel Lib: lib/tty.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/tty.c -o lib/tty.o
==> 編譯 Kernel Lib: lib/utils.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/utils.c -o lib/utils.o
==> 編譯 Kernel Lib: lib/vfs.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp *.elf isodir/boot
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  259g free
Added to ISO image: directory '/'='/tmp/grub.6eac9i'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     348 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4712 sectors
Written to medium : 4712 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
rm libc/stdio.o libc/unistd.o libc/syscall.o mkdir.o rm.o shell.o echo.o libc/stdlib.o cat.o write.o pong.o ping.o ls.o
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.001912 secs (5484184100 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day47_mkdir-subdirectories/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
( 1/36) Installing util-linux (2.41.2-r0)
( 2/36) Installing ncurses-terminfo-base (6.5_p20251123-r0)
( 3/36) Installing libncursesw (6.5_p20251123-r0)
( 4/36) Installing dmesg (2.41.2-r0)
( 5/36) Installing setarch (2.41.2-r0)
( 6/36) Installing libeconf (0.8.3-r0)
( 7/36) Installing libblkid (2.41.2-r0)
( 8/36) Installing libuuid (2.41.2-r0)
( 9/36) Installing libfdisk (2.41.2-r0)
(10/36) Installing libmount (2.41.2-r0)
(11/36) Installing libsmartcols (2.41.2-r0)
(12/36) Installing skalibs-libs (2.14.4.0-r0)
(13/36) Installing utmps-libs (0.1.3.1-r0)
(14/36) Installing util-linux-misc (2.41.2-r0)
(15/36) Installing linux-pam (1.7.1-r2)
(16/36) Installing runuser (2.41.2-r0)
(17/36) Installing mount (2.41.2-r0)
(18/36) Installing losetup (2.41.2-r0)
(19/36) Installing hexdump (2.41.2-r0)
(20/36) Installing uuidgen (2.41.2-r0)
(21/36) Installing blkid (2.41.2-r0)
(22/36) Installing sfdisk (2.41.2-r0)
(23/36) Installing mcookie (2.41.2-r0)
(24/36) Installing agetty (2.41.2-r0)
(25/36) Installing wipefs (2.41.2-r0)
(26/36) Installing cfdisk (2.41.2-r0)
(27/36) Installing umount (2.41.2-r0)
(28/36) Installing flock (2.41.2-r0)
(29/36) Installing lsblk (2.41.2-r0)
(30/36) Installing libcap-ng (0.8.5-r0)
(31/36) Installing setpriv (2.41.2-r0)
(32/36) Installing lscpu (2.41.2-r0)
(33/36) Installing logger (2.41.2-r0)
(34/36) Installing partx (2.41.2-r0)
(35/36) Installing fstrim (2.41.2-r0)
(36/36) Installing findmnt (2.41.2-r0)
Executing busybox-1.37.0-r30.trigger
OK: 13.6 MiB in 52 packages

Welcome to fdisk (util-linux 2.41.2).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.

Device does not contain a recognized partition table.
Created a new DOS (MBR) disklabel with disk identifier 0x6a661cc5.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0x88e1915d.

Command (m for help): Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): Partition number (1-4, default 1): First sector (2048-20479, default 2048): Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-20479, default 20479):
Created a new partition 1 of type 'Linux' and of size 9 MiB.

Command (m for help): The partition table has been altered.

==> 硬碟 hdd.img 建立與分區完成！
qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d
WARNING: Image format was not specified for 'hdd.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
QEMU 10.2.2 monitor - type 'help' for more information
(qemu)
```


---

```makefile
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

all: build-env shell.elf echo.elf cat.elf ping.elf pong.elf write.elf ls.elf rm.elf mkdir.elf myos.iso

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
	rm -f crt0.o *.o *.elf libc/*.o
	@echo "==> User Apps 已清除。"

# -----------------------------------------------------------------------------
# --- OS ISO 建置規則 ---
myos.iso: myos.bin grub.cfg shell.elf echo.elf cat.elf ping.elf pong.elf write.elf ls.elf rm.elf mkdir.elf
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp *.elf isodir/boot
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

run: myos.iso hdd.img shell.elf cat.elf ping.elf pong.elf write.elf ls.elf rm.elf mkdir.elf
	qemu-system-i386 -cdrom myos.iso -monitor stdio -hda hdd.img -boot d

debug: myos.iso hdd.img shell.elf cat.elf ping.elf pong.elf write.elf ls.elf rm.elf mkdir.elf
	qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
```

---
lib/kheap.c

```c
#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "tty.h"

block_header_t* first_block = 0;

// --- 公開 API ---

void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // 1. 批發 32 個實體分頁 (32 * 4KB = 128 KB)，這對目前的 OS 來說是一塊超大平原！
    for (int i = 0; i < 32; i++) {
        uint32_t phys_addr = pmm_alloc_page();
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // 2. 在這塊平原的最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (32 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身的大小
    first_block->is_free = 1;
    first_block->next = 0;
}

void* kmalloc(uint32_t size) {
    // 記憶體對齊：為了硬體效能，申請的大小必須是 4 的倍數
    uint32_t aligned_size = (size + 3) & ~3;

    block_header_t* current = first_block;

    while (current != 0) {
        if (current->is_free && current->size >= aligned_size) {
            // 【關鍵修復：切割邏輯】
            // 如果這塊空地很大，我們不能全給他，要把剩下的切出來當新空地！
            if (current->size > aligned_size + sizeof(block_header_t) + 16) {
                // 計算新地契的位置
                block_header_t* new_block = (block_header_t*)((uint32_t)current + sizeof(block_header_t) + aligned_size);
                new_block->is_free = 1;
                new_block->size = current->size - aligned_size - sizeof(block_header_t);
                new_block->next = current->next;

                current->size = aligned_size;
                current->next = new_block;
            }

            // 把地契標記為使用中，並回傳可用空間的起始位址
            current->is_free = 0;
            return (void*)((uint32_t)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    kprintf("PANIC: Kernel Heap Out of Memory! (Req: %d bytes)\n", size);
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return;

    // 往回退一點，找到這塊地的地契，把它標記為空閒
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    block->is_free = 1;

    // (在簡單版 OS 中，我們暫時省略把相鄰空地合併的邏輯，128KB 夠我們隨便花了)
}
```

---

lib/syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

fs_node_t* fd_table[32] = {0};

extern int vfs_create_file(char* filename, char* content);
extern int vfs_readdir(int index, char* out_name, uint32_t* out_size);
extern int vfs_delete_file(char* filename);
extern int vfs_mkdir(char* dirname);

// ==========================================
// IPC 訊息佇列 (Message Queue)
// ==========================================
#define MAX_MESSAGES 16

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

// 環狀佇列 (Circular Buffer)
ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量

// Mutex for Single Processor (SMP)
// 【新增】核心同步鎖：利用開關中斷來保護 Critical Section
void ipc_lock() {
    __asm__ volatile("cli"); // 關閉中斷：誰都別想搶走我的 CPU！
}

void ipc_unlock() {
    __asm__ volatile("sti"); // 開啟中斷：我用完了，大家可以繼續排隊了。
}

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    uint32_t eax = regs->eax;

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;
        fs_node_t* node = simplefs_find(filename);
        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) {
        schedule();
    }
    else if (eax == 7) {
        exit_task();
    }
    else if (eax == 8) {
        regs->eax = sys_fork(regs);
    }
    else if (eax == 9) {
        regs->eax = sys_exec(regs);
    }
    else if (eax == 10) {
        regs->eax = sys_wait(regs->ebx);
    }

    // ==========================================
    // 【新增】IPC 系統呼叫
    // ==========================================
    // Syscall 11: sys_send (傳送訊息)
    else if (eax == 11) {
        char* msg = (char*)regs->ebx;

        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, msg);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else {
            regs->eax = -1; // Queue Full
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
    }
    // Syscall 12: sys_recv (接收訊息)
    else if (eax == 12) {
        char* buffer = (char*)regs->ebx;

        // START OF CRITICAL SECTION
        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else {
            regs->eax = 0;
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
        // End OF CRITICAL SECTION
    }

    // Syscall 13: sbrk (動態記憶體擴充)
    else if (eax == 13) {
        int increment = (int)regs->ebx;
        uint32_t old_end = current_task->heap_end;

        // 把 Heap 的邊界往上推
        current_task->heap_end += increment;

        // 回傳舊的邊界，這就是新分配空間的起始位址！
        regs->eax = old_end;
    }

    // Syscall 14: sys_create (建立/寫入文字檔)
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;

        ipc_lock(); // 上鎖，防止寫入硬碟時被排程器打斷！
        regs->eax = vfs_create_file(filename, content);
        ipc_unlock();
    }
    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;

        ipc_lock();
        regs->eax = vfs_readdir(index, out_name, out_size);
        ipc_unlock();
    }

    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;

        ipc_lock(); // 上鎖！修改硬碟資料是非常危險的操作
        regs->eax = vfs_delete_file(filename);
        ipc_unlock();
    }

    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        ipc_lock();
        regs->eax = vfs_mkdir(dirname);
        ipc_unlock();
    }
}
```

---
lib/task.c
```c
#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;
task_t *idle_task = 0;

extern uint32_t page_directory[]; // [Day38] 取得 Kernel 原始的分頁目錄
// extern void switch_task(uint32_t *current_esp, uint32_t *next_esp); // [Day38][delete]
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3); // [Day38][change] 加上第三個參數 cr3
extern void child_ret_stub();

void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

void init_multitasking() {
    // kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory; // [Day38][Add] 住在原始宇宙

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->id = 9999;
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory; // [Day38][Add] 住在原始宇宙

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop;
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    idle_task->esp = (uint32_t) kstack;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->id) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }

    // [Day46]【新增】如果死的不是老爸 (Kernel 原始宇宙)，就把它專屬的宇宙回收！
    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    current_task->state = TASK_DEAD;
    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            curr->next = next_node->next;
            next_node = curr->next;
        } else {
            curr = next_node;
            next_node = curr->next;
        }
    }

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // switch_task(&prev->esp, &current_task->esp);
        // [Day38][Change]【關鍵】把新任務專屬的宇宙 CR3 傳遞給組合語言！
        switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
    }
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory; // [Day38][Add]

    // ==========================================
    // [Day43]【新增】為初代老爸 (Shell) 預先分配 10 個實體分頁給 User Heap
    // ==========================================
    for (int i = 0; i < 10; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    new_task->heap_end = 0x10000000; // 初始化 Heap 頂點

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // 留給 User Stack 一點呼吸空間
    uint32_t *ustack = (uint32_t*) (user_stack_top - 64);

    *(--ustack) = 0;
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;
    *(--ustack) = 0;
    *(--ustack) = 0;

    *(--kstack) = 0x23;
    *(--kstack) = (uint32_t)ustack;
    *(--kstack) = 0x0202;
    *(--kstack) = 0x1B;
    *(--kstack) = entry_point;

    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}

int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->id = next_task_id++;
    child->state = TASK_RUNNING;
    child->wait_pid = 0;
    child->page_directory = current_task->page_directory; // [Day38][Add] (sys_fork 裡是 child->page_directory)

    // ==========================================
    // [Day43]【新增】讓子行程暫時繼承老爸的 Heap 邊界紀錄
    // ==========================================
    child->heap_end = current_task->heap_end;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    uint32_t child_ustack_base = 0x083FF000 - (child->id * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + 4096) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + 4096) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + 4096) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break;
        }
    }

    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    *(--kstack) = child_ebp;
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // =====================================================================
    // 【打包行李】在切換宇宙前，把參數拷貝到 Kernel Heap，這樣新宇宙才看得到！
    // =====================================================================
    int argc = 0;
    char* safe_argv[16]; // 暫存指標陣列

    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);     // 在 Kernel Heap 申請空間
            memcpy(safe_argv[argc], old_argv[argc], len); // 把字串拷貝過來
            argc++;
        }
    }
    safe_argv[argc] = 0; // 結尾補 NULL

    // =====================================================================
    // 【神聖分離】為這個 Process 建立專屬的新宇宙！
    // =====================================================================
    uint32_t new_cr3 = create_page_directory();

    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    // =====================================================================

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    // 分配新宇宙的 User Stack
    uint32_t clean_user_stack_top = 0x083FF000 + 4096;
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // ==========================================
    // [Day]【新增】預先分配 10 個實體分頁 (40KB) 給 User Heap
    // ==========================================
    for (int i = 0; i < 10; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_end = 0x10000000; // 初始化 Heap 頂點

    uint32_t stack_ptr = clean_user_stack_top - 64;

    // ------------------------------------------
    // 使用「打包好的行李 (safe_argv)」來構造堆疊
    // ------------------------------------------
    uint32_t argv_ptrs[16] = {0};

    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;

            // 拷貝完就可以把 Kernel Heap 的行李丟掉了
            kfree(safe_argv[i]);
        }

        stack_ptr = stack_ptr & ~3;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(int pid) {
    task_t *temp = current_task->next;
    int found = 0;
    while (temp != current_task) {
        if (temp->id == (uint32_t)pid) { found = 1; break; }
        temp = temp->next;
    }
    if (!found) return 0;

    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();
    return 0;
}
```
