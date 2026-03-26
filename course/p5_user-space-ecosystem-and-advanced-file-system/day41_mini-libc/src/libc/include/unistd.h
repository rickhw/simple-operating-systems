#ifndef _UNISTD_H
#define _UNISTD_H

int fork();
int exec(char* filename, char** argv);
int wait(int pid);
void yield();
void exit();
void send(char* msg);
int recv(char* buffer);


#endif
