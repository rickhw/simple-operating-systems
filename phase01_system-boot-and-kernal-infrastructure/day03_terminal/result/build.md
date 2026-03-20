
```bash
❯ cd day03
❯ ls
Course.md src.macos
❯ cd src.macos
❯ ls
boot.S     Dockerfile grub.cfg   kernel.c   linker.ld  Makefile
❯ make clean
rm -f *.o *.bin *.iso
rm -rf isodir
❯ make
docker build -t os-builder .
[+] Building 1.9s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.8s
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
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day03/src.macos:/workspace -w /workspace os-builder nasm -f elf32 boot.S -o boot.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day03/src.macos:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -c kernel.c -o kernel.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day03/src.macos:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
docker run --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day03/src.macos:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
WARNING: The requested image's platform (linux/amd64) does not match the detected host platform (linux/arm64/v8) and no specific platform was requested
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  275g free
Added to ISO image: directory '/'='/tmp/grub.VWkJ0g'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     339 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4649 sectors
Written to medium : 4649 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir

❯ qemu-system-i386 -cdrom myos.iso
```
