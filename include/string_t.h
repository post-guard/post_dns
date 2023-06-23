//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_STRING_T_H
#define POST_DNS_STRING_T_H
#include "boolean.h"

/**
 * 字符串结构体
 */
typedef struct {
    char *value;
    int length;
} string_t;

/**
 * 初始化一个字符串
 * @param value 字符串字面值
 * @param length 字符串长度
 * @return 字符串结构体指针
 */
string_t *string_t_malloc(const char *value, int length);

/**
 * 释放字符串结构体占用的内存
 * @param pointer 结构体指针
 */
void string_t_free(string_t *pointer);

/**
 * 判断两个字符串是否相同
 * @param a 第一个字符串
 * @param b 第二个字符串
 * @return true相等 反之false
 */
bool string_t_equal(const string_t *a, const string_t *b);

/**
 * 计算字符串的哈希值
 * @param str 字符串指针
 * @return 哈希值
 */
int string_t_hash_code(const string_t *str);

#endif //POST_DNS_STRING_T_H
