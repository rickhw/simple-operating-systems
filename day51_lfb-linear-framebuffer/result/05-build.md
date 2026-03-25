
```bash
❯ make clean; make
rm -f crt0.o *.o *.elf libc/*.o
==> User Apps 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.5s (7/7) FINISHED                                                                             docker:orbstack
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 crt0.S -o crt0.o
==> 編譯 User App: shell.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c shell.c -o shell.o
==> 編譯 User App libc 組件: libc/stdio.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/stdio.c -o libc/stdio.o
==> 編譯 User App libc 組件: libc/stdlib.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/stdlib.c -o libc/stdlib.o
==> 編譯 User App libc 組件: libc/syscall.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/syscall.c -o libc/syscall.o
==> 編譯 User App libc 組件: libc/unistd.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c libc/unistd.c -o libc/unistd.o
==> 連結 User App: shell.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o shell.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o shell.elf
==> 編譯 User App: echo.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c echo.c -o echo.o
==> 連結 User App: echo.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o echo.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o echo.elf
==> 編譯 User App: cat.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c cat.c -o cat.o
==> 連結 User App: cat.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o cat.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o cat.elf
==> 編譯 User App: ping.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c ping.c -o ping.o
==> 連結 User App: ping.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o ping.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o ping.elf
==> 編譯 User App: pong.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c pong.c -o pong.o
==> 連結 User App: pong.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o pong.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o pong.elf
==> 編譯 User App: touch.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c touch.c -o touch.o
==> 連結 User App: touch.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o touch.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o touch.elf
==> 編譯 User App: ls.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c ls.c -o ls.o
==> 連結 User App: ls.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o ls.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o ls.elf
==> 編譯 User App: rm.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c rm.c -o rm.o
==> 連結 User App: rm.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o rm.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o rm.elf
==> 編譯 User App: mkdir.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -Ilibc/include -c mkdir.c -o mkdir.o
==> 連結 User App: mkdir.elf ...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 crt0.o mkdir.o libc/stdio.o libc/stdlib.o libc/syscall.o libc/unistd.o -o mkdir.elf
==> 編譯 Kernel ASM: asm/boot.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
==> 編譯 Kernel ASM: asm/gdt_flush.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
==> 編譯 Kernel ASM: asm/interrupts.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
==> 編譯 Kernel ASM: asm/paging.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging.S -o asm/paging.o
==> 編譯 Kernel ASM: asm/switch_task.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
==> 編譯 Kernel ASM: asm/user_mode.S
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
==> 編譯 Kernel Lib: lib/ata.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/ata.c -o lib/ata.o
==> 編譯 Kernel Lib: lib/elf.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/elf.c -o lib/elf.o
==> 編譯 Kernel Lib: lib/gdt.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/gdt.c -o lib/gdt.o
==> 編譯 Kernel Lib: lib/gfx.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/gfx.c -o lib/gfx.o
==> 編譯 Kernel Lib: lib/idt.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/idt.c -o lib/idt.o
==> 編譯 Kernel Lib: lib/kernel.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/kernel.c -o lib/kernel.o
In file included from lib/kernel.c:12:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/kernel.c:11:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
==> 編譯 Kernel Lib: lib/keyboard.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/keyboard.c -o lib/keyboard.o
==> 編譯 Kernel Lib: lib/kheap.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/kheap.c -o lib/kheap.o
==> 編譯 Kernel Lib: lib/mbr.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/mbr.c -o lib/mbr.o
==> 編譯 Kernel Lib: lib/paging.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/paging.c -o lib/paging.o
==> 編譯 Kernel Lib: lib/pmm.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/pmm.c -o lib/pmm.o
==> 編譯 Kernel Lib: lib/simplefs.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/simplefs.c -o lib/simplefs.o
In file included from lib/simplefs.c:1:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/include/simplefs.h:6,
                 from lib/simplefs.c:1:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
lib/simplefs.c: In function 'shallow_get_dir_lba':
lib/simplefs.c:47:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
   47 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_list_files':
lib/simplefs.c:134:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  134 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_find':
lib/simplefs.c:156:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  156 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_create_file':
lib/simplefs.c:187:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  187 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_readdir':
lib/simplefs.c:255:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  255 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_delete_file':
lib/simplefs.c:281:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  281 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_mkdir':
lib/simplefs.c:303:23: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  303 |     for (int i = 0; i < MAX_FILES; i++) {
      |                       ^
lib/simplefs.c: In function 'simplefs_getcwd':
lib/simplefs.c:388:27: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  388 |         for (int i = 0; i < MAX_FILES; i++) {
      |                           ^
lib/simplefs.c:403:27: warning: comparison of integer expressions of different signedness: 'int' and 'unsigned int' [-Wsign-compare]
  403 |         for (int i = 0; i < MAX_FILES; i++) {
      |                           ^
==> 編譯 Kernel Lib: lib/syscall.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/syscall.c -o lib/syscall.o
In file included from lib/syscall.c:4:
lib/include/simplefs.h:12: warning: "FS_FILE" redefined
   12 | #define FS_FILE 0
      |
In file included from lib/include/simplefs.h:6,
                 from lib/syscall.c:4:
lib/include/vfs.h:8: note: this is the location of the previous definition
    8 | #define FS_FILE        0x01   // 一般檔案
      |
lib/syscall.c: In function 'syscall_handler':
lib/syscall.c:183:21: warning: implicit declaration of function 'simplefs_readdir'; did you mean 'simplefs_read'? [-Wimplicit-function-declaration]
  183 |         regs->eax = simplefs_readdir(current_dir, index, out_name, out_size, out_type);
      |                     ^~~~~~~~~~~~~~~~
      |                     simplefs_read
==> 編譯 Kernel Lib: lib/task.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/task.c -o lib/task.o
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
lib/task.c:72:9: warning: implicit declaration of function 'free_page_directory' [-Wimplicit-function-declaration]
   72 |         free_page_directory(current_task->page_directory);
      |         ^~~~~~~~~~~~~~~~~~~
lib/task.c: In function 'schedule':
lib/task.c:113:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
  113 |         switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:20:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   20 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3); // [Day38][change] 加上第三個參數 cr3
      |                                                ~~~~~~~~~~^~~~~~~~
lib/task.c: In function 'create_user_task':
lib/task.c:129:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  129 |         uint32_t heap_phys = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
lib/task.c: In function 'sys_fork':
lib/task.c:186:34: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  186 |     uint32_t child_ustack_phys = pmm_alloc_page();
      |                                  ^~~~~~~~~~~~~~
lib/task.c: In function 'sys_exec':
lib/task.c:281:24: warning: implicit declaration of function 'create_page_directory' [-Wimplicit-function-declaration]
  281 |     uint32_t new_cr3 = create_page_directory();
      |                        ^~~~~~~~~~~~~~~~~~~~~
lib/task.c:301:28: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  301 |     uint32_t ustack_phys = pmm_alloc_page();
      |                            ^~~~~~~~~~~~~~
lib/task.c:308:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  308 |         uint32_t heap_phys = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
==> 編譯 Kernel Lib: lib/timer.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/timer.c -o lib/timer.o
==> 編譯 Kernel Lib: lib/tty.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/tty.c -o lib/tty.o
==> 編譯 Kernel Lib: lib/utils.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/utils.c -o lib/utils.o
==> 編譯 Kernel Lib: lib/vfs.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/gfx.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp *.elf isodir/boot
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  255g free
Added to ISO image: directory '/'='/tmp/grub.y0ddAh'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     348 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4714 sectors
Written to medium : 4714 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
rm libc/stdio.o libc/unistd.o libc/syscall.o mkdir.o rm.o shell.o touch.o echo.o libc/stdlib.o cat.o pong.o ping.o ls.o
❯ make run
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.001943 secs (5396685538 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day51_lfb-linear-framebuffer/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
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
Created a new DOS (MBR) disklabel with disk identifier 0x163b410f.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0x909f1bdc.

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
```
