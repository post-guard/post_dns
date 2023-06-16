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
boolean string_t_equal(const string_t *a, const string_t *b);

#endif //POST_DNS_STRING_T_H
