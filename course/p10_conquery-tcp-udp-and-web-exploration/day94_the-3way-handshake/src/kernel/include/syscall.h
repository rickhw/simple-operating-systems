#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "task.h"

// ==========================================
// 系統呼叫編號定義 (Syscall Numbers)
// ==========================================
#define SYS_PRINT_STR_LEN   0
#define SYS_PRINT_STR       2
#define SYS_OPEN            3
#define SYS_READ            4
#define SYS_GETCHAR         5
#define SYS_YIELD           6
#define SYS_EXIT            7
#define SYS_FORK            8
#define SYS_EXEC            9
#define SYS_WAIT            10
#define SYS_IPC_SEND        11
#define SYS_IPC_RECV        12
#define SYS_SBRK            13
#define SYS_CREATE          14
#define SYS_READDIR         15
#define SYS_REMOVE          16
#define SYS_MKDIR           17
#define SYS_CHDIR           18
#define SYS_GETCWD          19
#define SYS_GETPID          20
#define SYS_GETPROCS        21
#define SYS_CLEAR_SCREEN    22
#define SYS_KILL            24
#define SYS_GET_MEM_INFO    25
#define SYS_CREATE_WINDOW   26
#define SYS_UPDATE_WINDOW   27
#define SYS_GET_TIME        28
#define SYS_GET_WIN_EVENT   29
#define SYS_GET_WIN_KEY     30
#define SYS_PING            31
#define SYS_SEND_UDP        32
#define SYS_NET_UDP_BIND    33
#define SYS_NET_UDP_RECV    34
#define SYS_NET_TCP_CONNECT 35
#define SYS_SET_DISPLAY_MODE 99

/**
 * @brief 系統記憶體狀態資訊
 */
typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;

/** @brief 系統呼叫處理函式原型 */
typedef int (*syscall_t)(registers_t*);

/** @brief 初始化系統呼叫子系統 */
void init_syscalls(void);

/**
 * @brief 系統呼叫主要分派器 (Dispatcher)
 * @param regs 中斷發生時的暫存器快照
 */
void syscall_handler(registers_t *regs);

#endif
