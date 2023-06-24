//
// Created by ricardo on 23-6-24.
//

#ifndef POST_DNS_UTILS_H
#define POST_DNS_UTILS_H
#include "string_t.h"

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
string_t *inet6address2string(unsigned long long address);

/**
 * 将地址字符串转换为IPv6地址
 * @param address 地址字符串
 * @return IPv6地址
 */
unsigned long long string2inet6address(string_t *address);

#endif //POST_DNS_UTILS_H
