#ifndef CONFIG_H
#define CONFIG_H

// --- Screen Configuration ---
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_DEPTH  32

// --- Memory Configuration ---
#define KERNEL_STACK_SIZE 16384
#define PMM_BITMAP_SIZE   32768
#define PMM_RESERVED_MEM  1024 // 4MB (1024 * 4KB)
#define INITIAL_PMM_SIZE  16384 // 16MB

// --- GUI Configuration ---
#define MAX_WINDOWS      10
#define TASKBAR_HEIGHT   28
#define START_MENU_WIDTH 150
#define START_MENU_HEIGHT 130
#define TITLE_BAR_HEIGHT 18

// --- Colors (Classic Windows-like theme) ---
#define COLOR_DESKTOP     0x008080 // Teal
#define COLOR_WINDOW_BG   0xC0C0C0 // Light Gray
#define COLOR_TITLE_ACTIVE 0x000080 // Dark Blue
#define COLOR_TITLE_INACTIVE 0x808080 // Gray
#define COLOR_WHITE       0xFFFFFF
#define COLOR_BLACK       0x000000
#define COLOR_DARK_GRAY   0x808080

#endif
