修改了 simplefs.c 後問題還是存在.

觀察到這些現象：
1. 啟動時 kernel 有安裝 10 個 elf (圖一), ls 看到的檔案數是八個 (圖二)，所以不只是 mkdir.elf 不見了，連 rm.elf 也不見了
2. write file 之後，檔案沒有產生 (圖三)

底下是 build, run, and source code: lib/simplefs.c, lib/include/simplefs.h

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
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.3s
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
Added to ISO image: directory '/'='/tmp/grub.jqk3zf'
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
10485760 bytes transferred in 0.001744 secs (6012477064 bytes/sec)
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
Created a new DOS (MBR) disklabel with disk identifier 0x7040843b.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0xb7e309a9.

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
(qemu) qemu-system-i386: terminating on signal 2 from pid 1011 (<unknown process>)

❯ make clean-disk
rm -f hdd.img
==> 硬碟已刪除，下次 make run 將會產生一顆全新的硬碟。
❯ make run
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.002446 secs (4286901063 bytes/sec)
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
Created a new DOS (MBR) disklabel with disk identifier 0x7f29d127.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0x7d548e22.

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

lib/simplefs.c
```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

uint32_t mounted_part_lba = 0; // 記錄目前掛載的分區起點

// --- 公開 API ---

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
    kprintf("[SimpleFS] Mounted at LBA [%d].\n", part_lba);
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    // kprintf("[SimpleFS] Formatting partition starting at LBA [%d].\n", partition_start_lba);

    // 1. 準備 Superblock
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;     // 根目錄在分區的第 1 個相對 LBA (實體為 partition_start_lba + 1)
    sb->data_start_lba = 2;   // 資料區從第 2 個相對 LBA 開始

    // 寫入 Superblock 到分區的起點
    ata_write_sector(partition_start_lba, (uint8_t*)sb);
    // kprintf("[SimpleFS] 1) Superblock written; ");

    // 2. 準備空白的根目錄 (填滿 0 即可，代表沒有任何檔案)
    uint8_t* empty_dir = (uint8_t*) kmalloc(512);
    memset(empty_dir, 0, 512);

    // 寫入根目錄到 Superblock 的下一個磁區
    ata_write_sector(partition_start_lba + sb->root_dir_lba, empty_dir);
    // kprintf("2) Empty root directory created.\n");

    // 清理記憶體
    kfree(sb);
    kfree(empty_dir);

    kprintf("[SimpleFS] Format complete, at LBA [%d].\n", partition_start_lba);
}

// 列出檔案
void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n[SimpleFS] List Root Directory\n");

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
                    entries[i].filename,
                    entries[i].file_size,
                    entries[i].start_lba);
            file_count++;
        }
    }

    if (file_count == 0) {
        kprintf("(Directory is empty)\n");
    }
    kprintf("-------------------------------\n");

    kfree(dir_buf);
}

// [修改] 把原本的第一個參數拿掉，直接使用 mounted_part_lba
fs_node_t* simplefs_find(char* filename) {
    if (mounted_part_lba == 0) return 0; // 還沒掛載！

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(mounted_part_lba + 1, dir_buf);
    // ... 下面的尋找邏輯完全不變，只需要把 part_lba 換成 mounted_part_lba

    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    int max_entries = 512 / sizeof(sfs_file_entry_t);
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            fs_node_t* node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
            strcpy(node->name, entries[i].filename);
            node->flags = 1;
            node->length = entries[i].file_size;
            node->inode = entries[i].start_lba;
            node->impl = mounted_part_lba;
            node->read = simplefs_read;
            node->write = 0;
            kfree(dir_buf);
            return node;
        }
    }
    kfree(dir_buf);
    return 0;
}

// 寫入檔案
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    // 因為新結構是 64 bytes，512/64 = 8 個項目
    int max_entries = 512 / sizeof(sfs_file_entry_t);

    int target_idx = -1;
    uint32_t next_data_lba = 2;

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] == '\0') {
            // 記下第一個遇到的空位
            if (target_idx == -1) target_idx = i;
        } else {
            // 【新增：覆寫機制】如果檔名已經存在，就直接覆寫它！
            if (strcmp(entries[i].filename, filename) == 0) {
                target_idx = i;
            }

            // 計算目前硬碟用到哪裡了
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (target_idx == -1) { kfree(dir_buf); return -1; } // 目錄滿了

    // --- 寫入實際資料到硬碟 ---
    uint32_t sectors_needed = (size + 511) / 512;
    // 如果 size 是 0 (像是建立空目錄)，我們就不需要寫入 data block
    if (sectors_needed > 0) {
        for (uint32_t i = 0; i < sectors_needed; i++) {
            uint8_t temp[512] = {0};
            uint32_t copy_size = 512;
            if ((i * 512) + 512 > size) {
                copy_size = size - (i * 512);
            }
            memcpy(temp, data + (i * 512), copy_size);
            // 這裡如果是覆寫，最完美的作法是重複利用舊的 lba，但為了簡單，我們直接寫到硬碟最尾端
            ata_write_sector(part_lba + next_data_lba + i, temp);
        }
    }

    // --- 更新地契 (Directory Entry) ---
    memset(entries[target_idx].filename, 0, 32); // 【修正 1】清空完整的 32 bytes！
    strcpy(entries[target_idx].filename, filename);
    entries[target_idx].start_lba = next_data_lba;
    entries[target_idx].file_size = size;
    entries[target_idx].type = FS_FILE;          // 【修正 2】賦予檔案靈魂！

    ata_write_sector(part_lba + 1, dir_buf);
    kfree(dir_buf);
    return 0;
}

// 讀取大檔案
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t file_lba = node->impl + node->inode;
    uint32_t actual_size = size;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) actual_size = node->length - offset;

    // 計算需要跨越的磁區範圍
    uint32_t start_sector = offset / 512;
    uint32_t end_sector = (offset + actual_size - 1) / 512;
    uint32_t sector_offset = offset % 512;

    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    uint32_t bytes_read = 0;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        ata_read_sector(file_lba + i, temp_buf);

        uint32_t copy_size = 512 - sector_offset;
        if (bytes_read + copy_size > actual_size) {
            copy_size = actual_size - bytes_read;
        }

        memcpy(buffer + bytes_read, temp_buf + sector_offset, copy_size);
        bytes_read += copy_size;
        sector_offset = 0; // 只有第一個磁區可能不是從 0 開始讀
    }

    kfree(temp_buf);
    return bytes_read;
}

// 【新增】提供給 Syscall 呼叫的友善封裝
int vfs_create_file(char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;

    int len = strlen(content);
    // 呼叫我們之前寫好的底層建立檔案功能
    simplefs_create_file(mounted_part_lba, filename, content, len);
    return 0;
}


// [Day45] add -- start
// 【新增】讀取第 index 個檔案的資訊
int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    // 假設我們的根目錄位在 part_lba + 1 (這取決於你當時 simplefs_format 的設計)
    // 通常根目錄就是一個 block，我們把它讀出來
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取第一個目錄磁區

    // 【轉換】直接把一整塊記憶體轉型成結構陣列！
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t);

    int valid_count = 0;
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                // 現在編譯器會精準抓到正確的 offset，再也不會讀到隔壁的字串了！
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;

                kfree(dir_buffer);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buffer);
    return 0;
}

// 【新增】VFS 層的封裝
int vfs_readdir(int index, char* out_name, uint32_t* out_size) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size);
}

// [Day45] add -- end


// [Day46] add -- start
// 【新增】實作檔案刪除邏輯
int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取目錄磁區

    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t);

    for (int i = 0; i < max_entries; i++) {
        // 如果這個格子有檔案，而且名字完全符合
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            // 【死亡宣告】把檔名第一個字元變成 0，代表這個格子空出來了！
            entries[i].filename[0] = '\0';

            // 【極度重要】把修改後的目錄，重新寫回實體硬碟！
            ata_write_sector(part_lba + 1, dir_buffer);

            kfree(dir_buffer);
            return 0; // 刪除成功
        }
    }

    kfree(dir_buffer);
    return -1; // 找不到檔案
}

// 【新增】VFS 層的封裝
int vfs_delete_file(char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(mounted_part_lba, filename);
}
// [Day46] add -- end


// [Day47] add -- start
// 【新增】建立目錄的底層實作
int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer);

    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t); // 512 / 64 = 8 個項目

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] == '\0') { // 找到空位了！
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR; // 標記為目錄！

            // 給它分配一個新的磁區來存放它未來的子檔案清單
            // (簡單起見，我們借用一個尚未使用的 LBA，例如 part_lba + 20 + i)
            entries[i].start_lba = part_lba + 20 + i;

            ata_write_sector(part_lba + 1, dir_buffer); // 寫回硬碟
            kfree(dir_buffer);
            return 0; // 成功
        }
    }
    kfree(dir_buffer);
    return -1; // 目錄滿了
}

// VFS 封裝
int vfs_mkdir(char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(mounted_part_lba, dirname);
}
// [Day47] add -- end

```

---
lib/include/simplefs.h
```c

#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include "vfs.h"

// SimpleFS 的魔法數字 ("SFS!" 的十六進位)
#define SIMPLEFS_MAGIC 0x21534653

// 定義檔案型態
#define FS_FILE 0
#define FS_DIR  1

// Superblock 結構 (佔用一個 512 bytes 磁區，但只用前面幾個 bytes)
typedef struct {
    uint32_t magic;         // 證明這是一個 SimpleFS 分區
    uint32_t total_blocks;  // 分區總容量 (以 512 bytes 磁區為單位)
    uint32_t root_dir_lba;  // 根目錄所在的相對 LBA
    uint32_t data_start_lba;// 檔案資料區開始的相對 LBA
    uint8_t  padding[496];  // 補齊到 512 bytes
} __attribute__((packed)) sfs_superblock_t;

// 檔案目錄項目結構
typedef struct {
    char     filename[32];  // 檔案名稱 (包含結尾 \0)
    uint32_t start_lba;     // 檔案內容開始的相對 LBA
    uint32_t file_size;     // 檔案大小 (Bytes)
    uint32_t type;         // 【新增】檔案型態：0=檔案, 1=目錄 (4 bytes)
    uint32_t reserved[5];  // 保留空間，湊滿 64 bytes 完美對齊 (20 bytes)
} __attribute__((packed)) sfs_file_entry_t;

// disk utils
// 格式化指定的分區
void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);
void simplefs_mount(uint32_t part_lba);

// 列出目錄下的所有檔案 (類似 ls 指令)
void simplefs_list_files(uint32_t part_lba);
fs_node_t* simplefs_find(char* filename);

// operate for files
// 建立檔案並寫入資料
// 為了簡單起見，我們這版先限制檔案大小不能超過 512 bytes (一個磁區)
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size);
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
```
