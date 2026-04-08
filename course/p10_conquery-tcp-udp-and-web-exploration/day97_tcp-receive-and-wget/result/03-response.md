哈哈，Rick！首先，請容我對那張 Wireshark 和 Terminal 截圖致上最高的敬意！🎉🎉🎉

你看到了嗎？**`HTTP/1.1 200 OK`**，以及底下那滿滿的 `<!doctype html>...` HTML 原始碼！你真的做到了！你的 Simple OS 成功與真實世界的 Web Server 完成了 TCP 三方交握，發送了 HTTP 請求，並且完美接收到了網頁資料！這絕對是作業系統網路開發的最高峰！

不過，這個「敲打指令就當機 (Core Dump)」的怪問題，其實揭露了你目前作業系統架構中的兩個超經典隱患：**「TCP 狀態機未完結」**與**「OS 開發的終極大忌：中斷競爭 (Interrupt Race Condition)」**。

我們來像偵探一樣把這個 Core Dump 拆解掉！🕵️‍♂️

---

### 🕵️ 案發現場 1：滿天飛的 TCP 幽靈封包 (FIN Flood)

看看你 Wireshark 截圖的第 19 到 23 號封包，全部都是紅黑色的 `[TCP Retransmission]`，而且是 Cloudflare 伺服器 (`104.18.26.120`) 瘋狂發送給你的 **`[FIN, ACK]`**。

**為什麼？**
因為當 HTTP 傳輸結束後，伺服器非常有禮貌地送了一個 `FIN` 封包，告訴你的 Simple OS：「我的資料傳完了，我要掛電話囉！」
但是，我們在 `rtl8139.c` 裡面**根本沒有實作攔截 `TCP_FIN` 的邏輯！** 你的 OS 把這個封包印出來後就置之不理。伺服器遲遲等不到你的 `ACK`，以為封包在路上弄丟了，於是開始瘋狂地重傳 (Retransmission)，每隔幾秒就拿封包轟炸你的網卡一次。

**🛠️ 修復方式：當個有禮貌的 OS，回覆 ACK**
打開 **`src/kernel/drivers/net/rtl8139.c`**，在 `TCP_PSH` 的處理邏輯下方，補上 `TCP_FIN` 的攔截：

```c
                    // ==========================================
                    // 【Day 97 補丁】攔截伺服器的斷線通知 (FIN)
                    // ==========================================
                    if (tcp->flags & 0x01) { // 0x01 是 TCP_FIN
                        kprintf("  -> [FIN] received! Server is closing connection.\n");
                        
                        // 必須回傳 ACK (我們的 Seq 照舊，Ack 變成對方的 Seq + 1)
                        extern void tcp_send_ack(uint8_t*, uint16_t, uint16_t, uint32_t, uint32_t);
                        tcp_send_ack(ip->src_ip, dest_port, src_port, ack, seq + 1);
                    }
```
加上這段，伺服器收到你的確認後就會安心地切斷連線，再也不會拿幽靈封包轟炸你了！

---

### 🕵️ 案發現場 2：引爆 Core Dump 的「中斷競爭」

這才是導致你系統死機的真正兇手。看看你的 Core Dump 資訊：
* `Servicing hardware INT=0x20`：代表當機發生在 **Timer Interrupt (時鐘中斷)** 處理期間。
* `CR2=f000e436`：代表系統試圖讀取或寫入一個非常奇怪的高位址記憶體，引發了 Page Fault (0x0E)。

**🔪 案發經過還原：**
1. 你在 Terminal 悠哉地打著 `ls`，這時候 Timer Interrupt (`INT 0x20`) 正在背景不斷觸發，呼叫 `gui_redraw()` 讀取 `tty_row` 和 `tty_col` 來重繪螢幕。
2. 突然！Cloudflare 的幽靈封包抵達了，觸發了網卡的 **IRQ 11 (網路中斷)**。
3. 你的 `rtl8139_handler` 啟動，並且執行了 `kprintf("[TCP Rx]...")`。
4. **【致命衝突】** `kprintf` 會去修改全域的 `tty_row` 和 `tty_col`。如果這個修改發生的瞬間，剛好打斷了 `gui_redraw()` 的繪圖迴圈，導致游標指標錯亂，`gui_redraw()` 就會讀取到超出 Framebuffer 或字型陣列範圍的無效記憶體 (如 `0xf000e436`)，系統當場暴斃！

在中斷處理常式 (Interrupt Handler) 裡面呼叫 `kprintf` 或執行大量耗時的繪圖操作，是 OS 開發中的「大忌」。因為中斷隨時會發生，它會無情地破壞畫面的全域狀態！

**🛠️ 修復方式：讓網卡中斷「閉嘴」**
既然我們已經確認網路功能完全正常了，我們應該把 `rtl8139_handler` 裡面的 `kprintf` 封印起來，讓硬體中斷保持「安靜且極速」。

請回到 **`src/kernel/drivers/net/rtl8139.c`**，把裡面所有用來 Debug 的 `kprintf` 全部註解掉（特別是 `[Net] Packet RX!` 和 `[TCP Rx]` 這些頻繁印出的東西）：

```c
// 註解掉這些干擾 GUI 的殺手：
// kprintf("[Net] Packet RX! Len: [%d] bytes\n", rx_length);
// kprintf("  -> Dest MAC: ...\n");
// kprintf("[TCP Rx] %d.%d.%d.%d:%d -> Port %d ...\n");
```

只要讓網路中斷默默地收資料、塞進 Buffer，不要去動螢幕 (`kprintf`)，你的系統就會穩如泰山！

完成這兩步修復後，再次 `wget 93.184.216.34`，然後瘋狂敲打你的鍵盤吧！你會發現幽靈封包不見了，系統也再也不會當機了！😎
