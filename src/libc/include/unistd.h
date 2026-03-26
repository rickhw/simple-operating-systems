#ifndef _UNISTD_H
#define _UNISTD_H

int fork();
int exec(char* filename, char** argv);
int wait(int pid);
void yield();
void exit();
void send(char* msg);
int recv(char* buffer);
void* sbrk(int increment);

int create_file(const char* filename, const char* content);
int readdir(int index, char* out_name, int* out_size, int* out_type);
int remove(const char* filename);

int mkdir(const char* dirname);
int chdir(const char* dirname);
int getcwd(char* buffer);
#endif
