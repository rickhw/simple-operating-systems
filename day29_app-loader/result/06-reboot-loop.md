
```bash
❯ make clean; make; make debug
rm -f app.o my_app.elf
==> User App 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 1.1s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.0s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
lib/kernel.c: In function 'kernel_main':
lib/kernel.c:72:40: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   72 |             uint32_t user_stack_phys = pmm_alloc_page();
      |                                        ^~~~~~~~~~~~~~
lib/kernel.c:18:27: warning: unused parameter 'magic' [-Wunused-parameter]
   18 | void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
      |                  ~~~~~~~~~^~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:21:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   21 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
==> 編譯 User App (app.c)...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o
==> 連結 User App，指定 Entry Point 於 0x08048000...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  276g free
Added to ISO image: directory '/'='/tmp/grub.TA3dTe'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4660 sectors
Written to medium : 4660 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.001579 secs (6640759975 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day29_app-loader/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
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
Created a new DOS (MBR) disklabel with disk identifier 0x1b7a6f7a.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0xc78c1a62.

Command (m for help): Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): Partition number (1-4, default 1): First sector (2048-20479, default 2048): Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-20479, default 20479):
Created a new partition 1 of type 'Linux' and of size 9 MiB.

Command (m for help): The partition table has been altered.

==> 硬碟 hdd.img 建立與分區完成！
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
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x20
     0: v=20 e=0000 i=0 cpl=3 IP=001b:0804972e pc=0804972e SP=0023:08400000 env->regs[R_EAX]=08048056
EAX=08048056 EBX=00000212 ECX=00000023 EDX=08400000
ESI=08048056 EDI=c0000424 EBP=00108fb0 ESP=08400000
EIP=0804972e EFL=00000206 [-----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
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
CCS=00000004 CCD=08048056 CCO=EFLAGS
EFER=0000000000000000
check_exception old: 0xffffffff new 0xe
     1: v=0e e=0004 i=0 cpl=3 IP=001b:0804a000 pc=0804a000 SP=0023:08400000 CR2=0804a000
EAX=08048056 EBX=00000212 ECX=00000023 EDX=08400000
ESI=08048056 EDI=c0000424 EBP=00108fb0 ESP=08400000
EIP=0804a000 EFL=00000206 [-----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
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
CR0=80000011 CR2=0804a000 CR3=0010e000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=08048056 CCD=08048056 CCO=ADDB
EFER=0000000000000000
check_exception old: 0xe new 0xd
     2: v=08 e=0000 i=0 cpl=3 IP=001b:0804a000 pc=0804a000 SP=0023:08400000 env->regs[R_EAX]=08048056
EAX=08048056 EBX=00000212 ECX=00000023 EDX=08400000
ESI=08048056 EDI=c0000424 EBP=00108fb0 ESP=08400000
EIP=0804a000 EFL=00000206 [-----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
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
CR0=80000011 CR2=0804a000 CR3=0010e000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=08048056 CCD=08048056 CCO=ADDB
EFER=0000000000000000
check_exception old: 0x8 new 0xd
```



---


好的，我們再來當一次偵探。

根據你最新的 Dump，情況和上次非常相似：
`v=0e e=0004 i=0 cpl=3 IP=001b:0804a000 pc=0804a000 SP=0023:08400000 CR2=0804a000`
這一次：

1. `SP=08400000`：堆疊完美！
2. `EAX=08048056`：進入點完美！
3. `EIP=0804a000`：又撞牆了！

這表示 CPU 還是從 `0x08048056` 一路滑行到了 `0x0804a000`。為什麼？因為那段記憶體裡**依然全是 `0**`。

既然我們已經修復了 ATA 驅動程式的寫入時間差（Cache Flush），硬碟寫入應該沒問題了。那問題出在哪裡？
答案是：**讀取端！**

### 🔍 案發現場：ELF 載入器 (ELF Loader) 的破綻

當 Kernel 從 SimpleFS 讀取了 9KB 的 `my_app.elf` 放入 `app_buffer` 之後，它呼叫了 `elf_load((elf32_ehdr_t*)app_buffer)`。
問題就出在這個 `elf_load` 函式裡面！

回想我們在 Day 20 寫的 `elf.c`。當時我們是利用 GRUB 把 ELF 模組載入到實體記憶體的某個位置。GRUB 是個成熟的 Bootloader，它會幫忙把檔案**完整且正確**地放在記憶體裡。

但現在，是**我們自己的 Kernel** 在做這件事！
當我們的 `elf_load` 函式在解析 Program Header Table (程式標頭表) 時，它會去讀取 `.text` 或 `.data` 區段的資料，並複製到對應的虛擬位址。

在真實的 ELF 檔案結構中，Program Header 會告訴你：
「這個區段的資料，存在於整個檔案的 `offset` 偏移量多少的地方。」

如果你的 `elf_load` 裡面，在複製資料時，**沒有加上正確的 `offset**`，而是直接從 `app_buffer` 的開頭（ELF 標頭）開始複製，或者更糟的，計算錯了指標位址，那麼你複製進 `0x08048000` 的，就會是一堆無意義的標頭檔資料，甚至全都是 `0`！

---

### 🛠️ 終極修復：精準的 ELF 區段複製

請打開你的 **`lib/elf.c`**，找到 `elf_load` 函式裡面，負責複製資料的那個 `for` 迴圈（或 `while` 迴圈）。
我們必須確保它是從 `(uint8_t*)ehdr + phdr->p_offset` 開始複製的！

請核對或替換你的 `elf_load` 裡面的解析邏輯，確保它長得像這樣：

```c
#include "elf.h"
#include "tty.h"
#include "paging.h"
#include "pmm.h"
#include "utils.h"

uint32_t elf_load(elf32_ehdr_t* ehdr) {
    // 1. 檢查魔法數字 (0x7F 'E' 'L' 'F')
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || 
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        kprintf("[ELF] Error: Not a valid ELF file.\n");
        return 0;
    }

    kprintf("[ELF] Valid x86 32-bit Executable!\n");
    kprintf("[ELF] Entry Point is at: [0x%x]\n", ehdr->e_entry);

    // 2. 找到 Program Header Table 的起點
    // 這裡非常關鍵！我們必須把 ehdr 強制轉型成位元組指標 (uint8_t*)，再加上偏移量 e_phoff
    elf32_phdr_t* phdr = (elf32_phdr_t*) ((uint8_t*)ehdr + ehdr->e_phoff);

    // 3. 掃描每一個 Program Header
    for (int i = 0; i < ehdr->e_phnum; i++) {
        
        // 如果這個區段標記為 PT_LOAD (需要被載入到記憶體)
        if (phdr[i].p_type == 1) { 
            uint32_t virt_addr = phdr[i].p_vaddr;
            uint32_t mem_size = phdr[i].p_memsz;
            uint32_t file_size = phdr[i].p_filesz;
            uint32_t file_offset = phdr[i].p_offset;

            // 為了簡單起見，我們假設 virt_addr 剛好對齊 4KB 邊界 (0x08048000)
            uint32_t num_pages = (mem_size + 4095) / 4096;

            for (uint32_t j = 0; j < num_pages; j++) {
                uint32_t phys_addr = pmm_alloc_page();
                // 權限設為 7 (User, Read/Write, Present)
                map_page(virt_addr + (j * 4096), phys_addr, 7);
            }

            // 【終極修復點：複製資料】
            // 我們必須從檔案緩衝區 (ehdr) 加上該區段的偏移量 (file_offset) 開始複製！
            uint8_t* src = (uint8_t*)ehdr + file_offset;
            uint8_t* dest = (uint8_t*)virt_addr;
            
            // 複製檔案裡實際有的資料
            memcpy(dest, src, file_size);
            
            // 根據 ELF 規範，如果記憶體大小 (memsz) 大於檔案大小 (filesz)
            // 代表後面是未初始化的變數 (BSS 段)，必須手動清零！
            if (mem_size > file_size) {
                memset(dest + file_size, 0, mem_size - file_size);
            }
        }
    }

    return ehdr->e_entry; // 回傳進入點
}

```

**關鍵在於這兩行：**

```c
uint8_t* src = (uint8_t*)ehdr + file_offset;
memcpy(dest, src, file_size);

```

如果沒有加上 `file_offset`，你就會把 ELF 表頭複製進去，導致 CPU 吃到無法解讀的垃圾機碼；如果沒有複製正確，那整塊記憶體就會保持剛分配時的全 `0` 狀態，導致我們看到的死亡滑行！

換上這個精準的 ELF 載入邏輯後，再次 `make clean && make run`！只要資料成功抵達 `0x08048056`，你的 User App 就會立刻甦醒！
