#include "stdio.h"
#include "net.h"
#include "string.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: wget <ip_address>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip); // 為了簡化，我們先手動餵剛才 DNS 查到的 IP
    uint16_t port = 80; // HTTP 預設 Port 是 80

    printf("Connecting to Web Server at %d.%d.%d.%d:80...\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    // 1. TCP 3-Way Handshake
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {}
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {}

    // 2. 組裝 HTTP GET 請求 (這就是瀏覽器底層在做的事！)
    char* http_request =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    printf("Sending HTTP GET Request...\n");
    sys_tcp_send(target_ip, port, http_request);

    // 3. 等待並接收 HTML 原始碼
    printf("Waiting for server response...\n");
    static char rx_buf[4096];
    int rx_len = 0;
    int retry = 0;

    while (retry < 10) {
        rx_len = sys_tcp_recv(rx_buf);
        if (rx_len > 0) {
            printf("\n--- HTTP RESPONSE RECEIVED ---\n\n");
            printf("%s\n", rx_buf);
            printf("\n------------------------------\n");
            break;
        }
        for (volatile int i = 0; i < 20000000; i++) {} // Delay
        retry++;
    }

    if (rx_len == 0) printf("Timeout: No data received.\n");

    // 4. 掛斷電話
    sys_tcp_close(target_ip, port);
    return 0;
}
