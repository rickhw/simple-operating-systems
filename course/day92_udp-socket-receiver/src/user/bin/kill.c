#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return -1;
    }

    int pid = atoi(argv[1]);
    if (kill(pid) == 0) {
        printf("Process %d killed.\n", pid);
    } else {
        printf("Failed to kill PID %d. Not found or permission denied.\n", pid);
    }

    return 0;
}
