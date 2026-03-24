#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: write <filename> <content>\n");
        printf("Example: write note.txt \"Hello World!\"\n");
        return 1;
    }

    char* filename = argv[1];
    char* content = argv[2];

    printf("[WRITE] Creating file '%s' on disk...\n", filename);

    int result = create_file(filename, content);

    if (result == 0) {
        printf("[WRITE] Success! Use 'cat %s' to read it.\n", filename);
    } else {
        printf("[WRITE] Failed to create file.\n");
    }

    return 0;
}
