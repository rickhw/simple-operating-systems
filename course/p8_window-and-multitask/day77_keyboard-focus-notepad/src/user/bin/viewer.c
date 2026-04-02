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
