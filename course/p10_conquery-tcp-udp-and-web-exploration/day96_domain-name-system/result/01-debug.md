
我有做一些重構，跑起來看起來停在 ARP lookup 就停了，底下是可能的 code.

---

src/user/bin/host.c

```c
#include "stdio.h"
#include "syscall.h"
#include "string.h"
#include "dns.h"

// 網址轉換函數: "google.com" -> "\x06google\x03com\x00"
void format_dns_name(char* dns_name, char* host_name) {
    int lock = 0;
    for (int i = 0; i <= strlen(host_name); i++) {
        if (host_name[i] == '.' || host_name[i] == '\0') {
            *dns_name++ = i - lock;
            for (; lock < i; lock++) {
                *dns_name++ = host_name[lock];
            }
            lock++; // 跳過點
        }
    }
    *dns_name++ = '\0'; // 結尾
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: host <domain_name>\n");
        return 1;
    }

    char* domain = argv[1];
    printf("Resolving %s via 8.8.8.8...\n", domain);

    // 1. 準備 DNS 查詢封包
    uint8_t packet[256];
    memset(packet, 0, 256);

    dns_header_t* dns = (dns_header_t*)packet;
    dns->transaction_id = htons(0x1337);
    dns->flags = htons(0x0100); // Standard Query
    dns->questions = htons(1);

    uint8_t* qname = packet + sizeof(dns_header_t);
    format_dns_name((char*)qname, domain);

    // QNAME 後面接著 QTYPE(A record = 1) 和 QCLASS(IN = 1)
    uint16_t* qinfo = (uint16_t*)(qname + strlen((char*)qname) + 1);
    qinfo[0] = htons(1); // Type A (IPv4)
    qinfo[1] = htons(1); // Class IN

    int query_len = sizeof(dns_header_t) + strlen((char*)qname) + 1 + 4;

    // 2. 綁定接收 Port 並發射 UDP 封包
    uint8_t dns_server[4] = {8, 8, 8, 8}; // Google Public DNS
    sys_udp_bind(43210); // 我們綁定本機 43210 等待回應

    // 先發第一顆換 ARP，等一下再發第二顆真實查詢
    sys_udp_send(dns_server, 53, (char*)packet); // Port 53 是 DNS 預設 Port
    for (volatile int j = 0; j < 50000000; j++) {}
    sys_udp_send(dns_server, 53, (char*)packet);

    // 3. 等待回應
    uint8_t rx_buf[256];
    int rx_len = 0;
    while (1) {
        rx_len = sys_udp_recv((char*)rx_buf);
        if (rx_len > 0) break;
        for (volatile int i = 0; i < 1000000; i++) {} // Delay
    }

    // 4. 極度暴力的 DNS 解析器 (尋找最後 4 個 bytes)
    // 真正的 OS 會一步一步解析 Answer Section，
    // 但因為我們知道 Type A 的 IP 一定在封包的最後 4 個 bytes，直接偷吃步！
    if (rx_len > 4) {
        uint8_t* ip = rx_buf + rx_len - 4;
        printf(">> %s has address %d.%d.%d.%d\n", domain, ip[0], ip[1], ip[2], ip[3]);
    } else {
        printf("Failed to resolve.\n");
    }

    return 0;
}
```

---
src/user/include/stdlib.h
```c
#ifndef _STDLIB_H
#define _STDLIB_H

/**
 * @file stdlib.h
 * @brief 標準程式庫 (記憶體管理)
 */

#include <stddef.h>

/**
 * @brief 動態分配記憶體
 * @param size 分配大小
 * @return 指向分配空間的指標，若失敗則回傳 0
 */
void* malloc(int size);

/**
 * @brief 釋放動態分配的記憶體
 * @param ptr 指向要釋放空間的指標
 */
void free(void* ptr);

void* memset(void* bufptr, int value, size_t size);

#endif
```

---
src/user/lib/stdlib.c
```c
#include "stdlib.h"
#include "unistd.h"

/**
 * @file stdlib.c
 * @brief 使用者空間堆積 (Heap) 管理實作
 * @details 採用簡單的鏈結串列 (Linked List) 管理空閒區塊 (Free List)。
 */

/** @brief 堆積區塊標頭 */
typedef struct header {
    int size;               /**< 該區塊的實際可用大小 */
    int is_free;            /**< 標記是否為空閒 */
    struct header* next;    /**< 下一個區塊指標 */
} header_t;

static header_t* head = 0; /**< 記錄第一個區塊的指標 */

void* malloc(int size) {
    if (size <= 0) return 0;

    // 1. 先在現有的鏈結串列中尋找有沒有「空閒且夠大」的區塊
    header_t* curr = head;
    while (curr != 0) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = 0; // 標記為使用中
            return (void*)(curr + 1); // 回傳標頭「之後」的實際可用空間
        }
        curr = curr->next;
    }

    // 2. 找不到現成空間，呼叫 sbrk 向核心要求新記憶體
    int total_size = sizeof(header_t) + size;
    header_t* block = (header_t*)sbrk(total_size);

    if ((int)block == -1) return 0; // 記憶體耗盡

    // 3. 設定新區塊標頭，並加入鏈結串列
    block->size = size;
    block->is_free = 0;
    block->next = head;
    head = block;

    return (void*)(block + 1);
}

void free(void* ptr) {
    if (!ptr) return;

    // 往回退一個標頭大小，找回管理結構
    header_t* block = (header_t*)ptr - 1;
    block->is_free = 1; // 標記為空閒
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}
```

---
src/user/include/dns.h
```c
#ifndef DNS_H
#define DNS_H

#include <stdint.h>

typedef struct {
    uint16_t transaction_id; // 交易 ID (我們自己亂數定一個，例如 0x1234)
    uint16_t flags;          // 旗標 (查詢是 0x0100)
    uint16_t questions;      // 幾個問題？ (通常是 1)
    uint16_t answer_rrs;     // 幾個回答？ (發問時是 0)
    uint16_t authority_rrs;  // 授權記錄數量
    uint16_t additional_rrs; // 額外記錄數量
} __attribute__((packed)) dns_header_t;

// User Space 用的簡易 Endianness 轉換 (從 ethernet.h 抄過來的)
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}
static inline uint16_t ntohs(uint16_t netshort) { return htons(netshort); }

#endif
```

---
src/kernel/net/udp.c
```c
#include "utils.h"
#include "tty.h"
#include "ethernet.h"
#include "rtl8139.h"
#include "ipv4.h"
#include "arp.h"
#include "udp.h"

// 【Day 92 新增】簡易 UDP Socket 信箱
// ==========================================
static uint16_t bound_port = 0;       // 目前被 User App 監聽的 Port
static char rx_msg_buffer[256];       // 存放接收到的文字
static int rx_msg_ready = 0;          // 標記是否有新訊息 (1: 有, 0: 無)
// 修改狀態變數
static int rx_msg_len = 0; // 改為記錄長度 (0 代表沒訊息)

static uint8_t my_ip[4] = {10, 0, 2, 15};

// 讓 User App 綁定 Port
void udp_bind_port(uint16_t port) {
    bound_port = port;
    // rx_msg_ready = 0;
    kprintf("[Kernel UDP] Port %d bound by User Space.\n", port);
}

// 讓 User App 讀取訊息 (非阻塞)
int udp_recv_data(char* buffer) {
    if (rx_msg_len > 0) {
        memcpy(buffer, rx_msg_buffer, rx_msg_len); // 用 memcpy 才不會被 0x00 截斷！
        int len = rx_msg_len;
        rx_msg_len = 0; // 讀取完畢，清空狀態
        // rx_msg_ready = 0; // 讀取完畢，清空狀態
        return 1;         // 回傳 1 代表成功讀到新訊息
    }
    return 0;             // 回傳 0 代表目前沒訊息
}

// 讓 rtl8139 驅動把封包交給這裡
// 修改網卡對接處
void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len) {
    if (bound_port != 0 && dest_port == bound_port) {
        int copy_len = (payload_len < 255) ? payload_len : 255;
        memcpy(rx_msg_buffer, payload, copy_len);
        rx_msg_len = copy_len; // 記錄真正的長度！
    }
}

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len) {
    uint8_t* target_mac = arp_lookup(dest_ip);

    if (target_mac == 0) {
        kprintf("[UDP] MAC unknown! Initiating ARP request...\n");
        arp_send_request(dest_ip);
        return; // 丟棄這顆 UDP，等 ARP 解析完 User 再重試
    }

    // 總大小 = Eth(14) + IP(20) + UDP(8) + Payload
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t) + len;

    // 宣告一個夠大的 Buffer
    uint8_t packet[1500];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    udp_header_t* udp  = (udp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    uint8_t* payload   = packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t);

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->total_length = htons(sizeof(ipv4_header_t) + sizeof(udp_header_t) + len);
    ip->ident = htons(0x5678);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IPV4_PROTO_UDP; // 17 代表 UDP
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. UDP Header
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(sizeof(udp_header_t) + len);
    udp->checksum = 0; // IPv4 允許 UDP Checksum 為 0 (不檢查)

    // 4. Payload (塞入真正的資料)
    memcpy(payload, data, len);

    // 發射！
    rtl8139_send_packet(packet, packet_size);
}
```

---
src/user/lib/net.c
```c
#include "net.h"
#include "syscall.h"

/**
 * @file net.c
 * @brief 網路工具與系統呼叫實作
 */

// 簡易的字串轉 IP 陣列函式 (例如 "10.0.2.2" -> {10, 0, 2, 2})
void parse_ip(char* str, uint8_t* ip) {
    int i = 0;
    int num = 0;
    while (*str) {
        if (*str == '.') {
            ip[i++] = num;
            num = 0;
        } else {
            num = num * 10 + (*str - '0');
        }
        str++;
    }
    ip[i] = num; // 存入最後一個數字
}

// 發送 Ping 系統呼叫 (編號 31)
void sys_ping(uint8_t* ip) {
    syscall(SYS_PING, (int)ip, 0, 0, 0, 0);
}


// syscall.c 實作：
void sys_udp_send(uint8_t* ip, uint16_t port, char* msg) {
    __asm__ volatile (
        "int $128"
        :
        : "a"(SYS_NET_UDP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)msg)
        : "memory"
    );
}
void sys_udp_bind(uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_UDP_BIND), "b"((uint32_t)port) : "memory");
}

int sys_udp_recv(char* buffer) {
    int ret;
    __asm__ volatile ("int $128" : "=a"(ret) : "a"(SYS_NET_UDP_RECV), "b"((uint32_t)buffer) : "memory");
    return ret;
}

// TCP
void sys_tcp_connect(uint8_t* ip, uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_CONNECT), "b"((uint32_t)ip), "c"((uint32_t)port) : "memory");
}

void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)msg) : "memory");
}

void sys_tcp_close(uint8_t* ip, uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_CLOSE), "b"((uint32_t)ip), "c"((uint32_t)port) : "memory");
}
```

---
src/user/include/net.h
```c
#ifndef _LIBC_NET_H
#define _LIBC_NET_H

#include <stdint.h>

/**
 * @file net.h
 * @brief 網路相關工具與系統呼叫介面
 */

/**
 * @brief 簡易的字串轉 IP 陣列函式
 * @param str IP 字串 (例如 "10.0.2.2")
 * @param ip 輸出的 4 位元組 IP 陣列
 */
void parse_ip(char* str, uint8_t* ip);

/**
 * @brief 發送 ICMP Ping 請求
 * @param ip 目標 IP 陣列
 */
void sys_ping(uint8_t* ip);

// UDP
void sys_udp_send(uint8_t* ip, uint16_t port, char* msg);
void sys_udp_bind(uint16_t port);
int sys_udp_recv(char* buffer);

// TCP
void sys_tcp_connect(uint8_t* ip, uint16_t port);
void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg);
void sys_tcp_close(uint8_t* ip, uint16_t port);

#endif
```


---
src/user/include/syscall.h
```c
#ifndef _LIBC_SYSCALL_H
#define _LIBC_SYSCALL_H

#include <stdint.h>

/**
 * @file syscall.h
 * @brief 定義系統呼叫編號與通用呼叫介面
 */

// ==========================================
// 系統呼叫編號 (與核心定義同步)
// ==========================================
#define SYS_PRINT_STR_LEN    0
#define SYS_PRINT_STR        2
#define SYS_OPEN             3
#define SYS_READ             4
#define SYS_GETCHAR          5
#define SYS_YIELD            6
#define SYS_EXIT             7
#define SYS_FORK             8
#define SYS_EXEC             9
#define SYS_WAIT             10
#define SYS_IPC_SEND         11
#define SYS_IPC_RECV         12
#define SYS_SBRK             13
#define SYS_CREATE           14
#define SYS_READDIR          15
#define SYS_REMOVE           16
#define SYS_MKDIR            17
#define SYS_CHDIR            18
#define SYS_GETCWD           19
#define SYS_GETPID           20
#define SYS_GETPROCS         21
#define SYS_CLEAR_SCREEN     22
#define SYS_KILL             24
#define SYS_GET_MEM_INFO     25
#define SYS_CREATE_WINDOW    26
#define SYS_UPDATE_WINDOW    27
#define SYS_GET_TIME         28
#define SYS_GET_WIN_EVENT    29
#define SYS_GET_WIN_KEY      30
#define SYS_PING             31
#define SYS_NET_UDP_SEND     32
#define SYS_NET_UDP_BIND     33
#define SYS_NET_UDP_RECV     34
#define SYS_NET_TCP_CONNECT  35
#define SYS_NET_TCP_SEND     36
#define SYS_NET_TCP_CLOSE    37
#define SYS_SET_DISPLAY_MODE 99

/**
 * @brief 通用系統呼叫封裝
 * @param num 系統呼叫編號
 * @param p1 參數 1 (ebx)
 * @param p2 參數 2 (ecx)
 * @param p3 參數 3 (edx)
 * @param p4 參數 4 (esi)
 * @param p5 參數 5 (edi)
 * @return 核心回傳值 (eax)
 */
int syscall(int num, int p1, int p2, int p3, int p4, int p5);

#endif
```

---
src/user/lib/syscall.c
```c
#include "syscall.h"

/**
 * @file syscall.c
 * @brief 系統呼叫底層實作 (透過 int $0x80 中斷與核心通訊)
 */

/**
 * @brief 通用系統呼叫封裝
 * @details 支援最多 5 個參數，分別透過 ebx, ecx, edx, esi, edi 暫存器傳遞。
 *          中斷號碼目前統一使用 0x80 (128)。
 */
int syscall(int num, int p1, int p2, int p3, int p4, int p5) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (num), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5)
        : "memory"
    );
    return ret;
}
```
