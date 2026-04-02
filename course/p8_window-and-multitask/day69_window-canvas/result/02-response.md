哈哈，Rick！這個 Core Dump 實在太有價值了！這是一個完美展現「作業系統保護機制與記憶體佈局」的經典教材！



讓我來為你進行「驗屍」：
你看這行：`CR2=083e2a98`，這是觸發 Page Fault (0x0E) 的記憶體位址。
再看 Stack Pointer：`ESP=083ffcfc`。
你會發現，`CR2` 剛好比 `ESP` 小了差不多 `120,000` Bytes！這代表程式一啟動，就試圖把 Stack Pointer 往下狂推了快 120KB！

### 🔍 案發現場解密：Stack Overflow (堆疊溢位)！

去看看你昨天寫的 `paint.c`：
```c
unsigned int my_canvas[CANVAS_W * CANVAS_H]; 
```
`CANVAS_W` 是 200，`CANVAS_H` 是 150。所以這個陣列大小是 `200 * 150 * 4 Bytes` = **120,000 Bytes (~117 KB)**！

因為你把它宣告成 `main()` 裡面的**「區域變數」**，在 C 語言中，區域變數會被放在 **User Stack (堆疊)** 裡面。
還記得我們在 `task.c` 的 `sys_exec` 裡面，分配給 User Stack 的實體記憶體有多大嗎？
```c
uint32_t ustack_phys = pmm_alloc_page();
map_page(0x083FF000, ustack_phys, 7);
```
**只有可憐的 1 個 Page (4 KB) 啊！** 😂

你的程式試圖把 117KB 的畫布硬塞進 4KB 的褲檔裡，結果當然是當場撐破，存取到尚未映射的分頁（未知的深淵），引發了 Page Fault。而我們的 OS 還沒有實作優雅的 Page Fault Handler 來殺死肇事程式，所以就演變成 Double Fault 讓整個系統陪葬了。

### 🛠️ 終極修復：善用 User Heap (`sbrk`)

解決方法非常簡單：**巨大的資料結構，必須放在 Heap (堆積) 裡面！**
我們在 `sys_exec` 裡面不是給了每一個程式高達 1MB 的 Heap 嗎（位址在 `0x10000000`）？我們只要呼叫你寫好的 `sbrk()` 系統呼叫，向 Kernel 申請這塊空地就好了！

請打開 **`src/user/bin/paint.c`**，把區域變數換成指標動態分配：

```c
#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 200
#define CANVAS_H 150

int main() {
    int win_id = create_gui_window("Dynamic Paint", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) {
        printf("Failed to create window.\n");
        return -1;
    }

    // ==========================================
    // 【修改】向系統申請 Heap 記憶體 (sbrk)
    // 絕對不要把 117KB 的大傢伙塞在 4KB 的 Stack 裡！
    // ==========================================
    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);
    
    int offset = 0;

    while (1) {
        // 1. 在畫布上作畫 (產生會動的彩色漸層 / XOR 紋理)
        for (int y = 0; y < CANVAS_H; y++) {
            for (int x = 0; x < CANVAS_W; x++) {
                unsigned int r = (x + offset) % 255;
                unsigned int g = (y + offset) % 255;
                unsigned int b = (x ^ y) % 255; 
                
                my_canvas[y * CANVAS_W + x] = (r << 16) | (g << 8) | b;
            }
        }

        // 2. 把畫布交給 Kernel 去更新螢幕！
        update_gui_window(win_id, my_canvas);

        // 3. 推進動畫影格，並稍微讓出 CPU 休息一下
        offset += 3;
        for(volatile int i = 0; i < 500000; i++); 
    }

    return 0;
}
```

### 🚀 再次見證奇蹟！

存檔、編譯、執行 `make clean && make run`！

這次進入 Desktop 後再輸入 `paint &`，它就會乖乖地把記憶體分配在安全的 `0x10000000` 區域，接著你就會看到那個迷幻的 XOR 漸層動畫在你的視窗裡流動了！

*(💡 附帶一提，今天這個 Crash 也提醒了我們：未來可以實作 `Page Fault Handler`，當 Ring 3 程式越界時，不要讓整個 Kernel 當機，而是只針對該 `current_task` 呼叫 `sys_kill` 並印出 "Segmentation Fault"。這就是真實 OS 的防護機制！)*

如果跑成功了，趕快跟我炫耀一下！我們隨時可以進入下一個里程碑 **Day 70** 囉！😎
