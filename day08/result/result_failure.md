
## First time

```bash
❯ make clean; make
rm -f *.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.5s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.3s
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
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder nasm -f elf32 boot.S -o boot.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -c kernel.c -o kernel.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -c gdt.c -o gdt.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder nasm -f elf32 gdt_flush.S -o gdt_flush.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -c idt.c -o idt.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
idt.c: In function 'pic_remap':
idt.c:38:13: warning: unused variable 'a2' [-Wunused-variable]
   38 |     uint8_t a2 = inb(0xA1);
      |             ^~
idt.c:37:13: warning: unused variable 'a1' [-Wunused-variable]
   37 |     uint8_t a1 = inb(0x21);
      |             ^~
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder nasm -f elf32 interrupts.S -o interrupts.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day08/src.macos:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o gdt.o gdt_flush.o idt.o interrupts.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
ld: idt.o: in function `pic_remap':
idt.c:(.text+0x42): undefined reference to `inb'
ld: idt.c:(.text+0x4e): undefined reference to `inb'
ld: idt.c:(.text+0x59): undefined reference to `outb'
ld: idt.c:(.text+0x67): undefined reference to `outb'
ld: idt.c:(.text+0x72): undefined reference to `outb'
ld: idt.c:(.text+0x80): undefined reference to `outb'
ld: idt.c:(.text+0x8b): undefined reference to `outb'
ld: idt.o:idt.c:(.text+0x99): more undefined references to `outb' follow
ld: idt.o: in function `keyboard_handler':
idt.c:(.text+0xf2): undefined reference to `inb'
ld: idt.c:(.text+0x11b): undefined reference to `outb'
make: *** [myos.bin] Error 1
```



##
