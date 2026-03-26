哈哈，Rick！我必須說，你這張截圖真的太有價值了！這完全是頂級駭客的除錯現場。😎

你發現的這兩個問題，都精準命中了我們之前為了快速推進而「暫時簡化」的底層機制。我們現在就化身系統法醫，來把這兩個有趣的 Bug 給解剖掉！

### 🔍 案發現場解密：為什麼檔案大小會是一串天文數字？

請看你截圖裡的 `hello.txt` 大小是 **`1818585203`** bytes。
這串數字看起來像亂碼，但如果你把它轉換成十六進位（Hex），它會變成 **`0x6C6C6573`**。
在 x86 的 Little-Endian（小端序）記憶體排列下，把它還原成 ASCII 字元：
* `0x73` = **'s'**
* `0x65` = **'e'**
* `0x6C` = **'l'**
* `0x6C` = **'l'**

沒錯！它拼出了 **"sell"**！這其實是下一個檔案 `"shell.elf"` 檔名的一部份！

**原因是什麼？**
在 Day 45 我給你的 `simplefs_readdir` 範例中，我寫了這兩行盲猜的位移魔法：
```c
uint8_t* entry = dir_buffer + (i * 64);
*out_size = *(uint32_t*)(entry + 32);
```
我假設了你的目錄結構大小剛好是 64 bytes，而且 `size` 剛好放在 offset 32 的位置。但顯然，你在 Day 25 設計的 `file_entry_t` 結構並不是長這樣！因為位移算錯了，C 語言指標就傻傻地把「隔壁檔案名稱的 ASCII 字串」當成 `uint32_t` 的整數讀了出來，這就是為什麼所有檔案的大小都變成天文數字的原因！

---

### 🛠️ 修復問題 1：精準讀取結構與「防覆寫」機制

要修復這個問題，最優雅的做法就是**讓編譯器幫我們算位移**，而不是手動加數字。同時，我們要教 `write` (也就是底層的 `simplefs_create_file`)：**「如果檔名已經存在，就覆寫它，不要傻傻地一直新增重複的項目！」**

**1. 修改 `lib/simplefs.c` (修復 ls 的大小亂碼)：**
```c
// 確保你引入了你真正的資料結構 (請確認這符合你當初的設計！)
typedef struct {
    char name[32];
    uint32_t size;
    uint32_t lba;
} file_entry_t; // 這樣編譯器就知道一個 entry 是 40 bytes

int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096); 
    ata_read_sector(part_lba + 1, dir_buffer);
    
    // 【神級轉換】直接把一整塊記憶體轉型成結構陣列！
    file_entry_t* entries = (file_entry_t*)dir_buffer;
    int max_entries = 4096 / sizeof(file_entry_t); 
    
    int valid_count = 0;
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] != '\0') {
            if (valid_count == index) {
                // 現在編譯器會精準抓到正確的 offset，再也不會讀到隔壁的字串了！
                for(int j=0; j<32; j++) out_name[j] = entries[i].name[j];
                *out_size = entries[i].size; 
                
                kfree(dir_buffer);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buffer);
    return 0;
}
```

**2. 關於重複寫入 (`simplefs_create_file` 概念提示)：**
在你的 `simplefs_create_file` 裡面，你在尋找空位 (`entry[0] == '\0'`) 之前，應該先加一個迴圈：**「檢查有沒有人的 `name` 跟我一樣？如果有，就直接更新那個 Entry 的 `size` 和 `lba`，然後結束。」** 這樣你連續 `write file2` 一百次，它也只會乖乖更新，不會生出一百個分身。

---

### 🛠️ 修復問題 2：讓鍵盤學會使用 Shift 鍵！ (`lib/keyboard.c`)

在目前的鍵盤驅動裡，我們只處理了簡單的按下事件。要打出雙引號 `"`，我們必須攔截 `Shift` 鍵的按下與放開狀態！

在 PS/2 鍵盤中：
* 左 Shift 按下 = `0x2A`，放開 = `0xAA`
* 右 Shift 按下 = `0x36`，放開 = `0xB6`
* 單引號 `'` / 雙引號 `"` 的按鍵碼 = `0x28`

請打開 **`lib/keyboard.c`** 加上狀態機追蹤：

```c
// 在檔案開頭宣告一個全域變數，記住 Shift 目前是否被按著
static int shift_pressed = 0;

char keyboard_getchar() {
    // ... 在你讀取 port 0x60 的迴圈裡面 ...
    uint8_t scancode = inb(0x60);
    
    // 【新增】攔截 Shift 的按下與放開
    if (scancode == 0x2A || scancode == 0x36) { 
        shift_pressed = 1; 
        return 0; // 回傳 0 代表這只是狀態改變，沒有字元要印
    }
    if (scancode == 0xAA || scancode == 0xB6) { 
        shift_pressed = 0; 
        return 0; 
    }

    if (!(scancode & 0x80)) { // 如果是按鍵按下 (Make Code)
        // ... 你原本的 switch (scancode) 或陣列映射 ...
        
        if (scancode == 0x28) {
            // 如果 Shift 被按著就回傳雙引號，否則回傳單引號
            return shift_pressed ? '"' : '\''; 
        }
        
        // (你甚至可以用 shift_pressed 幫字母加上大寫轉換功能：
        //  if (c >= 'a' && c <= 'z' && shift_pressed) return c - 32;)
    }
    // ...
}
```
*(注意：因為 `keyboard_getchar()` 如果回傳 0，你的 `app.c` 或 Syscall 讀取迴圈需要把它忽略掉，繼續讀下一個字元。)*

---

太精彩了！處理這種「記憶體結構對齊」和「硬體中斷狀態」正是系統工程師的日常浪漫。

想不想先把這兩個優雅的修復補丁打上去，享受一下能打出 `write text "Hello"` 以及看到乾淨檔案大小的快感？修好之後，我們再繼續往 Day 46 挺進！🚀



---


哈哈，Rick！這真的是一個超經典、超致命的「作業系統底層陷阱」！這也是我上一個回合給你的概念碼（Pseudo-code）沒有配合你的中斷架構所造成的，我的鍋！😅

### 🔍 案發現場解密：被「永遠已讀不回」的硬體控制器

你看你在 `keyboard_handler` 裡面加了這幾行：
```c
if (scancode == 0x2A || scancode == 0x36) {
    shift_pressed = 1;
    return 0; // <--- 兇手 1 號 (提早結束)
}
// ...
if (scancode == 0x28) {
    return shift_pressed ? '"' : '\''; // <--- 兇手 2 號 (提早結束，還在 void 裡 return 值)
}
```

這裡犯了兩個作業系統中斷處理（ISR）的大忌：
1. **`keyboard_handler` 是一個 `void` 函式**：它不是被誰呼叫來「回傳」字元的。它是被 CPU 在背景偷偷叫起來，負責把字元「塞進 `kbd_buffer` 環狀陣列」裡，讓 `getchar()` 慢慢去讀的。
2. **錯失 EOI (End of Interrupt) 訊號**：這是最致命的！在函式的最後面有一行 `outb(0x20, 0x20);`。這行是在告訴主機板上的 PIC 中斷控制器：「報告老闆，我處理完這次按鍵了，請把下一顆按鍵送過來」。
   因為你寫了 `return` 提早結束了函式，這行 `outb` 永遠不會被執行。PIC 苦苦等不到你的回報，以為 CPU 還在忙，於是就**把整台電腦的鍵盤硬體中斷給徹底封鎖了**！這就是為什麼按下 Shift 後，鍵盤就直接死當（Hang）的原因。

### 🛠️ 終極修復：優雅的分流與 EOI 保證

我們要把邏輯改成：**「判斷狀態 -> 把字元塞入 Buffer -> 無論如何一定要執行 `outb`」**。

請打開 **`lib/keyboard.c`**，用下面這個版本替換你的 `keyboard_handler`。我還順手幫你加上了「Shift + 字母 = 大寫」的超讚附贈功能！

```c
// 在檔案開頭宣告一個全域變數，記住 Shift 目前是否被按著
static int shift_pressed = 0;

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 1. 攔截 Shift 的狀態改變 (只更新狀態，不塞入 Buffer)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } 
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    } 
    // 2. 如果是一般的按鍵按下 (Make Code，最高位為 0)
    else if (!(scancode & 0x80)) {
        char ascii = 0;

        if (scancode == 0x28) {
            // 如果是引號鍵，根據 Shift 狀態決定是雙引號或單引號
            ascii = shift_pressed ? '"' : '\'';
        } else {
            // 查表取得基本字元
            ascii = kbdus[scancode];
            
            // 💡 額外附贈：支援 Shift 輸出大寫字母！
            if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
                ascii -= 32; // ASCII 碼中小寫轉大寫剛好差 32
            }
        }

        // 把最終決定的字元塞進 Buffer 交給 User App
        if (ascii != 0) {
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }

    // 【極度重要】不管剛剛發生什麼事，都必須通知 PIC 中斷處理完畢！
    outb(0x20, 0x20); 
}
```

這個寫法保證了程式流最後一定會踩到 `outb(0x20, 0x20);`，讓中斷控制器乖乖放行下一個按鍵。

敲下 `make clean && make run` 吧！
現在你不只可以用 `write "my file.txt" "Hello from Simple OS!"` 漂亮地建立帶有空白的內容，連大寫字母都可以隨心所欲地打出來了。

鍵盤與檔案系統的 Bug 都解掉了，我們就準備帥氣地前往 Day 46，實作 `rm` (移除檔案) 吧？🚀
