跑起來後，他停在 "Kernel dropping to idle state. Have fun!"，下一步則是 schedule()，然後就不動了，我敲什麼都沒有用。

這次一樣附上 dump 以及大部分的 code, 看看可能是哪裡的問題.

asm:
  - interrupts.S

```bash
❯ make clean
rm -f app.o my_app.elf
==> User App 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
❯ make
docker build -t os-builder .
[+] Building 1.2s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.0s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging.S -o asm/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:16:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   16 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/task.c -o lib/task.o
lib/task.c: In function 'schedule':
lib/task.c:81:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
   81 |         switch_task(&prev->esp, &current_task->esp);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:17:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   17 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
      |                                                ~~~~~~~~~~^~~~~~~~
lib/task.c: In function 'sys_fork':
lib/task.c:130:34: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  130 |     uint32_t child_ustack_phys = pmm_alloc_page();
      |                                  ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
==> 編譯 User App (app.c)...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o
==> 連結 User App，指定 Entry Point 於 0x08048000...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  269g free
Added to ISO image: directory '/'='/tmp/grub.c2Qfgh'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4663 sectors
Written to medium : 4663 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir
❯ make run
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.002037 secs (5147648503 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day35_sys-exec/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
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
Created a new DOS (MBR) disklabel with disk identifier 0x3a0ddba9.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0x871ce3be.

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
(qemu) qemu-system-i386: terminating on signal 2 from pid 10254 (<unknown process>)

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
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x21
     0: v=21 e=0000 i=0 cpl=0 IP=0008:00101790 pc=00101790 SP=0010:00109fc8 env->regs[R_EAX]=c0000030
EAX=c0000030 EBX=c000000c ECX=00000000 EDX=c0000030
ESI=c000000c EDI=00105084 EBP=00000000 ESP=00109fc8
EIP=00101790 EFL=00000297 [--S-APC] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010a000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010a080 0000002f
IDT=     0010a0e0 000007ff
CR0=80000011 CR2=00000000 CR3=0010f000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000001 CCD=ffffffff CCO=SUBL
EFER=0000000000000000
Servicing hardware INT=0x20
     1: v=20 e=0000 i=0 cpl=0 IP=0008:00101790 pc=00101790 SP=0010:00109fc8 env->regs[R_EAX]=c0000030
EAX=c0000030 EBX=c000000c ECX=00000000 EDX=c0000030
ESI=c000000c EDI=00105084 EBP=00000000 ESP=00109fc8
EIP=00101790 EFL=00000297 [--S-APC] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010a000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010a080 0000002f
IDT=     0010a0e0 000007ff
CR0=80000011 CR2=00000000 CR3=0010f000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000095 CCD=ffffffff CCO=EFLAGS
EFER=0000000000000000
^Cqemu-system-i386: terminating on signal 2 from pid 10254 (<unknown process>)
```


---
interrupts.S
```asm
global idt_flush
global isr0
extern isr0_handler
global isr32
extern timer_handler
global isr33
extern keyboard_handler
global isr128
extern syscall_handler

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

isr0:
    cli
    call isr0_handler
    sti
    iret

isr32:
    pusha
    call timer_handler
    popa
    iret

isr33:
    pusha
    call keyboard_handler
    popa
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    ; 魔法：syscall_handler 已經把回傳值寫進堆疊的 EAX 位置了
    popa            ; 恢復所有暫存器
    iret

```

---
paging.S
```asm
global load_page_directory
global enable_paging

; void load_page_directory(uint32_t* dir);
load_page_directory:
    mov eax, [esp+4]  ; 取得 C 語言傳進來的 page_directory 陣列位址
    mov cr3, eax      ; 將位址存入 CR3 暫存器
    ret

; void enable_paging(void);
enable_paging:
    mov eax, cr0      ; 讀取目前的 CR0 狀態
    or eax, 0x80000000; 使用 OR 運算，把最高位 (PG bit 31) 強制設為 1
    mov cr0, eax      ; 寫回 CR0，虛擬記憶體機制瞬間啟動！
    ret
```

---
switch_task.S
```asm
[bits 32]
global switch_task
global child_ret_stub ; 暴露給 task.c 使用

switch_task:
    pusha
    pushf
    mov eax, [esp + 40]
    mov [eax], esp
    mov eax, [esp + 44]
    mov esp, [eax]
    popf
    popa
    ret

; ==========================================
; 【終極降落傘】純組合語言實作，保證絕不弄髒暫存器！
; ==========================================
child_ret_stub:
    ; 剛從 switch_task 的 ret 跳過來，此時 ESP 指向假裝的返回位址
    add esp, 4      ; 丟棄假返回位址

    ; 精準彈出我們在 task.c 裡手工對齊的 4 個暫存器
    pop edi
    pop esi
    pop ebx
    pop ebp       ; 這裡拿回了完美修正過的 EBP！

    ; 換上平民服裝 (User Data Segment = 0x23)
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    mov eax, 0    ; 【核心魔法】小孩拿到回傳值 0！

    ; 此時 ESP 剛好指著我們手工準備好的 IRET 幀 (EIP, CS, EFLAGS, ESP, SS)
    iret
```

---
user_mode.S
```asm
global enter_user_mode
enter_user_mode:
    ; 標準的 C 語言函式開頭
    push ebp
    mov ebp, esp

    ; 因為 push 了 ebp，參數的位移要 +4
    mov eax, [ebp+8]  ; 參數 1: entry_point
    mov edx, [ebp+12] ; 參數 2: user_stack_ptr

    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23         ; SS (User Data Segment)
    push edx          ; ESP (給平民專用的堆疊！)
    pushfd
    pop ebx
    or ebx, 0x200     ; 開啟中斷
    push ebx
    push 0x1b         ; CS (User Code Segment)
    push eax          ; EIP
    iret
```

---
elf.c
```c
// ELF: Executable and Linkable Format
#include "elf.h"
#include "tty.h"
#include "paging.h"
#include "pmm.h"
#include "utils.h"

// --- 公開 API ---

// 檢查這是不是一個我們支援的 x86 32-bit ELF 執行檔
bool elf_check_supported(elf32_ehdr_t* header) {
    if (!header) return false;

    // 1. 檢查魔法數字 (Magic Number)
    if (header->magic != ELF_MAGIC) {
        kprintf("[ELF] Error: Invalid Magic Number. Not an ELF file.\n");
        return false;
    }

    // 2. 檢查是否為 32 位元 (ident[0] == 1 代表 32-bit)
    if (header->ident[0] != 1) {
        kprintf("[ELF] Error: Not a 32-bit ELF file.\n");
        return false;
    }

    // 3. 檢查是否為可執行檔 (Type 2 = Executable)
    if (header->type != 2) {
        kprintf("[ELF] Error: Not an executable file.\n");
        return false;
    }

    // 4. 檢查是否為 x86 架構 (Machine 3 = x86)
    if (header->machine != 3) {
        kprintf("[ELF] Error: Not compiled for x86 CPU.\n");
        return false;
    }

    kprintf("[ELF] Valid x86 32-bit Executable!\n");
    kprintf("[ELF] Entry Point is at: [0x%x]\n", header->entry);

    return true;
}

uint32_t elf_load(elf32_ehdr_t* ehdr) {
    // 1. 使用我們寫好的檢查函式
    if (!elf_check_supported(ehdr)) {
        return 0;
    }

    // 2. 找到 Program Header Table 的起點 (使用簡化版的 phoff)
    elf32_phdr_t* phdr = (elf32_phdr_t*) ((uint8_t*)ehdr + ehdr->phoff);

    // 3. 掃描每一個 Program Header (使用簡化版的 phnum)
    for (int i = 0; i < ehdr->phnum; i++) {

        // 如果這個區段標記為 PT_LOAD (需要被載入到記憶體)
        if (phdr[i].type == 1) {
            uint32_t virt_addr = phdr[i].vaddr;
            uint32_t mem_size = phdr[i].memsz;
            uint32_t file_size = phdr[i].filesz;
            uint32_t file_offset = phdr[i].offset;

            // 分配並映射記憶體分頁
            uint32_t num_pages = (mem_size + 4095) / 4096;
            for (uint32_t j = 0; j < num_pages; j++) {
                // 強制轉型消除 warning
                uint32_t phys_addr = (uint32_t) pmm_alloc_page();
                map_page(virt_addr + (j * 4096), phys_addr, 7);
            }

            // 【終極修復點：複製資料】
            uint8_t* src = (uint8_t*)ehdr + file_offset;
            uint8_t* dest = (uint8_t*)virt_addr;

            // 複製檔案裡實際有的資料
            memcpy(dest, src, file_size);

            // 將未初始化的變數 (BSS 段) 清零
            if (mem_size > file_size) {
                memset(dest + file_size, 0, mem_size - file_size);
            }
        }
    }

    return ehdr->entry; // 回傳進入點
}
```
---
gdt.c
```c
// GDT: Global Descriptor Table
// TSS: Task State Segment
#include <stdint.h>
#include "gdt.h"
#include "utils.h"

gdt_entry_t gdt_entries[6]; // 6 個元素：Null, KCode, KData, UCode, UData, TSS
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);    // 外部組合語言函式，用來載入 GDT (Global Descriptor Table)
extern void tss_flush(void);        // 外部組合語言函式，用來載入 TSS (Task State Segment)

// 設定單一 GDT 條目的輔助函式
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// 初始化 TSS (Task State Segment) 的輔助函式
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry_t);

    // 把 TSS 當作一個特殊的 GDT 區段加進去 (Access: 0xE9 代表 TSS)
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry_t));
    tss_entry.ss0  = ss0;   // 設定 Kernel Data Segment (0x10)
    tss_entry.esp0 = esp0;  // 設定當前 Kernel 的 Stack 頂端
}

// --- 公開 API ---

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 第一個必須是 Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 第二個是 Code Segment (位址 0~4GB)
    // Access 0x9A: Ring 0, 可執行, 可讀取
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 第三個是 Data Segment (位址 0~4GB)
    // Access 0x92: Ring 0, 可讀寫
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // [新增] User Code Segment (位址 0~4GB)
    // Access 0xFA: Ring 3 (DPL=3), 可執行, 可讀取
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // [新增] User Data Segment (位址 0~4GB)
    // Access 0xF2: Ring 3 (DPL=3), 可讀寫
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // [新增] 初始化 TSS (掛在第 5 個位置，也就是 0x28)
    // 這裡的 0x10 是 Kernel Data Segment。0x0 暫時填 0，晚點會被動態更新
    write_tss(5, 0x10, 0x0);

    // 呼叫組合語言，正式套用新的 GDT
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush(); // 告訴 CPU：「逃生路線圖在這裡！」
}

// 在 lib/gdt.c 檔案的下方新增這個函式：
void set_kernel_stack(uint32_t esp) {
    // 0x10 代表 Kernel Data Segment (你的 GDT 設定中，Data 段的 offset)
    tss_entry.ss0 = 0x10;
    tss_entry.esp0 = esp;
}
```
---
idt.c
```c
// IDT: Interrupt Descriptor Table
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
// PIC: [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0();     // 第 0  號中斷：的 Assembly 進入點
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr128();   // 第 128 號中斷：system calls

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
void pic_remap() {
    // 儲存原本的遮罩 (Masks)
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; (void)a2; // 消除 unused variable 警告

    // 開始初始化序列 (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定 Master PIC 的偏移量為 0x20 (十進位 32)
    outb(0x21, 0x20);
    // ICW2: 設定 Slave PIC 的偏移量為 0x28 (十進位 40)
    outb(0xA1, 0x28);

    // ICW3: 告訴 Master PIC 有一個 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: 告訴 Slave PIC 它的串聯身份
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 我們目前只開啟 IRQ1 (鍵盤)，關閉其他所有硬體中斷 (避免 Timer 狂噴中斷干擾我們)
    // 0xFD 的二進位是 1111 1101 (第 1 個 bit 是 0，代表開啟鍵盤)
    // 0xFC 的二進位是 1111 1100 (第 0 和第 1 個 bit 都是 0，代表開啟 Timer 與 Keyboard)
    outb(0x21, 0xFC); // [修改] 從 0xFD 變成 0xFC
    outb(0xA1, 0xFF);
}


// --- 公開 API ---

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // [新增] 重新映射 PIC
    pic_remap();

    // [新增] 掛載第 32 號中斷 (IRQ0 Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    // [新增] 掛載第 33 號中斷 (IRQ1 鍵盤)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // [新增] 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}
```
---
kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"

// 將檔案系統的初始化與安裝過程獨立出來
void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");

    // 1. 掛載與格式化
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);

    // 2. 建立測試文字檔
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 3. 模擬系統安裝：從 GRUB 模組把 my_app.elf 寫入實體硬碟
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        uint32_t app_size = mod->mod_end - mod->mod_start;
        kprintf("[Kernel] 'Installing' Shell to HDD (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod->mod_start, app_size);
    }

    // 印出目錄結構確認
    kprintf("\n--- SimpleFS Root Directory ---\n");
    simplefs_list_files(part_lba);
    kprintf("-------------------------------\n\n");
}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    __asm__ volatile ("sti");

    kprintf("=== OS Subsystems Ready ===\n\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }

    // 呼叫我們重構好的函式！
    setup_filesystem(part_lba, mbd);

    // --- 應用程式載入與排程 ---
    kprintf("[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating ONE Initial User Task (Init Process)...\n\n");

            init_multitasking();

            // 為唯一的 Shell 分配 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("Kernel dropping to idle state. Have fun!\n");
            schedule();
        }
    } else {
        kprintf("[Kernel] Error: Shell (my_app.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
keyboard.c
```c
#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"
#include "io.h"
#include "tty.h"
#include "utils.h"
#include "task.h"

#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0;
volatile int kbd_tail = 0;

unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// --- 公開 API ---

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        char ascii = kbdus[scancode];
        if (ascii != 0) {
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }
    outb(0x20, 0x20);
}

char keyboard_getchar() {
    while (kbd_head == kbd_tail) {
        __asm__ volatile("sti");
        schedule();  // 【關鍵】讓出 CPU，讓多工作業順利流轉
        __asm__ volatile("hlt");
    }
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    __asm__ volatile("sti");
    return c;
}
```
---
kheap.c
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
mbr.c
```c
#include "mbr.h"
#include "ata.h"
#include "tty.h"
#include "utils.h"
#include "kheap.h"

// --- 公開 API ---

uint32_t parse_mbr(void) {
    uint8_t* buffer = (uint8_t*) kmalloc(512);
    ata_read_sector(0, buffer);

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        kfree(buffer);
        return 0; // 找不到 MBR
    }

    mbr_partition_t* partitions = (mbr_partition_t*)(buffer + 446);
    uint32_t target_lba = 0;

    for (int i = 0; i < 4; i++) {
        if (partitions[i].sector_count > 0) {
            kprintf("Partition %d found at LBA %d\n", i + 1, partitions[i].lba_start);
            // 記錄第一個找到的合法分區
            if (target_lba == 0) {
                target_lba = partitions[i].lba_start;
                // 注意：這裡我們偷懶省略了傳遞 sector_count，
                // 實務上也可以用 struct 把 start 和 count 一起傳回去
            }
        }
    }

    kfree(buffer);
    return target_lba;
}
```
---
paging.c
```c
#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"

// 1. 宣告兩張分頁表 (必須對齊 4KB)
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096))); // 管 0MB ~ 4MB
uint32_t second_page_table[1024] __attribute__((aligned(4096)));// 我們用這張表來管 2GB 附近的高階記憶體
uint32_t third_page_table[1024] __attribute__((aligned(4096))); // 給 3GB (0xC0000000) 用
uint32_t user_page_table[1024] __attribute__((aligned(4096)));

// 宣告外部的組合語言函式
extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

// --- 公開 API ---

void init_paging(void) {
    // 1. 初始化 Page Directory：將所有條目設為「不存在」
    // 屬性 0x02 代表：Read/Write (可讀寫), 但 Present (存在) 為 0
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // 2. 建立第一張 Page Table：涵蓋 0MB ~ 4MB 的實體記憶體
    // 這張表有 1024 項，每項 4KB，所以剛好是 4MB (包含我們的核心與 VGA)
    for(int i = 0; i < 1024; i++) {
        // 實體位址 = i * 4096 (也就是 0x1000)
        // [修改] 將權限從 3 改成 7 (Present | Read/Write | User)
        first_page_table[i] = (i * 0x1000) | 7;
    }

    // [新增] 初始化第二張表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        second_page_table[i] = 0;
    }

    // 初始化第三張表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        third_page_table[i] = 0;
    }

    // [新增] 初始化 User 表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        user_page_table[i] = 0;
    }

    // 將兩張表掛載到目錄上
    // 這樣 0x00000000 到 0x003FFFFF 的虛擬位址就會被翻譯到這裡
    // [修改] 目錄的第 0 項也必須加上 User 權限 (7)
    page_directory[0] = ((uint32_t)first_page_table) | 7;

    // 掛載到目錄的第 32 項 (管理 128MB ~ 132MB 的虛擬空間)
    page_directory[32] = ((uint32_t)user_page_table) | 7; // 7 代表允許 Ring 3 存取

    // 0x80000000 除以 4MB (0x400000) = 512，所以 2GB 的位址是由目錄的第 512 項來管！
    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3; // [新增] 3GB 目錄項

    // 4. 呼叫組合語言，把目錄位址交給 CPU，並開啟 Paging
    load_page_directory(page_directory);
    enable_paging();
}

// 動態映射函式
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // 1. 取得目錄索引 (最高 10 bits)：右移 22 位元
    uint32_t pd_idx = virt >> 22;
    // 2. 取得分頁表索引 (中間 10 bits)：右移 12 位元後，用 0x3FF (1023) 遮罩
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    // 取得該目錄項指到的分頁表位址
    uint32_t* page_table;
    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {          // [新增] 支援 128MB 區域
        page_table = user_page_table;
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table; // [新增] 支援 3GB 映射
    } else {
        // 在一個完整的 OS 中，這裡應該要 pmm_alloc_page() 一個新的實體框，
        // 然後用遞迴映射 (Recursive Paging) 的黑魔法來初始化它。我們今天先跳過這塊深水區。
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    // 3. 把實體位址 (對齊 4KB) 加上權限標籤，寫入分頁表！
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;

    // 4. [極度重要] 刷新 TLB (Translation Lookaside Buffer)
    // CPU 為了加速，會把舊的地址對應記在快取裡。我們改了字典，必須強迫 CPU 重讀！
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

```

---
pmm.c
```c
// PMM: Physical Memory Management
#include "pmm.h"
#include "utils.h"
#include "tty.h"

// 假設我們最多支援 4GB RAM (總共 1,048,576 個 Frames)
// 1048576 bits / 32 bits (一個 uint32_t) = 32768 個陣列元素
#define BITMAP_SIZE 32768
uint32_t memory_bitmap[BITMAP_SIZE];

uint32_t max_frames = 0; // 系統實際擁有的可用 Frame 數量

// --- 內部輔助函式：操作特定的 Bit ---

// 將第 frame_idx 個 bit 設為 1 (代表已使用)
static void bitmap_set(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] |= (1 << bit_offset);
}

// 將第 frame_idx 個 bit 設為 0 (代表釋放)
static void bitmap_clear(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] &= ~(1 << bit_offset);
}

// 檢查第 frame_idx 個 bit 是否為 1
static bool bitmap_test(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    return (memory_bitmap[array_idx] & (1 << bit_offset)) != 0;
}

// 尋找第一個為 0 的 bit (第一塊空地)
static int bitmap_find_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        // 如果這個整數不是 0xFFFFFFFF (代表裡面至少有一個 bit 是 0)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame_idx = i * 32 + j;
                if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量

                if (!bitmap_test(frame_idx)) {
                    return frame_idx;
                }
            }
        }
    }
    return -1; // 記憶體全滿 (Out of Memory)
}

// --- 公開 API ---

void init_pmm(uint32_t mem_size_kb) {
    // 總 KB 數 / 4 = 總共有多少個 4KB 的 Frames
    max_frames = mem_size_kb / 4;

    // 一開始先把所有的記憶體都標記為「已使用」(填滿 1)
    // 這是為了安全，避免我們不小心分配到不存在的硬體空間
    memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));

    // 然後，我們只把「真實存在」的記憶體標記為「可用」(設為 0)
    for (uint32_t i = 0; i < max_frames; i++) {
        bitmap_clear(i);
    }

    // [極度重要] 保留系統前 4MB (0 ~ 1024 個 Frames) 不被分配！
    // 因為這 4MB 已經被我們的 Kernel 程式碼、VGA 記憶體、IDT/GDT 給佔用了
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
}

void* pmm_alloc_page(void) {
    int free_frame = bitmap_find_first_free();
    if (free_frame == -1) {
        kprintf("PANIC: Out of Physical Memory!\n");
        return NULL; // OOM
    }

    // 標記為已使用
    bitmap_set(free_frame);

    // 計算出實際的物理位址 (Frame 索引 * 4096)
    uint32_t phys_addr = free_frame * PMM_FRAME_SIZE;
    return (void*)phys_addr;
}

void pmm_free_page(void* ptr) {
    uint32_t phys_addr = (uint32_t)ptr;
    uint32_t frame_idx = phys_addr / PMM_FRAME_SIZE;

    bitmap_clear(frame_idx);
}

```
---
simplefs.c
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
    kprintf("[SimpleFS] Mounted at LBA %d\n", part_lba);
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    kprintf("[SimpleFS] Formatting partition starting at LBA %d...\n", partition_start_lba);

    // 1. 準備 Superblock
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;     // 根目錄在分區的第 1 個相對 LBA (實體為 partition_start_lba + 1)
    sb->data_start_lba = 2;   // 資料區從第 2 個相對 LBA 開始

    // 寫入 Superblock 到分區的起點
    ata_write_sector(partition_start_lba, (uint8_t*)sb);
    kprintf("[SimpleFS] Superblock written.\n");

    // 2. 準備空白的根目錄 (填滿 0 即可，代表沒有任何檔案)
    uint8_t* empty_dir = (uint8_t*) kmalloc(512);
    memset(empty_dir, 0, 512);

    // 寫入根目錄到 Superblock 的下一個磁區
    ata_write_sector(partition_start_lba + sb->root_dir_lba, empty_dir);
    kprintf("[SimpleFS] Empty root directory created.\n");

    // 清理記憶體
    kfree(sb);
    kfree(empty_dir);

    kprintf("[SimpleFS] Format complete!\n");
}

// 列出檔案
void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n--- SimpleFS Root Directory ---\n");

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("[-] %s  (Size: %d bytes, LBA: %d)\n",
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
    for (int i = 0; i < 16; i++) {
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

    int empty_idx = -1;
    uint32_t next_data_lba = 2;

    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] == '\0') {
            if (empty_idx == -1) empty_idx = i;
        } else {
            // [修改] 計算這個檔案佔用了幾個磁區 (無條件進位)
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (empty_idx == -1) { kfree(dir_buf); return -1; }

    // 跨磁區寫入資料
    uint32_t sectors_needed = (size + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        uint8_t temp[512] = {0}; // 清空暫存區
        uint32_t copy_size = 512;
        if ((i * 512) + 512 > size) {
            copy_size = size - (i * 512); // 最後一個磁區可能裝不滿
        }
        memcpy(temp, data + (i * 512), copy_size);
        ata_write_sector(part_lba + next_data_lba + i, temp);
    }

    memset(entries[empty_idx].filename, 0, 24);
    strcpy(entries[empty_idx].filename, filename);
    entries[empty_idx].start_lba = next_data_lba;
    entries[empty_idx].file_size = size;

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

```
---
syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

// 【修復】拔掉 extern，讓它真正在這個檔案裡被分配記憶體空間！
fs_node_t* fd_table[32] = {0};

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
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
}
```
---
task.c
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

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
// 宣告組合語言寫好的降落傘，不再用 C 語言實作！
extern void child_ret_stub();

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        // 如果有人在等我，搖醒他！
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->id) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }
    current_task->state = TASK_DEAD;
    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;
    while (next_node != curr) {
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
        kprintf("\n[Kernel] No runnable tasks. CPU Idle.\n");
        while(1) { __asm__ volatile("sti; hlt"); }
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp);
    }
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // --- 1. 構造硬體 IRET 幀 (由高到低) ---
    *(--kstack) = 0x23;             // User SS
    *(--kstack) = user_stack_top;   // User ESP
    *(--kstack) = 0x0202;           // EFLAGS (IF=1)
    *(--kstack) = 0x1B;             // User CS
    *(--kstack) = entry_point;      // EIP

    // --- 2. 構造給 child_ret_stub 的 4 個通用暫存器 ---
    *(--kstack) = 0; // ebp
    *(--kstack) = 0; // ebx
    *(--kstack) = 0; // esi
    *(--kstack) = 0; // edi

    *(--kstack) = 0; // 假裝的返回位址
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

    // 【外科手術】修正 EBP 指標
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

    // --- 1. 構造硬體 IRET 幀 (由高到低) ---
    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    // --- 2. 構造給 child_ret_stub 的 4 個通用暫存器 ---
    *(--kstack) = child_ebp; // 【放入修正過後的 EBP】
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = 0; // 假裝的返回位址
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
    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) {
        kprintf("[Exec] Error: File '%s' not found!\n", filename);
        return -1;
    }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) return -1;

    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;

    regs->eip = entry_point;
    regs->user_esp = clean_user_stack_top;

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
---
timer.c
```c
#include <stddef.h>
#include "timer.h"
#include "io.h"
#include "utils.h"
#include "task.h"

volatile uint32_t tick = 0;

void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
}

void timer_handler(void) {
    tick++;

    // 必須先發送 EOI，才能切換任務！否則 PIC 會被卡死
    outb(0x20, 0x20);

    // 強行切換任務
    schedule();
}
```
---
tty.c

```c
// TTY: TeleTYpewriter
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include "tty.h"
#include "io.h"
#include "utils.h"

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// --- 公開 API ---

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F;
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column == VGA_COLS) {
            terminal_column = 0;
            terminal_row++;
        }
    }
    if (terminal_row == VGA_ROWS) {
        terminal_scroll();
    }

    // [新增] 每次印出字元或換行後，更新硬體游標的位置！
    update_cursor(terminal_column, terminal_row);
}

void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        terminal_putchar(data[index]);
        index++;
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

void terminal_scroll() {
    // 1. 把第 1~24 行的資料，直接「整塊」複製到第 0~23 行的位置
    // 每個字元佔 2 bytes，所以大小是 (VGA_ROWS - 1) * VGA_COLS * 2
    size_t bytes_to_copy = (VGA_ROWS - 1) * VGA_COLS * 2;
    memcpy((void*)vga_buffer, (void*)(vga_buffer + VGA_COLS), bytes_to_copy);

    // 2. 清空最後一行 (使用我們原本的迴圈，或者你未來也可以進階寫個 memclr)
    for (size_t x = 0; x < VGA_COLS; x++) {
        terminal_putentryat(' ', terminal_color, x, VGA_ROWS - 1);
    }
    terminal_row = VGA_ROWS - 1;
}

// 控制 VGA 硬體游標
void update_cursor(int x, int y) {
    // 游標位置是一維陣列的索引 (0 ~ 1999)
    uint16_t pos = y * VGA_COLS + x;

    // VGA 控制器的 Port 0x3D4 是「索引暫存器」，用來告訴 VGA 我們要設定什麼
    // VGA 控制器的 Port 0x3D5 是「資料暫存器」，用來寫入實際的值

    // 設定游標位置的低 8 bits (暫存器 0x0F)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));

    // 設定游標位置的高 8 bits (暫存器 0x0E)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
```
---
util.c
```c
#include "tty.h"

#include <stdarg.h>
#include <stdbool.h>

// === Memory Utils ===

// 記憶體複製 (將 src 複製 size 個 bytes 到 dst)
void* memcpy(void* dstptr, const void* srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}

// === StringUilts ===
// 輔助函式：反轉字串
void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

// 核心工具：整數轉字串 (itoa)
// value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base) {
    int i = 0;
    bool is_negative = false;
    unsigned int uvalue; // [新增] 用來做運算的無號整數

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 處理負數 (僅限十進位)
    if (value < 0 && base == 10) {
        is_negative = true;
        uvalue = (unsigned int)(-value);
    } else {
        // [關鍵] 如果是16進位，直接把位元當作無號整數看待 (完美對應記憶體的原始狀態)
        uvalue = (unsigned int)value;
    }

    // 逐位取出餘數 (改用 uvalue)
    while (uvalue != 0) {
        int rem = uvalue % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        uvalue = uvalue / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);
}


// === IO Utils ===

// Kernel 專屬格式化輸出 (kprintf)
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format); // 初始化不定參數列表

    for (size_t i = 0; format[i] != '\0'; i++) {
        // 如果不是 '%'，就當作一般字元直接印出
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        // 遇到 '%'，看下一個字元是什麼來決定格式
        i++;
        switch (format[i]) {
            case 'd': { // 十進位整數
                int num = va_arg(args, int);
                char buffer[32];
                itoa(num, buffer, 10);
                terminal_writestring(buffer);
                break;
            }
            case 'x': { // 十六進位整數
                // [關鍵] 改用 unsigned int 取出參數
                unsigned int num = va_arg(args, unsigned int);
                char buffer[32];
                terminal_writestring("0x");
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
            case 's': { // 字串
                char* str = va_arg(args, char*);
                terminal_writestring(str);
                break;
            }
            case 'c': { // 單一字元
                // char 在變數傳遞時會被提升為 int
                char c = (char) va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case '%': { // 印出 '%' 本身
                terminal_putchar('%');
                break;
            }
            default: // 未知的格式，原樣印出
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args); // 清理不定參數列表
}


uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}
```
---
vfs.c
```c
// VFS: Virtual File System
#include "vfs.h"
#include "tty.h"
#include "utils.h"

// 宣告全域的根節點 (目前還是空的，等具體檔案系統掛載後才會賦值)
fs_node_t *fs_root = 0;

// --- 公開 API ---

void vfs_open(fs_node_t *node) {
    if (node->open != 0) {
        node->open(node);
    }
}

void vfs_close(fs_node_t *node) {
    if (node->close != 0) {
        node->close(node);
    }
}

// VFS 通用讀取函式
uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 檢查這個節點有沒有實作讀取功能
    if (node->read != 0) {
        return node->read(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support read operation.\n", node->name);
        return 0;
    }
}

// VFS 通用寫入函式
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->write != 0) {
        return node->write(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support write operation.\n", node->name);
        return 0;
    }
}
```
