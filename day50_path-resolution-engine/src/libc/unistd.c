#include "unistd.h"
#include "syscall.h"

int fork() { return syscall(8, 0, 0, 0, 0, 0); }
int exec(char* filename, char** argv) { return syscall(9, (int)filename, (int)argv, 0, 0, 0); }
int wait(int pid) { return syscall(10, pid, 0, 0, 0, 0); }
void yield() { syscall(6, 0, 0, 0, 0, 0); }
void exit() { syscall(7, 0, 0, 0, 0, 0); while(1); } // 死迴圈防止意外返回

void send(char* msg) { syscall(11, (int)msg, 0, 0, 0, 0); }
int recv(char* buffer) { return syscall(12, (int)buffer, 0, 0, 0, 0); }

void* sbrk(int increment) { return (void*)syscall(13, increment, 0, 0, 0, 0); }

int create_file(const char* filename, const char* content) {
    return syscall(14, (int)filename, (int)content, 0, 0, 0);
}

int readdir(int index, char* out_name, int* out_size, int* out_type) {
    return syscall(15, index, (int)out_name, (int)out_size, (int)out_type, 0);
}

int remove(const char* filename) {
    return syscall(16, (int)filename, 0, 0, 0, 0);
}

int mkdir(const char* filename) {
    return syscall(17, (int)filename, 0, 0, 0, 0);
}

int chdir(const char* dirname) {
    return syscall(18, (int)dirname, 0, 0, 0, 0);
}

int getcwd(char* buffer) {
    return syscall(19, (int)buffer, 0, 0, 0, 0);
}
