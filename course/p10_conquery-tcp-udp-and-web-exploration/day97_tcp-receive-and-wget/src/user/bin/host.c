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
    sys_udp_bind(12345); // 我們綁定本機 12345 等待回應

    // 先發第一顆換 ARP，等一下再發第二顆真實查詢
    sys_udp_send(dns_server, 53, (char*)packet, query_len); // Port 53 是 DNS 預設 Port
    for (volatile int j = 0; j < 50000000; j++) {}
    sys_udp_send(dns_server, 53, (char*)packet, query_len);

    // 3. 等待回應
    uint8_t rx_buf[256];
    int rx_len = 0;
    while (1) {
        rx_len = sys_udp_recv((char*)rx_buf);
        if (rx_len > 0) break;
        for (volatile int i = 0; i < 1000000; i++) {} // Delay
    }

    printf(">> rx_len: %d \n", rx_len);

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
