#include "unistd.h"
#include "syscall.h"


// process
int fork() { return syscall(8, 0, 0, 0, 0, 0); }
int exec(char* filename, char** argv) { return syscall(9, (int)filename, (int)argv, 0, 0, 0); }
int wait(int pid) { return syscall(10, pid, 0, 0, 0, 0); }
void yield() { syscall(6, 0, 0, 0, 0, 0); }
void exit() { syscall(7, 0, 0, 0, 0, 0); while(1); } // 死迴圈防止意外返回

// ipc
void send(char* msg) { syscall(11, (int)msg, 0, 0, 0, 0); }
int recv(char* buffer) { return syscall(12, (int)buffer, 0, 0, 0, 0); }

// file system
void* sbrk(int increment) { return (void*)syscall(13, increment, 0, 0, 0, 0); }

int create_file(const char* filename, const char* content) {
    return syscall(14, (int)filename, (int)content, 0, 0, 0);
}

int remove(const char* filename) {
    return syscall(16, (int)filename, 0, 0, 0, 0);
}

// directory
int readdir(int index, char* out_name, int* out_size, int* out_type) {
    return syscall(15, index, (int)out_name, (int)out_size, (int)out_type, 0);
}

int mkdir(const char* filename) {
    return syscall(17, (int)filename, 0, 0, 0, 0);
}

// change dir: cd
int chdir(const char* dirname) {
    return syscall(18, (int)dirname, 0, 0, 0, 0);
}

// current working directory
int getcwd(char* buffer) {
    return syscall(19, (int)buffer, 0, 0, 0, 0);
}

// 透過組合語言觸發軟體中斷
int set_display_mode(int is_gui) {
    int ret;
    // 假設 128 是你的 syscall 中斷號碼，EAX=10 代表切換模式，EBX=參數
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (99), "b" (is_gui)
    );
    return ret;
}


// 【Day 63 新增】取得自己的 PID
int getpid(void) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (20)
    );
    return ret;
}

// 【Day 63 新增】取得系統行程列表
int get_process_list(process_info_t* list, int max_count) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (21), "b" (list), "c" (max_count)
    );
    return ret;
}

// 【新增】透過中斷 22 清空螢幕
void clear_screen(void) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (22)
    );
}


// 【新增】呼叫死神
int kill(int pid) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (24), "b" (pid)
    );
    return ret;
}

int get_mem_info(mem_info_t* info) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (25), "b" (info)
    );
    return ret;
}

int create_gui_window(const char* title, int width, int height) {
    int ret;
    __asm__ volatile (
        "int $128"
        : "=a" (ret)
        : "a" (26), "b" (title), "c" (width), "d" (height)
    );
    return ret;
}
