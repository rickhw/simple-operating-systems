哈哈，Rick！這是一個非常經典的「眼睛業障重」時刻！😂

你仔細看一下你貼給我的 `utils.c`，裡面有 `memcpy`（複製）和 `memset`（填充），也有 `strcmp`（字串比較），但就是**沒有 `memcmp` (記憶體區塊比較)** 啦！

因為我們在 `arp.c` 裡面使用了 `memcmp(arp_table[i].ip, ip, 4) == 0` 來比對 4 個 byte 的 IP 位址是否相同，所以編譯器才會大聲抱怨找不到這個函式。

別擔心，我們只要自己手刻一個標準的 `memcmp` 補進去就可以了！請跟著我做這兩個小步驟：

### 步驟 1：在 `utils.c` 加入 `memcmp` 的實作

請打開 **`src/kernel/lib/utils.c`**，在 `memcpy` 和 `memset` 的附近，加入這段實作：

```c
// 比較兩塊記憶體前 n 個 bytes 是否相同
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i]; // 只要有不同，就回傳差值
        }
    }
    return 0; // 完全相同回傳 0
}
```

### 步驟 2：在 `utils.h` 宣告 `memcmp`

接著打開 **`src/kernel/include/utils.h`**，在 `Memory Utils` 區塊加上它的宣告：

```c
// === Memory Utils ===
void* memcpy(void* dstptr, const void* srcptr, size_t size);
void* memset(void* bufptr, int value, size_t size);
int memcmp(const void *s1, const void *s2, size_t n); // 【新增這行】
```

---

就這麼簡單！存檔之後再次執行 `make clean && make run`，那個討人厭的 `undefined reference` 就會消失了。

接下來你就能順利看到你的 OS 發出 ARP 尋人啟事，並且把 QEMU 路由器的 MAC 位址存進 ARP Table 裡了！趕快試試看吧！😎
