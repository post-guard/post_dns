//
// Created by ricardo on 23-6-16.
//
#include "string_t.h"
#include "stdlib.h"
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