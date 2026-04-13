#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    uint32_t ms = 1000;
    if (argc > 1) {
        ms = atoi(argv[1]);
    }

    printf("Sleeping for %d ms...\n", ms);

    printf("Wake up!\n");

    uint32_t start = get_ticks();

    msleep(ms);

    uint32_t end = get_ticks();
    uint32_t diff = end - start;

    printf("Start tick: %d\n", start);
    printf("End tick: %d\n", end);
    printf("Difference: %d ticks\n", diff);

    // 如果你在 main.c 是 init_timer(100); 這裡的 diff 應該要是 100
    // 如果是 init_timer(1000); 這裡的 diff 應該要是 1000
    return 0;
}
