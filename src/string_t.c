//
// Created by ricardo on 23-6-16.
//
#include "string_t.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

string_t *string_t_malloc(const char *value, int length)
{
    string_t *result = (string_t *) malloc(sizeof(string_t));

    result->value = (char *) malloc(sizeof(char) * length);
    result->length = length;
    strcpy(result->value, value);

    return result;
}

void string_t_free(string_t *pointer)
{
    if (pointer == NULL)
    {
        return;
    }

    free(pointer->value);
    pointer->value = NULL;

    free(pointer);
}

bool string_t_equal(const string_t *a, const string_t *b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }

    if (a->length != b->length)
    {
        return false;
    }

    bool flag = true;

    for (int i = 0; i < a->length; i++)
    {
        if (a->value[i] != b->value[i])
        {
            flag = false;
            break;
        }
    }

    return flag;
}

int string_t_hash_code(const string_t *str)
{
    int result = 0;

    for (int i = 0; i < str->length; i++)
    {
        result = result * 31 + str->value[i];
    }

    return result;
}

split_array_t *string_t_split(const string_t *str, char separator)
{
    split_array_t *result = malloc(sizeof(split_array_t));

    int last_pos = 0;
    int next_pos = 0;

    while (next_pos < str->length)
    {
        if (str->value[next_pos] == separator)
        {
            string_t *s = malloc(sizeof(string_t));
            s->length = next_pos - last_pos;
            s->value = malloc(sizeof(char) * s->length);
            // 这里是从上次起始位置复制需要的长度到结果字符串中
            strncpy(s->value, &str->value[last_pos], s->length);

            // 扩大字符串数组的大小
            string_t **old_array = result->array;
            result->length = result->length + 1;
            result->array = malloc(sizeof(string_t *) * result->length);
            memcpy(result->array, old_array, sizeof(string_t *) * (result->length - 1));
            free(old_array);
            result->array[result->length - 1] = s;

            next_pos++;
            last_pos = next_pos;
        }

        next_pos++;
    }

    // 字符串最后一个分隔符之后的字串也需要复制
    string_t *s = malloc(sizeof(string_t));
    s->length = next_pos - last_pos;
    s->value = malloc(sizeof(char) * s->length);
    // 这里是从上次起始位置复制需要的长度到结果字符串中
    strncpy(s->value, &str->value[last_pos], s->length);

    // 扩大字符串数组的大小
    string_t **old_array = result->array;
    result->length = result->length + 1;
    result->array = malloc(sizeof(string_t *) * result->length);
    memcpy(result->array, old_array, sizeof(string_t *) * (result->length - 1));
    free(old_array);
    result->array[result->length - 1] = s;

    return result;
}

void string_t_print(const string_t *str)
{
    char *print = malloc(sizeof(char) * (str->length + 1));
    print[str->length] = 0;
    strncpy(print, str->value, str->length);
    printf("%s", print);
    free(print);
}