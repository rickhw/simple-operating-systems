編譯 arp.c 的時候出現 undefined ref error, 我檢查過了 utils.c 裡是有 memcmp 的 ...

---
``bash
==> 編譯 Kernel Core: src/kernel/kernel.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/kernel.c -o kernel/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder ld -m elf_i386 -n -T ../scripts/linker.ld -o myos.bin kernel/asm/boot.o kernel/asm/gdt_flush.o kernel/asm/interrupts.o kernel/asm/paging.o kernel/asm/switch_task.o kernel/asm/user_mode.o kernel/lib/arp.o kernel/lib/ata.o kernel/lib/elf.o kernel/lib/gdt.o kernel/lib/gfx.o kernel/lib/gui.o kernel/lib/idt.o kernel/lib/keyboard.o kernel/lib/kheap.o kernel/lib/mbr.o kernel/lib/mouse.o kernel/lib/paging.o kernel/lib/pci.o kernel/lib/pmm.o kernel/lib/rtc.o kernel/lib/rtl8139.o kernel/lib/simplefs.o kernel/lib/syscall.o kernel/lib/task.o kernel/lib/timer.o kernel/lib/tty.o kernel/lib/utils.o kernel/lib/vfs.o kernel/kernel.o
ld: kernel/lib/arp.o: in function `arp_update_table':
arp.c:(.text+0x118): undefined reference to `memcmp'
make: *** [src/myos.bin] Error 1
rm src/user/lib/unistd.o src/user/lib/syscall.o src/user/lib/simpleui.o src/user/lib/stdio.o src/user/asm/crt0.o src/user/lib/stdlib.o
```

---

arp.c
```c
#include "arp.h"
#include "rtl8139.h"
#include "tty.h"
#include "utils.h" // 需要用到 memcpy

// 簡單的 ARP Table 結構
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    int is_valid;
} arp_entry_t;

#define ARP_TABLE_SIZE 16
static arp_entry_t arp_table[ARP_TABLE_SIZE];

// QEMU SLIRP 預設分配給我們虛擬機的 IP
static uint8_t my_ip[4] = {10, 0, 2, 15};

void arp_send_request(uint8_t* target_ip) {
    // 整個封包大小 = Ethernet 標頭 (14) + ARP 標頭 (28) = 42 Bytes
    uint8_t packet[sizeof(ethernet_header_t) + sizeof(arp_packet_t)];

    // 將陣列指標轉型，方便我們填寫欄位
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(ethernet_header_t));

    uint8_t* my_mac = rtl8139_get_mac();

    // ==========================================
    // 1. 填寫 Ethernet Header (L2)
    // ==========================================
    for(int i=0; i<6; i++) {
        eth->dest_mac[i] = 0xFF; // 廣播給網路上所有人
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = htons(ETHERTYPE_ARP);

    // ==========================================
    // 2. 填寫 ARP Header (L2.5)
    // ==========================================
    arp->hardware_type = htons(0x0001); // Ethernet
    arp->protocol_type = htons(ETHERTYPE_IPv4);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REQUEST);

    for(int i=0; i<6; i++) {
        arp->src_mac[i] = my_mac[i];
        arp->dest_mac[i] = 0x00; // 還不知道對方的 MAC，所以填 0
    }

    for(int i=0; i<4; i++) {
        arp->src_ip[i] = my_ip[i];
        arp->dest_ip[i] = target_ip[i];
    }

    // ==========================================
    // 3. 透過網卡發送！(雖然只有 42 bytes，但網卡會自動幫我們 padding 到 60 bytes)
    // ==========================================
    rtl8139_send_packet(packet, sizeof(packet));

    kprintf("[ARP] Broadcast Request sent: Who has [%d.%d.%d.%d]?\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}


// 更新電話簿的 API
void arp_update_table(uint8_t* ip, uint8_t* mac) {
    // 尋找空位或已存在的 IP 並更新 MAC
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].is_valid || memcmp(arp_table[i].ip, ip, 4) == 0) {
            memcpy(arp_table[i].ip, ip, 4);
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].is_valid = 1;
            kprintf("[ARP] Table Updated: %d.%d.%d.%d -> %x:%x:%x:%x:%x:%x\n",
                    ip[0], ip[1], ip[2], ip[3],
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return;
        }
    }
}
```

---

utils.c
```c
#include "tty.h"
#include "gui.h"

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

int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// 尋找子字串 (如果找到 needle，回傳它在 haystack 中的指標，否則回傳 0)
char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char* h = haystack;
            const char* n = needle;
            while (*h && *n && *h == *n) {
                h++;
                n++;
            }
            if (!*n) return (char*)haystack; // 完全比對成功
        }
    }
    return 0; // 找不到
}

// 安全的字串拷貝 (最多拷貝 n 個字元，並保證 null 結尾)
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for ( ; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
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
                // terminal_writestring("0x");
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

    // 整句 kprintf 組合完畢後，再一次性渲染到螢幕上！
    gui_redraw();
}
```

---
utils.h
```c
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// === Memory Utils ===
void* memcpy(void* dstptr, const void* srcptr, size_t size);
void* memset(void* bufptr, int value, size_t size);

// === String Utils ===
void reverse_string(char* str, int length);
void reverse_string(char* str, int length);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
int strlen(const char* str);
// 尋找子字串 (如果找到 needle，回傳它在 haystack 中的指標，否則回傳 0)
char* strstr(const char* haystack, const char* needle);
// 安全的字串拷貝 (最多拷貝 n 個字元，並保證 null 結尾)
char *strncpy(char *dest, const char *src, size_t n);

// 核心工具：整數轉字串 (itoa), value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base);

// === IO Utils ===
void kprintf(const char* format, ...);

#endif
```
