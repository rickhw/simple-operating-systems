#ifndef _LIBC_NET_H
#define _LIBC_NET_H

#include <stdint.h>

/**
 * @file net.h
 * @brief 網路相關工具與系統呼叫介面
 */

/**
 * @brief 簡易的字串轉 IP 陣列函式
 * @param str IP 字串 (例如 "10.0.2.2")
 * @param ip 輸出的 4 位元組 IP 陣列
 */
void parse_ip(char* str, uint8_t* ip);

/**
 * @brief 發送 ICMP Ping 請求
 * @param ip 目標 IP 陣列
 */
void sys_ping(uint8_t* ip);

// UDP
void sys_udp_send(uint8_t* ip, uint16_t port, uint8_t* data, int len);
void sys_udp_bind(uint16_t port);
int sys_udp_recv(char* buffer);

// TCP
void sys_tcp_connect(uint8_t* ip, uint16_t port);
void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg);
void sys_tcp_close(uint8_t* ip, uint16_t port);
int sys_tcp_recv(char* buffer);

#endif
