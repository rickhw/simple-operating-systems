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
            printf("\n--- TCP Data Received (%d bytes) ---\n", rx_len);

            // 【HTTP 解析器】尋找 \r\n\r\n
            char* body = 0;
            for (int i = 0; i < rx_len - 3; i++) {
                if (rx_buf[i] == '\r' && rx_buf[i+1] == '\n' &&
                    rx_buf[i+2] == '\r' && rx_buf[i+3] == '\n') {
                    body = &rx_buf[i+4]; // Payload 的起點！
                    break;
                }
            }

            if (body != 0) {
                int body_len = rx_len - (body - rx_buf);
                printf("HTTP Headers stripped. Payload size: %d bytes.\n", body_len);

                // 【檔案系統整合】寫入虛擬硬碟！
                printf("Saving to 'index.html'...\n");

                // 【修復】為字串補上結尾，因為 vfs_create_file 底層通常依賴 strlen 計算長度
                body[body_len] = '\0';

                // 【終極偷吃步】直接呼叫 SYS_CREATE (14)，將檔名與內容一次送進 SimpleFS！
                int res = syscall(14, (int)"index.html", (int)body, 0, 0, 0);

                if (res == 0) {
                    printf("Success! Webpage saved to disk.\n");
                } else {
                    printf("Error: Failed to create or write index.html.\n");
                }
            } else {
                printf("Error: Could not find HTTP body in response.\n");
            }
            break;
        }
        for (volatile int i = 0; i < 20000000; i++) {} // Delay
        retry++;
    }

    // 4. 掛斷電話 (保持不變)

    if (rx_len == 0) printf("Timeout: No data received.\n");

    // 4. 掛斷電話
    sys_tcp_close(target_ip, port);
    return 0;
}
