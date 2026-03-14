

```bash
❯ ls
asm        Dockerfile gen-elf.sh grub.cfg   lib        linker.ld  Makefile
❯ chmod 755 gen-elf.sh
❯ ./gen-elf.sh
ld: warning: cannot find entry symbol _start; defaulting to 0000000008048000
❯ ls
app.c      app.o      asm        Dockerfile gen-elf.sh grub.cfg   lib        linker.ld  Makefile   my_app.elf
❯ make clean; make
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day19_multiboot-modules/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
asm/boot.S:44: error: label or instruction expected at start of line
make: *** [asm/boot.o] Error 1
    ~/Repos/ai-lab/simple-operating-systems/day19_multiboot-modules/src  on   main ?1 ··············· at 10:17:59 
❯
```
