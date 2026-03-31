#ifndef _UNISTD_H
#define _UNISTD_H

// 【Day 63 新增】
typedef struct {
    unsigned int pid;
    unsigned int ppid;
    char name[32];
    unsigned int state;
    unsigned int memory_used;
    unsigned int total_ticks; // <--- 確保這行有加進去，且用 unsigned int！
} process_info_t;

// 取得記憶體資訊
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;

int fork();
int exec(char* filename, char** argv);
int wait(int pid);
void yield();
void exit();

void send(char* msg);
int recv(char* buffer);

void* sbrk(int increment);
int create_file(const char* filename, const char* content);
int remove(const char* filename);

int readdir(int index, char* out_name, int* out_size, int* out_type);
int mkdir(const char* dirname);
int chdir(const char* dirname);
int getcwd(char* buffer);

int set_display_mode(int is_gui);

// Process API
int getpid(void);
int get_process_list(process_info_t* list, int max_count);
void clear_screen(void); // 【新增】清空螢幕 API
int kill(int pid); // 【新增】

#endif
