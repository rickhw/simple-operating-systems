哈哈，Rick！你現在遇到的這個問題，是每個開發作業系統的人**必定會經歷的「重構陣痛期」**！

在早期的開發中，我們通常需要什麼指令就隨手寫在哪裡（所以你會看到 `pci.c` 裡有 `outl`，`rtl8139.c` 裡有 `outw`，`utils.c` 裡還有舊版的 `outw`）。當系統越來越龐大，這些散落各地的重複宣告就會開始打架，引發你看到的 `static declaration follows non-static` 錯誤，或是 `implicit declaration` (找不到宣告) 的錯誤。

你的想法非常正確！**把所有 I/O 相關的底層組語統一收攏到 `io.h`，並且使用 `static inline`**，這正是 Linux Kernel 標準的作法！

讓我們用外科手術般的精準度，分 4 步解決這個混亂：

---

### 步驟 1：打造終極版的 `io.h`

我們要一次把 8-bit (`b`), 16-bit (`w`), 32-bit (`l`) 的 `in/out` 函式全部補齊。

請打開 **`src/kernel/include/io.h`**，將內容**完全替換**為以下程式碼：

```c
#ifndef IO_H
#define IO_H

#include <stdint.h>

// ==========================================
// 8-bit I/O (Byte)
// ==========================================
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// ==========================================
// 16-bit I/O (Word)
// ==========================================
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// ==========================================
// 32-bit I/O (Long / Double Word)
// ==========================================
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#endif
```

---

### 步驟 2：清理 `utils.c` 與 `utils.h` (斬斷舊牽絆)

既然 `io.h` 已經接管了，我們要把 `utils` 裡面舊的宣告刪掉，避免編譯器精神分裂。

1. 打開 **`src/kernel/lib/utils.c`**：
   **刪除** 裡面的 `inw` 和 `outw` 函式實作。
2. 打開 **`src/kernel/include/utils.h`**：
   **刪除** 裡面的 `uint16_t inw(uint16_t port);` 和 `void outw(uint16_t port, uint16_t data);` 這兩行宣告。

---

### 步驟 3：清理 `pci.c`

我們要把 `pci.c` 裡面手寫的 32-bit I/O 刪掉，改用統一的 `io.h`。順便修復你 Log 裡提到的 `kprintf` 找不到的問題。

打開 **`src/kernel/lib/pci.c`**：
1. 在最上方的 `#include` 區塊，確保有這三行：
   ```c
   #include "pci.h"
   #include "tty.h" // 修復 kprintf 的警告
   #include "io.h"  // 【新增】引入標準 IO 庫
   #include "rtl8139.h"
   ```
2. **刪除** 這兩段程式碼：
   ```c
   // 32-bit I/O 寫入
   static inline void outl(uint16_t port, uint32_t val) { ... }
   
   // 32-bit I/O 讀取
   static inline uint32_t inl(uint16_t port) { ... }
   ```

---

### 步驟 4：清理 `rtl8139.c`

最後，把網卡驅動也切換到標準的 `io.h`。

打開 **`src/kernel/lib/rtl8139.c`**：
1. 在最上方加入 `#include "io.h"`：
   ```c
   #include "rtl8139.h"
   #include "pci.h"
   #include "tty.h"
   #include "io.h"  // 【新增】引入標準 IO 庫
   ```
2. **刪除** 這三段你自己寫的/宣告的舊程式：
   ```c
   extern uint8_t inb(uint16_t port);
   extern void outb(uint16_t port, uint8_t data);
   
   // 【Day 83 新增】16-bit I/O 寫入
   static inline void outw(uint16_t port, uint16_t val) { ... }
   ```

---

### 🚀 重構完成！重新起飛！

這 4 個步驟做完後，你的作業系統底層架構會變得異常乾淨：所有的硬體溝通全部走 `io.h`，不會再有重複定義的問題，而且 `rtl8139.c` 也能順利認識 `outl` 了！

執行 `make clean && make run`！
看看你的編譯器是不是乖乖閉嘴，順利編譯通過，並且再次看到網卡被喚醒的畫面了？😎
