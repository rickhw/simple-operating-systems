
原本 main.c，`arp_send_request` 這個觸發 update ARP table 還是需要的，

```c
    init_pci();
    uint8_t router_ip[4] = {10, 0, 2, 2};

    // 1. 先主動問路由器 MAC 是多少
    arp_send_request(router_ip);

    // 2. 等待一下，讓網卡接收 ARP Reply 並寫入 ARP Table
    for (volatile int j = 0; j < 100000000; j++) {}
```

不然又會跟昨天一樣，等很久都沒反應。

結果如附圖。

`arp_send_request(router_ip);` 這段是否適合放到 ping.c 裡面？

```c
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ping <ip_address>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);

    // 1. 先主動問路由器 MAC 是多少
    arp_send_request(router_ip);

    // 2. 等待一下，讓網卡接收 ARP Reply 並寫入 ARP Table
    for (volatile int j = 0; j < 100000000; j++) {}

    printf("PING %d.%d.%d.%d 32 bytes of data.\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    // 連發 4 顆 Ping (模擬 Windows/Linux 的行為)
    for (int i = 0; i < 4; i++) {
        sys_ping(target_ip);

        // 延遲一下，避免發送過快導致網卡信箱塞車或路由器忽略
        // (如果有 sys_sleep 可以用，沒有的話就用 busy wait)
        for (volatile int j = 0; j < 50000000; j++) {}
    }

    return 0;
}
```
