#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    uint32_t ms = 1000;
    if (argc > 1) {
        ms = atoi(argv[1]);
    }

    printf("Sleeping for %d ms...\n", ms);

    uint32_t start = get_ticks();
    msleep(ms);
    uint32_t end = get_ticks();
    uint32_t diff = end - start;

    printf("Wake up!\n");    // ← 移到 msleep 之後

    printf("Start tick: %d\n", start);
    printf("End tick:   %d\n", end);
    printf("Difference: %d ticks\n", diff);

    return 0;
}
