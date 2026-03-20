
```bash
❯ make clean
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
❯ make
docker build -t os-builder .
[+] Building 1.6s (7/7) FINISHED                                                                                                      docker:orbstack
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
lib/kernel.c: In function 'kernel_main':
lib/kernel.c:19:27: warning: unused parameter 'magic' [-Wunused-parameter]
   19 | void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
      |                  ~~~~~~~~~^~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
lib/syscall.c: In function 'syscall_handler':
lib/syscall.c:16:31: warning: unused parameter 'edi' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                      ~~~~~~~~~^~~
lib/syscall.c:16:45: warning: unused parameter 'esi' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                    ~~~~~~~~~^~~
lib/syscall.c:16:59: warning: unused parameter 'ebp' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                                  ~~~~~~~~~^~~
lib/syscall.c:16:73: warning: unused parameter 'esp' [-Wunused-parameter]
   16 | void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
      |                                                                ~~~~~~~~~^~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  279g free
Added to ISO image: directory '/'='/tmp/grub.htsQHg'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4662 sectors
Written to medium : 4662 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
```

```bash
❯ make run
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.001915 secs (5475592689 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day28_file-descriptor/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
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
Created a new DOS (MBR) disklabel with disk identifier 0x0c4de922.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0xf8ad9ef9.

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
QEMU 10.2.1 monitor - type 'help' for more information
(qemu) %
```


```bash
❯ make debug
qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
WARNING: Image format was not specified for 'hdd.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
SMM: enter
EAX=00000000 EBX=00000000 ECX=02000000 EDX=02000628
ESI=0000000b EDI=02000000 EBP=06feb010 ESP=00006d5c
EIP=000ebede EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=ffffffff CCO=EFLAGS
EFER=0000000000000000
SMM: after RSM
EAX=00000000 EBX=00000000 ECX=02000000 EDX=02000628
ESI=0000000b EDI=02000000 EBP=06feb010 ESP=00006d5c
EIP=000ebede EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=00006cff
ESI=00006cb8 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 008f9300
CS =f000 000f0000 ffffffff 008f9b00
SS =0000 00000000 ffffffff 008f9300
DS =0000 00000000 ffffffff 008f9300
FS =0000 00000000 ffffffff 008f9300
GS =0000 00000000 ffffffff 008f9300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006c78 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=00006cff
ESI=00006cb8 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=06fffe88
ESI=000e9780 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=000f7d44 EFL=00000012 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006c64 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=06fffe88
ESI=000e9780 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =0000 00000000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d2a EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 008f9300
CS =f000 000f0000 ffffffff 008f9b00
SS =0000 00000000 ffffffff 008f9300
DS =0000 00000000 ffffffff 008f9300
FS =0000 00000000 ffffffff 008f9300
GS =ca00 000ca000 ffffffff 008f9300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006962 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=0000694e CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d47 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=0000695c CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06fd1fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006948 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06fd1fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d2a EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006962 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=0000694e CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d47 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=0000695c CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06f31fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006948 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06f31fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x09
     0: v=80 e=0000 i=1 cpl=3 IP=001b:0804806c pc=0804806c SP=0023:00108f2c env->regs[R_EAX]=00000002
EAX=00000002 EBX=08049000 ECX=08048074 EDX=08049000
ESI=08048074 EDI=00000000 EBP=00108f30 ESP=00108f2c
EIP=0804806c EFL=00000216 [----AP-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00109000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00109080 0000002f
IDT=     001090e0 000007ff
CR0=80000011 CR2=00000000 CR3=0010e000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00002fa3 CCD=0804b000 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xffffffff new 0xe
     1: v=0e e=0002 i=0 cpl=3 IP=001b:0804806c pc=0804806c SP=0023:00108f2c CR2=fffffffc
EAX=00000002 EBX=08049000 ECX=08048074 EDX=08049000
ESI=08048074 EDI=00000000 EBP=00108f30 ESP=00108f2c
EIP=0804806c EFL=00000216 [----AP-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00109000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00109080 0000002f
IDT=     001090e0 000007ff
CR0=80000011 CR2=fffffffc CR3=0010e000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00002fa3 CCD=0804b000 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xe new 0xd
     2: v=08 e=0000 i=0 cpl=3 IP=001b:0804806c pc=0804806c SP=0023:00108f2c env->regs[R_EAX]=00000002
EAX=00000002 EBX=08049000 ECX=08048074 EDX=08049000
ESI=08048074 EDI=00000000 EBP=00108f30 ESP=00108f2c
EIP=0804806c EFL=00000216 [----AP-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00109000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00109080 0000002f
IDT=     001090e0 000007ff
CR0=80000011 CR2=fffffffc CR3=0010e000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00002fa3 CCD=0804b000 CCO=ADDL
EFER=0000000000000000
check_exception old: 0x8 new 0xd
```



---

這真的是作業系統開發中最經典、也最有深度的當機畫面！你所遇到的這個錯誤，代表我們的 Simple OS 正正式式地撞上了一道「現代作業系統保護機制」的隱形牆。

我們來當一次系統法醫，解讀你的 Debug Log：

1. `check_exception old: 0xffffffff new 0xe`：`0xE` 是十進位的 14，也就是大名鼎鼎的 **Page Fault (分頁錯誤)**！
2. `CR2=fffffffc`：這個暫存器記錄了「案發地點」。代表 CPU 試圖存取 `0xFFFFFFFC` 這個記憶體位址，但是失敗了。
3. `CPL=3`：這代表案發當時，CPU 是在 **Ring 3 (平民模式)** 運作的。

### 🔍 案發現場解密：為什麼會 Page Fault？

回想一下我們在 Day 20 做的「虛實縫合」。我們用 `map_page` 把 `my_app.elf` 的程式碼映射到了 `0x08048000`。
**但是，我們從來沒有給這個應用程式「堆疊 (Stack)」！**

在 Day 20 時，`app.c` 裡面只有幾行簡單的 Inline ASM，沒有宣告任何區域變數，也沒有呼叫函式，所以它不需要堆疊就能活下來（即使當時我們作弊，直接把 Kernel 的 `esp` 丟給它用）。

可是到了 Day 28，你的 `app.c` 裡面出現了這個：

```c
char buffer[128]; // 區域變數！必須放在堆疊上！
sys_read(fd, buffer, 100); // 函式呼叫！必須把回傳位址 push 到堆疊上！

```

當 Ring 3 的應用程式試圖去使用堆疊時，發現手上的 `esp` 指標要嘛指向了 Kernel 的記憶體（Ring 3 無權寫入），要嘛就是一片空白（`0x0`，減 4 後變成 `0xFFFFFFFC`），於是當場觸發 Page Fault，接著因為沒有寫 Ring 3 的例外處理機制，直接引發 Double Fault (`new 0x8`) 並重開機。

---

### 🛠️ 終極修復：賜予平民專屬的記憶體堆疊

這是一個非常重要的里程碑：我們必須在 Kernel 載入程式時，合法地配置一塊記憶體當作 User App 的專屬堆疊！

#### 第一步：修改 `asm/user_mode.S`

我們需要讓 `enter_user_mode` 能夠接收「第二個參數」：也就是我們準備好的堆疊指標 (Stack Pointer)。

請打開 **`asm/user_mode.S`**，修改如下：

```nasm
global enter_user_mode
enter_user_mode:
    mov ebp, esp
    mov eax, [ebp+4]  ; 參數 1: entry_point (應用程式進入點)
    mov edx, [ebp+8]  ; 參數 2: user_stack_ptr (我們準備好的平民堆疊)

    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23         ; SS (User Data Segment)
    push edx          ; ESP (使用我們剛剛拿到的 user_stack_ptr ！)
    pushfd
    pop ebx
    or ebx, 0x200     ; 開啟 IF 旗標 (允許中斷)
    push ebx
    push 0x1b         ; CS (User Code Segment)
    push eax          ; EIP
    iret

```

#### 第二步：在 Kernel 中分配並映射堆疊

打開 **`lib/kernel.c`**，找到我們準備跳入 Ring 3 的那段程式碼。
還記得我們在 Day 20 寫 `paging.c` 時，為 User App 準備了 `user_page_table` 嗎？它負責管理 128MB 開始的 4MB 空間（`0x08000000` ~ `0x083FFFFF`）。我們就把堆疊放在這個合法範圍的最後面！

修改 `lib/kernel.c` 的跳轉邏輯：

```c
        kprintf("\nLoading ELF Application...\n");
        uint32_t entry_point = elf_load(real_app);
        
        if (entry_point != 0) {
            kprintf("Allocating User Stack and Dropping to Ring 3...\n\n");
            
            // 1. 從 PMM 申請一塊 4KB 的實體記憶體當作堆疊
            uint32_t user_stack_phys = pmm_alloc_page();
            
            // 2. 將這塊實體記憶體，映射到 0x083FF000 
            // (這剛好落在我們 user_page_table 管理的最後一個分頁！權限 7 代表 User RW)
            map_page(0x083FF000, user_stack_phys, 7);
            
            // 3. 堆疊是由高位址往低位址生長，所以初始指標要放在這 4KB 的「最頂端」
            uint32_t user_stack_ptr = 0x083FF000 + 4096;
            
            // 4. 呼叫我們升級版的進入函式
            extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);
            enter_user_mode(entry_point, user_stack_ptr);
        }

```

---

### 重新出發

這是一個作業系統最美麗的設計模式：**Code 放在低位址（`0x08048000`），Stack 放在高位址（`0x08400000` 往下長）**，兩者互不干擾。這就是 Linux 在做的事情！

請儲存這兩個檔案，再次執行：

```bash
make clean && make run

```

加上了這塊專屬的 User Stack，你的應用程式就能盡情地宣告區域變數、盡情地呼叫系統服務了！畫面印出 `hello.txt` 的內容後記得跟我說，我們隨時可以進入 Day 29！
