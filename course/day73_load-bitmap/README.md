太神啦，Rick！🎉 看到這套系統一路成長，從黑壓壓的終端機，到現在擁有防穿透的完美 Z-Order 事件路由，視窗能順暢地在畫面上自由飛舞，這絕對是作業系統開發過程中最痛快的時刻！你現在已經完全掌握了 GUI 引擎的核心精神了。

既然我們的視窗已經有了獨立的 `canvas` 畫布，而且我們也擁有 SimpleFS 檔案系統，**Day 73**，我們要來做一件每個圖形作業系統必經的里程碑：**「讀取硬碟檔案，並在視窗中渲染圖片！」**

我們今天要打造一支名為 **`viewer.elf`** 的 BMP 秀圖軟體。

BMP (Bitmap) 是一種最純粹的無壓縮圖片格式，它的檔頭（Header）包含了圖片的寬、高與偏移量，接著就是純粹的 RGB 像素陣列。我們要讓 User Space 程式自己去硬碟 `open()` 這張圖，解析格式，然後把它畫在專屬的視窗裡！

請跟著我進行這 4 個充滿成就感的步驟：

---

### 步驟 1：開通 User Space 的 File I/O 系統呼叫

在之前的階段，Kernel 已經實作了 `sys_open` (Syscall 3) 和 `sys_read` (Syscall 4)，但我們在 User Space 的 `unistd.c` 還沒有把它們封裝起來。

**1. 打開 `src/user/include/unistd.h`，加入宣告：**
```c
int open(const char* filename);
int read(int fd, void* buffer, int size);
```

**2. 打開 `src/user/lib/unistd.c`，加入實作：**
```c
// ==========================================
// 【Day 73 新增】檔案 I/O 操作
// ==========================================
int open(const char* filename) {
    return syscall(3, (int)filename, 0, 0, 0, 0); 
}

int read(int fd, void* buffer, int size) {
    return syscall(4, fd, (int)buffer, size, 0, 0);
}
```

---

### 步驟 2：打造 BMP 秀圖軟體 (`src/user/bin/viewer.c`)

這是一支非常經典的程式，它會利用你剛剛寫的 `open` 讀取硬碟，解析 BMP 的二進位結構，並將像素轉譯成系統的 `canvas` 格式。

建立 **`src/user/bin/viewer.c`**：

```c
#include "stdio.h"
#include "unistd.h"

int main() {
    printf("Starting Image Viewer...\n");

    // 1. 開啟硬碟中的圖片檔
    int fd = open("logo.bmp");
    if (fd < 0) {
        printf("Error: logo.bmp not found on disk!\n");
        return -1;
    }

    // 2. 向 OS 申請 150KB 的 Heap 記憶體來裝檔案內容
    // (假設圖片不會超過 150KB，大約可裝 200x200 的 24-bit 圖片)
    unsigned char* file_buf = (unsigned char*)sbrk(150000);
    int bytes_read = read(fd, file_buf, 150000);
    
    if (bytes_read <= 0) {
        printf("Error: Failed to read file.\n");
        return -1;
    }

    // 3. 解析 BMP 檔頭 (BMP 檔案開頭必須是 'B', 'M')
    if (file_buf[0] != 'B' || file_buf[1] != 'M') {
        printf("Error: Not a valid BMP file.\n");
        return -1;
    }

    // 透過指標偏移，直接讀出 BMP 的關鍵資訊 (x86 允許 Unaligned Access)
    unsigned int pixel_offset = *(unsigned int*)(&file_buf[10]); // 像素資料的起始位置
    int width = *(int*)(&file_buf[18]);                          // 圖片寬度
    int height = *(int*)(&file_buf[22]);                         // 圖片高度
    short bpp = *(short*)(&file_buf[28]);                        // 色彩深度 (Bits Per Pixel)

    if (bpp != 24 && bpp != 32) {
        printf("Error: Only 24-bit or 32-bit BMP are supported (Got %d bpp).\n", bpp);
        return -1;
    }

    printf("Loaded BMP: %dx%d, %d bpp\n", width, height, bpp);

    // 4. 根據圖片尺寸，向 Window Manager 申請視窗！
    int win_id = create_gui_window("Image Viewer", width + 4, height + 24);
    if (win_id < 0) {
        printf("Failed to create window.\n");
        return -1;
    }

    // 申請畫布
    unsigned int* my_canvas = (unsigned int*)sbrk(width * height * 4);

    // BMP 規定每行像素的 byte 數必須是 4 的倍數，不足要補 0
    int row_padded = (width * (bpp / 8) + 3) & (~3);
    unsigned char* pixel_data = file_buf + pixel_offset;

    // 5. 像素轉譯 (BMP 的特性是「由下往上」存儲，且顏色順序為 B-G-R)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int bmp_y = height - 1 - y; // 垂直翻轉
            int p_offset = bmp_y * row_padded + x * (bpp / 8);
            
            unsigned char b = pixel_data[p_offset];
            unsigned char g = pixel_data[p_offset + 1];
            unsigned char r = pixel_data[p_offset + 2];
            
            // 組合合成 32-bit 像素 (0x00RRGGBB)
            my_canvas[y * width + x] = (r << 16) | (g << 8) | b;
        }
    }

    // 6. 把畫好的圖推給 GUI 引擎！
    update_gui_window(win_id, my_canvas);

    // 進入背景守護迴圈，讓視窗持續存在
    while (1) {
        yield();
    }

    return 0;
}
```

---

### 步驟 3：準備一張 BMP 圖片與更新 Kernel

我們需要讓這張圖片被打包進 OS 的硬碟裡。

**1. 準備圖片檔：**
請在網路上找一張圖，或是用小畫家/繪圖軟體畫一張，將它存成 **`logo.bmp`**。
* **格式要求**：必須是 `24-bit` 或 `32-bit` 的 BMP 檔案。
* **尺寸要求**：請把它縮小到大約 `150 x 150` 像素左右（避免超過我們申請的 150KB Buffer）。
* **放置位置**：把它放到你 OS 原始碼的 `user/bin/` 目錄下（或是你放置 elf 的同一個編譯產出資料夾）。

**2. 告知 Kernel 複製檔案：**
打開 **`src/kernel/kernel.c`**，在 `setup_filesystem` 裡的 `filenames[]` 陣列中，加入 `viewer.elf` 和 `logo.bmp`：
```c
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping.elf", "pong.elf",
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            "ps.elf", "top.elf", "kill.elf", "free.elf", "segfault.elf",
            "status.elf", "paint.elf", 
            "viewer.elf", "logo.bmp"  // 【Day 73 新增】
        };
```

**3. 更新 GRUB 配置 (`scripts/grub.cfg`)：**
打開你的 GRUB 設定檔，在最下方確保有加入這兩個模組：
```text
module /boot/viewer.elf
module /boot/logo.bmp
```
*(💡 確保 `grub.cfg` 裡的 module 順序跟 `filenames[]` 的順序完全一致！)*

---

### 🚀 驗收：見證圖片渲染的魔法

修改 Makefile 確保 `viewer.c` 會被編譯成 `viewer.elf`，然後執行：
```bash
make clean && make run
```

進入 Desktop 後，在 Terminal 輸入：
```bash
viewer &
```

👉 **一瞬間，畫面上將會彈出一個依照圖片尺寸量身打造的視窗，裡面完美渲染出你剛剛準備好的那張 `logo.bmp`！** 並且你可以自由地拖曳它！

Rick，這就是作業系統中「解碼器 (Decoder)」與「檔案系統」結合的極致表現！把外部檔案變成記憶體裡的像素，這代表你的 OS 已經具備了擴充多媒體應用的能力。

如果你順利看到圖片了，準備好在 **Day 74** 來做一個「桌面時鐘動態更新」還是「檔案總管 (GUI 點擊開啟檔案)」呢？😎
