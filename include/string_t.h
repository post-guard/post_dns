//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_STRING_T_H
#define POST_DNS_STRING_T_H

#include "defines.h"

#include <string.h>

/**
 * 字符串结构体
 */
typedef struct
{
    char *value;
    int length;
} string_t;

typedef struct
{
    string_t **array;
    int length;
} split_array_t;

/**
 * 初始化一个字符串
 * @param value 字符串字面值
 * @param length 字符串长度
 * @return 字符串结构体指针
 */
string_t *string_malloc(const char *value, int length);

/**
 * 释放字符串结构体占用的内存
 * @param pointer 结构体指针
 */
void string_free(string_t *pointer);

/**
 * 判断两个字符串是否相同
 * @param a 第一个字符串
 * @param b 第二个字符串
 * @return true相等 反之false
 */
bool string_equal(const string_t *a, const string_t *b);

/**
 * 计算字符串的哈希值
 * @param str 字符串指针
 * @return 哈希值
 */
int string_hash_code(const string_t *str);

/**
 * 按照给定的字符分割字符串
 * @param str 需要被分割的字符串
 * @param separator 分割符
 * @return 分割结果数组
 */
split_array_t *string_split(const string_t *str, char separator);

/**
 * 打印字符串
 * 没有换行符
 * @param str 需要被打印的字符串
 */
char *string_print(const string_t *str);

/**
 * 复制指定的字符串
 * @param target 需要复制的字符串
 * @return 复制之后的字符串指针
 */
string_t *string_dup(const string_t *target);

#endif //POST_DNS_STRING_T_H
