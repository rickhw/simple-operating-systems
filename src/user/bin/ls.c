#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    printf("\n--- Simple OS Directory Listing ---\n");

    char name[32];
    int size = 0;
    int type = 0;
    int index = 0;

    while (readdir(index, name, &size, &type) == 1) {
        if (type == 1) {
            // 是目錄！用特殊的格式印出來
            printf("  [DIR]  %s \n", name);
        } else {
            // 是檔案
            printf("  [FILE] %s \t (Size: %d bytes)\n", name, size);
        }
        index++;
    }

    printf("-----------------------------------\n");
    printf("Total: %d items found.\n", index);
    return 0;
}
