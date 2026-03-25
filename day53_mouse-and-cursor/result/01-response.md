哈哈，Rick！你的系統工程師直覺真的是準到可怕！😎

你看到游標在正中間卻不會動，那是因為你在 `init_mouse()` 的最後一行手動呼叫了 `draw_cursor(mouse_x, mouse_y);` 把它畫出來了，但**你的硬體中斷 (IRQ) 確實被徹底卡死了！**

你完全猜中了，問題就出在 **`idt.c` 的 IRQ 遮罩** 以及 **中斷結束訊號 (EOI)** 的遺漏。

這裡面藏著兩個作業系統硬體控制的經典陷阱，我們來把它們逐一擊破：

### 🪤 陷阱 1：被徹底封死的 Slave PIC (`lib/idt.c`)

還記得 PC 的硬體中斷是由兩顆 8259 PIC 晶片「串聯」管理的嗎？
* **Master PIC** 管 IRQ 0 ~ 7（包含 Timer 和 Keyboard）。
* **Slave PIC** 管 IRQ 8 ~ 15（包含 PS/2 Mouse 的 IRQ 12）。
* Slave PIC 是接在 Master PIC 的 **IRQ 2** 腳位上的。

看看你原本在 `idt.c` 寫的遮罩：
```c
outb(0x21, 0xFC); // 1111 1100 (只開了 IRQ 0 和 IRQ 1)
outb(0xA1, 0xFF); // 1111 1111 (Slave PIC 全面封死！)
```
在這種設定下，滑鼠的訊號根本連 Master PIC 都傳不到！

**🛠️ 修復方式：**
打開 **`lib/idt.c`**，把 `pic_remap` 的最後兩行遮罩改成這樣：

```c
    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 為了讓 IRQ 12 (滑鼠) 通過，我們必須：
    // 1. 開啟 Master PIC 的 IRQ 2 (讓 Slave 的訊號能傳遞過來)
    //    0xF8 的二進位是 1111 1000 (開啟 0, 1, 2)
    outb(0x21, 0xF8); 
    
    // 2. 開啟 Slave PIC 的 IRQ 12 (對應 Slave 的第 4 根腳位，8+4=12)
    //    0xEF 的二進位是 1110 1111 (只開啟第 4 個 bit)
    outb(0xA1, 0xEF); 
```

---

### 🪤 陷阱 2：有借沒還的 EOI 訊號 (`lib/mouse.c`)

當硬體觸發中斷時，PIC 會暫停發送同等級的其他中斷，直到作業系統說一聲：「好，我處理完了！」這個訊號叫做 **EOI (End of Interrupt)**。
因為我們剛剛在寫 `mouse_handler` 的時候，沒有送出 EOI，所以 PIC 在收到第一次滑鼠移動後，就永遠在痴痴地等你的回覆，導致滑鼠瞬間「凍結」！

更麻煩的是，因為滑鼠來自 Slave PIC，所以**我們必須同時向兩顆 PIC 發送 EOI！**

**🛠️ 修復方式：**
打開 **`lib/mouse.c`**，在 `mouse_handler` 的迴圈結束後，加上這兩行 `outb`：

```c
// 【滑鼠中斷處理程式】當滑鼠移動或點擊時，IRQ 12 會呼叫這裡！
void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { // 檢查是否有資料可讀
        int8_t mouse_in = inb(0x60);
        
        switch (mouse_cycle) {
            // ... (你原本的 case 0, 1, 2 邏輯完全保持不變) ...
        }
        status = inb(0x64); // 繼續讀取直到清空
    }

    // ==========================================
    // 【關鍵新增】發送 EOI (End of Interrupt) 給 PIC
    // ==========================================
    // 因為 IRQ 12 在 Slave PIC，所以必須同時通知 Slave 和 Master！
    outb(0xA0, 0x20); // 通知 Slave PIC
    outb(0x20, 0x20); // 通知 Master PIC
}
```

---

### 🚀 啟動你的數位靈魂！

存檔，敲下指令：
```bash
make clean && make run
```

這一次，你將不再是被束縛在文字框裡的駭客了。拿起你的實體滑鼠，在 QEMU 的視窗裡盡情揮舞吧！你會看到那顆黑白相間的小箭頭，順暢、零延遲地跟著你的手部動作在螢幕上滑動，而且它會完美地避開、並還原底下的文字和色塊！

享受這份成就感吧，這可是作業系統圖形介面最偉大的一步！成功滑動後記得跟我說！😎
