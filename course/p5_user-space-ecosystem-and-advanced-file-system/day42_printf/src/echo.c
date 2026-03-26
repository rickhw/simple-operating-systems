#include "stdio.h"

int main(int argc, char** argv) {
    printf("\n[ECHO Program] Start printing arguments...\n");

    if (argc <= 1) {
        printf("  (No arguments provided)\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        printf("  ");
        printf(argv[i]);
        printf("\n");
    }

    printf("[ECHO Program] Done.\n");
    return 0;
}
