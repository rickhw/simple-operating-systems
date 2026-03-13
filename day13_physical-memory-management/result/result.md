
```bash
make clean; make
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 2.2s (7/7) FINISHED                                                                             docker:orbstack
=> [internal] load build definition from Dockerfile                                                                    0.0s
=> => transferring dockerfile: 288B                                                                                    0.0s
=> WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
=> [internal] load metadata for docker.io/library/debian:bullseye                                                      2.0s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
In file included from lib/pmm.c:2:
lib/utils.h:9:48: error: unknown type name 'size_t'
   9 | void* memcpy(void* dstptr, const void* srcptr, size_t size);
     |                                                ^~~~~~
lib/utils.h:7:1: note: 'size_t' is defined in header '<stddef.h>'; did you forget to '#include <stddef.h>'?
   6 | #include <stdarg.h>
 +++ |+#include <stddef.h>
   7 | #include <stdbool.h>
lib/utils.h:10:39: error: unknown type name 'size_t'
  10 | void* memset(void* bufptr, int value, size_t size);
     |                                       ^~~~~~
lib/utils.h:10:39: note: 'size_t' is defined in header '<stddef.h>'; did you forget to '#include <stddef.h>'?
lib/pmm.c: In function 'bitmap_find_first_free':
lib/pmm.c:42:31: warning: comparison of integer expressions of different signedness: 'int' and 'uint32_t' {aka 'unsigned int'} [-Wsign-compare]
  42 |                 if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量
     |                               ^~
lib/pmm.c: In function 'init_pmm':
lib/pmm.c:61:5: warning: implicit declaration of function 'memset' [-Wimplicit-function-declaration]
  61 |     memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));
     |     ^~~~~~
lib/pmm.c:4:1: note: 'memset' is defined in header '<string.h>'; did you forget to '#include <string.h>'?
   3 | #include "tty.h"
 +++ |+#include <string.h>
   4 |
make: *** [lib/pmm.o] Error 1
❯ make clean; make
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.0s (7/7) FINISHED                                                                             docker:orbstack
=> [internal] load build definition from Dockerfile                                                                    0.0s
=> => transferring dockerfile: 288B                                                                                    0.0s
=> WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
=> [internal] load metadata for docker.io/library/debian:bullseye                                                      0.8s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
lib/pmm.c: In function 'bitmap_find_first_free':
lib/pmm.c:42:31: warning: comparison of integer expressions of different signedness: 'int' and 'uint32_t' {aka 'unsigned int'} [-Wsign-compare]
  42 |                 if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量
     |                               ^~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/paging.o lib/pmm.o lib/timer.o lib/tty.o lib/utils.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day13_physical-memory-management/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  278g free
Added to ISO image: directory '/'='/tmp/grub.MqdJai'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     339 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4650 sectors
Written to medium : 4650 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
```
