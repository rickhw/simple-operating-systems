#ifndef _LIBC_SYSCALL_H
#define _LIBC_SYSCALL_H

#include <stdint.h>

/**
 * @file syscall.h
 * @brief 定義系統呼叫編號與通用呼叫介面
 */

// ==========================================
// 系統呼叫編號 (與核心定義同步)
// ==========================================
#define SYS_PRINT_STR_LEN    0
#define SYS_PRINT_STR        2
#define SYS_OPEN             3
#define SYS_READ             4
#define SYS_GETCHAR          5
#define SYS_YIELD            6
#define SYS_EXIT             7
#define SYS_FORK             8
#define SYS_EXEC             9
#define SYS_WAIT             10
#define SYS_IPC_SEND         11
#define SYS_IPC_RECV         12
#define SYS_SBRK             13
#define SYS_CREATE           14
#define SYS_READDIR          15
#define SYS_REMOVE           16
#define SYS_MKDIR            17
#define SYS_CHDIR            18
#define SYS_GETCWD           19
#define SYS_GETPID           20
#define SYS_GETPROCS         21
#define SYS_CLEAR_SCREEN     22
#define SYS_KILL             24
#define SYS_GET_MEM_INFO     25
#define SYS_CREATE_WINDOW    26
#define SYS_UPDATE_WINDOW    27
#define SYS_GET_TIME         28
#define SYS_GET_WIN_EVENT    29
#define SYS_GET_WIN_KEY      30
#define SYS_PING             31
#define SYS_NET_UDP_SEND     32
#define SYS_NET_UDP_BIND     33
#define SYS_NET_UDP_RECV     34
#define SYS_NET_TCP_CONNECT  35
#define SYS_NET_TCP_SEND     36
#define SYS_NET_TCP_CLOSE    37
#define SYS_SET_DISPLAY_MODE 99

/**
 * @brief 通用系統呼叫封裝
 * @param num 系統呼叫編號
 * @param p1 參數 1 (ebx)
 * @param p2 參數 2 (ecx)
 * @param p3 參數 3 (edx)
 * @param p4 參數 4 (esi)
 * @param p5 參數 5 (edi)
 * @return 核心回傳值 (eax)
 */
int syscall(int num, int p1, int p2, int p3, int p4, int p5);

#endif
