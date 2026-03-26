#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    (void)argc; // 暫時用不到參數
    (void)argv;

    printf("\n--- Simple OS Directory Listing ---\n");

    char name[32];
    int size = 0;
    int index = 0;

    // 不斷向 OS 要下一個檔案，直到 OS 說沒有為止
    while (readdir(index, name, &size) == 1) {
        printf("  [FILE] %s \t (Size: [%d] bytes)\n", name, size);
        index++;
    }

    printf("-----------------------------------\n");
    printf("Total: %d files found.\n", index);

    return 0;
}
