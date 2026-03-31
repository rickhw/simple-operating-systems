#include "stdio.h"
#include "unistd.h"

int main() {
    mem_info_t mem;
    if (get_mem_info(&mem) != 0) {
        printf("Error: Could not retrieve memory info.\n");
        return -1;
    }

    // 每個 Frame 是 4096 Bytes (4KB)
    unsigned int total_kb = mem.total_frames * 4;
    unsigned int used_kb = mem.used_frames * 4;
    unsigned int free_kb = mem.free_frames * 4;

    printf("\n=== Simple OS Memory Status ===\n");
    printf("Physical Memory (PMM):\n");
    printf("  Total: %d KB (%d Frames)\n", total_kb, mem.total_frames);
    printf("  Used : %d KB (%d Frames)\n", used_kb, mem.used_frames);
    printf("  Free : %d KB (%d Frames)\n", free_kb, mem.free_frames);
    
    printf("\nVirtual Memory (Paging):\n");
    printf("  Active Universes : %d / 16\n", mem.active_universes);
    printf("===============================\n\n");

    return 0;
}
