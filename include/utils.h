//
// Created by ricardo on 23-6-24.
//

#ifndef POST_DNS_UTILS_H
#define POST_DNS_UTILS_H
#include "string_t.h"
#include "uv.h"

/**
 * 将IPv4地址转换为字符串
 * @param address IPv4地址
 * @return 地址字符串
 */
string_t *inet4address2string(unsigned int address);

/**
 * 将地址字符串转换为IPv4地址
 * @param address 地址字符串
 * @return IPv4地址
 */
unsigned int string2inet4address(string_t *address);

/**
 * 将IPv6地址转换为字符串
 * @param address IPv6地址
 * @return 地址字符串
 */
string_t *inet6address2string(const unsigned char * address);

/**
 * 将地址字符串转换为IPv6地址
 * @param address 地址字符串
 * @return IPv6地址
 */
unsigned char *string2inet6address(string_t *address);

/**
 * 两个char合成一个unsigned short
 * @param high 合成的高位
 * @param low 合成的低位
 * @return 合成结果
 */
unsigned short char2Short(char high, char low);

/**
 * UDP分配临时空间辅助函数
 */
void udp_alloc_buffer(uv_handle_t *handle, size_t suggest_size, uv_buf_t *buf);

/**
 * UDP发送回调函数
 */
void udp_on_send(uv_udp_send_t *req, int status);

/**
 * 大小端转换函数(short)
 * @param p 起始地址
 */
void swap16(void * p);

/**
 * 大小端转换函数(int)
 * @param p 起始地址
 */
void swap32(void * p);

/**
 * 大小端转换函数(longlong)
 * @param p 起始地址
 */
void swap64(void * p);
#endif //POST_DNS_UTILS_H
