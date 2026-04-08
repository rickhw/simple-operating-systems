#ifndef _UNISTD_H
#define _UNISTD_H

#include <stdint.h>

/**
 * @file unistd.h
 * @brief 定義標準的 POSIX 系統呼叫封裝介面
 */

// 行程資訊結構 (用於 get_process_list)
typedef struct {
    unsigned int pid;
    unsigned int ppid;
    char name[32];
    unsigned int state;
    unsigned int memory_used;
    unsigned int total_ticks;
} process_info_t;

// 系統記憶體資訊結構 (與核心核心核心同步)
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;

// ==========================================
// 行程管理 (Process Management)
// ==========================================
int fork(void);
int exec(char* filename, char** argv);
int wait(int pid);
void yield(void);
void exit(void) __attribute__((noreturn));
int getpid(void);
int get_process_list(process_info_t* list, int max_count);
int kill(int pid);

// ==========================================
// 檔案系統 (File System)
// ==========================================
int open(const char* filename);
int read(int fd, void* buffer, int size);
int create_file(const char* filename, const char* content);
int remove(const char* filename);
int readdir(int index, char* out_name, int* out_size, int* out_type);
int mkdir(const char* dirname);
int chdir(const char* dirname);
int getcwd(char* buffer);

// ==========================================
// 記憶體管理 (Memory Management)
// ==========================================
void* sbrk(int increment);
int get_mem_info(mem_info_t* info);

// ==========================================
// 行程間通訊 (IPC)
// ==========================================
void send(char* msg);
int recv(char* buffer);

// ==========================================
// 系統控制 (System Control)
// ==========================================
int set_display_mode(int is_gui);
void clear_screen(void);
void get_time(int* h, int* m);

#endif
