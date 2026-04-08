哈哈，Rick！你抓到了一個超級核心的作業系統架構問題：**「介面 (Interface) 有開，但底層驅動 (Driver) 沒實作！」** 😂

你看 Terminal 印出的這行：
`[VFS] Error: Node 'index.html' does not support write operation.`

這完美呼應了你在 `vfs.c` 寫的防呆邏輯：
```c
if (node->write != 0) {
    return node->write(...);
} else {
    kprintf("[VFS] Error: Node '%s' does not support write operation.\n", node->name);
}
```

**🔪 為什麼會這樣？**
VFS (Virtual File System) 是一個「抽象層」。我們在 VFS 定義了 `node->write` 這個函式指標（這非常符合 POSIX 標準）。但是，當你用 `SYS_OPEN` 去硬碟裡找檔案時，底層的 **SimpleFS** 驅動程式建立這個節點時，**並沒有把它的 `write` 函式掛上去！** 要在一個極簡的檔案系統 (SimpleFS) 中實作任意 offset 的 `write`，其實非常複雜（牽涉到尋找空閒磁區、串接 LBA、更新目錄大小等等），這也就是為什麼你的 SimpleFS 當初沒有綁定 `node->write`。

### 🛠️ 峰迴路轉的秒殺修復法 (Bypass & Override)

等等，我們回頭看一下你 `vfs.h` 裡面宣告的這支 API：
```c
int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
```
這代表當初我們在設計 SimpleFS 時，為了方便，**早就實作了「建立檔案並一次性塞入內容」的外掛功能！**

我們根本不需要遵循標準的 `open` -> `write` -> `close` 流程！我們可以直接用 14 號系統呼叫 (`SYS_CREATE`)，一拳把整個 HTML 網頁砸進硬碟裡！

請打開你的 **`src/user/bin/wget.c`**，把存檔的那段邏輯修改成這樣：

```c
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
            }
```

把原本落落長的 `SYS_OPEN` 和 `SYS_WRITE` 刪掉，換成這優雅的一擊。

---

### 🚀 歷史性的一刻：Cat 出你的網際網路！

存檔，執行 `make clean && make run`！

再次執行你的神聖三連擊：
1. **`wget 104.18.27.120`** (抓取 example.com)
2. **`ls`** (確認 `index.html` 躺在你的硬碟裡了)
3. **`cat index.html`**

當你看到那滿滿的 HTML 標籤從 `cat` 指令噴發出來時，這代表你的作業系統：
✅ 具備了硬體中斷與驅動 (RTL8139)
✅ 具備了完整的網路堆疊 (Ethernet, ARP, IP, ICMP, UDP, TCP)
✅ 具備了 DNS 與 HTTP 解析能力
✅ 並且能完美地與實體儲存系統 (SimpleFS, VFS) 進行全域整合！

Rick，這就是作業系統的「大一統」境界，Phase 10 的最終成就達成！恭喜你親手打造了一個擁有現代網路靈魂的 OS！🎉😎 準備好截下那張 `cat index.html` 的畢業照了嗎？
