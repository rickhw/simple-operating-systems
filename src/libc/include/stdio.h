#ifndef _STDIO_H
#define _STDIO_H

void print(const char* str);
char getchar();

int open(char* filename);
int read(int fd, char* buffer, int size);

#endif
