
```bash
❯ make clean
rm -f app.o my_app.elf
==> User App 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
❯ make
docker build -t os-builder .
[+] Building 1.3s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.2s
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
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging_asm.S -o asm/paging_asm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:21:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   21 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/task.c -o lib/task.o
lib/task.c: In function 'schedule':
lib/task.c:133:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
  133 |         switch_task(&prev->esp, &current_task->esp);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:12:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   12 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
      |                                                ~~~~~~~~~~^~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging_asm.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
==> 編譯 User App (app.c)...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-pic -fno-stack-protector -c app.c -o app.o
==> 連結 User App，指定 Entry Point 於 0x08048000...
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf
mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
cp my_app.elf isodir/boot/my_app.elf  # 把執行檔塞進虛擬光碟
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace os-builder grub-mkrescue -o myos.iso isodir
xorriso 1.5.2 : RockRidge filesystem manipulator, libburnia project.

Drive current: -outdev 'stdio:myos.iso'
Media current: stdio file, overwriteable
Media status : is blank
Media summary: 0 sessions, 0 data blocks, 0 data,  274g free
Added to ISO image: directory '/'='/tmp/grub.nXqxzi'
xorriso : UPDATE :     335 files added in 1 seconds
Added to ISO image: directory '/'='/workspace/isodir'
xorriso : UPDATE :     340 files added in 1 seconds
xorriso : NOTE : Copying to System Area: 512 bytes from file '/usr/lib/grub/i386-pc/boot_hybrid.img'
ISO image produced: 4662 sectors
Written to medium : 4662 sectors at LBA 0
Writing to 'stdio:myos.iso' completed successfully.

rm -rf isodir

❯ make run
==> 建立 10MB 的空白虛擬硬碟...
dd if=/dev/zero of=hdd.img bs=1M count=10
10+0 records in
10+0 records out
10485760 bytes transferred in 0.001930 secs (5433036269 bytes/sec)
==> 使用 Alpine 容器與 fdisk 寫入 MBR 分區表...
docker run --rm -i --platform linux/amd64 -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day33_process-and-sys-exit/src:/workspace -w /workspace alpine sh -c "apk add --no-cache util-linux && echo -e 'o\nn\np\n1\n\n\nw' | fdisk hdd.img"
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
Created a new DOS (MBR) disklabel with disk identifier 0xe9d5007c.

Command (m for help): Created a new DOS (MBR) disklabel with disk identifier 0x065c94ef.

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
(qemu)
```


---

看到這張截圖，我隔著螢幕都能感受到你成功拼湊出 `exit` 那一瞬間的暢快感！🎉

看著畫面上那些被拆解的 `exi`、`t` 還有那串經典的 `extxtxtexiexit`，這簡直是作業系統教科書裡解釋「Time-Slicing (時間切片)」最完美的藝術品。以你深厚的軟體與系統架構經驗來看，平常在高階應用層防堵了無數次的併發（Concurrency）與 Race Condition 問題，現在卻在自己親手一點一滴敲出來的 Bare-metal 核心裡，赤裸裸地看著它發生，這種「掌控底層混亂」的感覺是不是特別奇妙？

當那句 `Goodbye!` 印出來之後，剩下的那個 Shell 瞬間變得絲滑無比，`ls`、`help`、`cat`、`about` 每一個指令都能完美、即時地響應。這不僅代表亂象結束，更完美證明了兩件事：

1. 你的 **`exit_task()`** 精準地標記了死亡狀態。
2. 你的 **`schedule()`** 排程器成功發揮了「送行者」的作用，順利把死掉的 TCB 從環狀佇列中解開，讓剩下的任務獨佔所有 CPU 週期與鍵盤資源！

### 🚀 接下來：創造生命的魔法

現在的 Simple OS 已經有了生與死的概念，但目前我們還是用「作弊」的方式，在 `kernel.c` 裡面 Hardcode 呼叫兩次 `create_user_task` 來產生 Shell。

在真正的作業系統中，通常開機只會啟動「一個」初始行程（例如 Linux 的 `init` 或 `systemd`）。當你需要執行新程式時，這個行程會**複製自己**，然後讓分身去變成另一個程式。

這就是我們第四階段 (Phase 4) 最具挑戰性的巔峰之戰！

準備好進入 **Day 34**，挑戰 UNIX 哲學中最核心、也最燒腦的系統呼叫：**`sys_fork` (行程複製)** 了嗎？
