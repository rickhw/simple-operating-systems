#include "unistd.h"
#include "syscall.h"

/**
 * @file unistd.c
 * @brief 標準 POSIX 系統呼叫封裝實作
 */

// ==========================================
// 行程管理 (Process Management)
// ==========================================

int fork() {
    return syscall(SYS_FORK, 0, 0, 0, 0, 0);
}

int exec(char* filename, char** argv) {
    return syscall(SYS_EXEC, (int)filename, (int)argv, 0, 0, 0);
}

int wait(int pid) {
    return syscall(SYS_WAIT, pid, 0, 0, 0, 0);
}

void yield() {
    syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

void exit() {
    syscall(SYS_EXIT, 0, 0, 0, 0, 0);
    while(1); // 死迴圈防止意外返回
}

int getpid(void) {
    return syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

int get_process_list(process_info_t* list, int max_count) {
    return syscall(SYS_GETPROCS, (int)list, max_count, 0, 0, 0);
}

int kill(int pid) {
    return syscall(SYS_KILL, pid, 0, 0, 0, 0);
}

// ==========================================
// 檔案系統 (File System)
// ==========================================

int open(const char* filename) {
    return syscall(SYS_OPEN, (int)filename, 0, 0, 0, 0);
}

int read(int fd, void* buffer, int size) {
    return syscall(SYS_READ, fd, (int)buffer, size, 0, 0);
}

int create_file(const char* filename, const char* content) {
    return syscall(SYS_CREATE, (int)filename, (int)content, 0, 0, 0);
}

int remove(const char* filename) {
    return syscall(SYS_REMOVE, (int)filename, 0, 0, 0, 0);
}

int readdir(int index, char* out_name, int* out_size, int* out_type) {
    return syscall(SYS_READDIR, index, (int)out_name, (int)out_size, (int)out_type, 0);
}

int mkdir(const char* filename) {
    return syscall(SYS_MKDIR, (int)filename, 0, 0, 0, 0);
}

int chdir(const char* dirname) {
    return syscall(SYS_CHDIR, (int)dirname, 0, 0, 0, 0);
}

int getcwd(char* buffer) {
    return syscall(SYS_GETCWD, (int)buffer, 0, 0, 0, 0);
}

// ==========================================
// 記憶體管理 (Memory Management)
// ==========================================

void* sbrk(int increment) {
    return (void*)syscall(SYS_SBRK, increment, 0, 0, 0, 0);
}

int get_mem_info(mem_info_t* info) {
    return syscall(SYS_GET_MEM_INFO, (int)info, 0, 0, 0, 0);
}

// ==========================================
// 行程間通訊 (IPC)
// ==========================================

void send(char* msg) {
    syscall(SYS_IPC_SEND, (int)msg, 0, 0, 0, 0);
}

int recv(char* buffer) {
    return syscall(SYS_IPC_RECV, (int)buffer, 0, 0, 0, 0);
}

// ==========================================
// 系統控制 (System Control)
// ==========================================

int set_display_mode(int is_gui) {
    return syscall(SYS_SET_DISPLAY_MODE, is_gui, 0, 0, 0, 0);
}

void clear_screen(void) {
    syscall(SYS_CLEAR_SCREEN, 0, 0, 0, 0, 0);
}

void get_time(int* h, int* m) {
    syscall(SYS_GET_TIME, (int)h, (int)m, 0, 0, 0);
}

// Timer
void msleep(uint32_t ms) {
    syscall(SYS_SLEEP, ms, 0, 0, 0, 0);
}

void sleep(uint32_t sec) {
    msleep(sec * 1000);
}

uint32_t get_ticks() {
    return syscall(SYS_GET_TICKS, 0, 0, 0, 0, 0);
}
