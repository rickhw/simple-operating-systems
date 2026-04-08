#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

int strlen(const char* s);
void strcpy(char* dest, const char* src);
int strcmp(const char *s1, const char *s2);

int parse_args(char* input, char** argv);

#endif
