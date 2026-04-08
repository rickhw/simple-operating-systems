在編譯的時候，出現 htonl 找不到，比對之前的 code 應該是放在 ethernet.h 裡，但現在卻沒有。

ethernet.h
```c
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
// ntohs: network to host short
// 說明：它的主要功能是將一個 16 位元（2 位元組） 的整數從 網路位元組順序（Network Byte Order） 轉換為 主機位元組順序（Host Byte Order）。
// 用途：通常用於處理從網路接收到的資料（如 TCP/UDP 埠號），因為網路通訊協定規定使用「大端序」（Big-Endian），而許多電腦（如 x86 架構）內部使用「小端序」（Little-Endian）。
// 參數與回傳值：它接收一個網路順序的 16 位元無符號短整數（uint16_t），並回傳該數值在當前主機系統上的正確順序。
// 平台差異：如果主機本身就使用大端序，此函式通常不執行任何操作；若主機使用小端序，它會將位元組順序反轉。
//
// htons: host to network short
// ntohl: network to host long
// htonl: host to network long
// more see: https://linux.die.net/man/3/ntohs
#define htons ntohs

// 乙太網路標頭 (長度剛好 14 bytes)
typedef struct {
    uint8_t  dest_mac[6]; // 目標 MAC 位址
    uint8_t  src_mac[6];  // 來源 MAC 位址
    uint16_t ethertype;   // 協定類型 (如 IPv4, ARP)
} __attribute__((packed)) ethernet_header_t;

// 常見的 EtherType (Big-Endian)
#define ETHERTYPE_IPv4 0x0800
#define ETHERTYPE_ARP  0x0806

// 網路端與主機端的 Endianness 轉換
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}

#endif
```

error
```bash
==> 編譯 Kernel C: src/kernel/net/tcp.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/net/tcp.c -o kernel/net/tcp.o
kernel/net/tcp.c: In function 'tcp_send_syn':
kernel/net/tcp.c:48:20: warning: implicit declaration of function 'htonl'; did you mean 'htons'? [-Wimplicit-function-declaration]
   48 |     tcp->seq_num = htonl(0x12345678); // 初始序號 (ISN)，真實 OS 會用隨機數，這裡我們寫死方便觀察
      |                    ^~~~~
      |                    htons
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder ld -m elf_i386 -n -T ../scripts/linker.ld -o myos.bin kernel/arch/x86/boot.o kernel/arch/x86/paging_asm.o kernel/arch/x86/user_mode.o kernel/arch/x86/interrupts.o kernel/arch/x86/gdt_flush.o kernel/arch/x86/switch_task.o kernel/init/main.o kernel/net/arp.o kernel/net/udp.o kernel/net/tcp.o kernel/net/icmp.o kernel/drivers/pci/pci.o kernel/drivers/video/gfx.o kernel/drivers/video/tty.o kernel/drivers/net/rtl8139.o kernel/drivers/input/mouse.o kernel/drivers/input/keyboard.o kernel/drivers/rtc/rtc.o kernel/drivers/block/ata.o kernel/drivers/block/mbr.o kernel/arch/x86/timer.o kernel/arch/x86/paging.o kernel/arch/x86/gdt.o kernel/arch/x86/idt.o kernel/lib/utils.o kernel/mm/pmm.o kernel/mm/kheap.o kernel/fs/simplefs.o kernel/fs/elf.o kernel/fs/vfs.o kernel/kernel/gui.o kernel/kernel/syscall.o kernel/kernel/task.o
ld: kernel/net/tcp.o: in function `tcp_send_syn':
tcp.c:(.text+0xfa): undefined reference to `htonl'
make: *** [src/myos.bin] Error 1
```
