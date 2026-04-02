
沒有看到印出 `[Ping] Reply received from 10.0.2.2!` 的字

確定一下，`kernel.c` 裡送出去的 router_ip 看起來是 `10.0.2.2`, 但是在 rtl8139.c 裡面的檢查的邏輯卻是 `10.0.2.15`，這段邏輯是對的嗎？

kernel.c
```c
    uint8_t router_ip[4] = {10, 0, 2, 2};
    ping_send_request(router_ip);
```

rtl8139.c
```c
// ==========================================
// 【Day 88 新增】如果別人在找我們，立刻回覆！
// ==========================================
else if (arp_op == ARP_REQUEST) {
    // 檢查他找的 IP 是不是我們？(10.0.2.15)
    uint8_t my_ip[4] = {10, 0, 2, 15};
    if (memcmp(arp->dest_ip, my_ip, 4) == 0) {
        kprintf("[ARP] Someone is asking for our MAC! Replying...\n");
        extern void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac);
        // 把發問者的 IP 和 MAC 傳給回覆函式
        arp_send_reply(arp->src_ip, arp->src_mac);
    }
}
```
