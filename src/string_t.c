//
// Created by ricardo on 23-6-16.
//
#include "string_t.h"
#include "stdlib.h"
#include "string.h"

/**
 * murmur3哈希函数
 * 来源 https://en.wikipedia.org/wiki/MurmurHash#cite_note-6
 * @param key
 * @param len
 * @param seed
 * @return
 */
unsigned int murmur3_32(const unsigned char* key, size_t len, unsigned int seed);

string_t *string_malloc(const char *value, int length)
{
    string_t *result = (string_t *) malloc(sizeof(string_t));

    result->value = (char *) malloc(sizeof(char) * length);
    result->length = length;
    memcpy(result->value, value, result->length);

    return result;
}

void string_free(string_t *pointer)
{
    if (pointer == NULL)
    {
        return;
    }

    free(pointer->value);
    pointer->value = NULL;

    free(pointer);
}

bool string_equal(const string_t *a, const string_t *b)
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

int string_hash_code(const string_t *str)
{
    unsigned int seed = 0x54623413;

    return murmur3_32((const unsigned char*)str->value, str->length, seed) & 0x7fffffff;
}

split_array_t *string_split(const string_t *str, char separator)
{
    split_array_t *result = malloc(sizeof(split_array_t));

    // 计算分割之后数组的大小
    result->length = 1;
    for (int i = 0; i < str->length; i++)
    {
        if (str->value[i] == separator)
        {
            result->length++;
        }
    }

    result->array = malloc((sizeof(string_t*) * result->length));

    int last_pos = 0;
    int pos = 0;
    for (int i = 0; i < str->length; i++)
    {
        if (str->value[i] == separator)
        {
            result->array[pos] = string_malloc(&str->value[last_pos], i - last_pos);
            pos++;
            last_pos = i + 1;
        }
    }

    // 最后一个
    result->array[pos] = string_malloc(&str->value[last_pos], str->length - last_pos);

    return result;
}

char *string_print(const string_t *str)
{
    char *print = malloc(sizeof(char) * (str->length + 1));
    print[str->length] = 0;
    strncpy(print, str->value, str->length);
    return print;
}

string_t *string_dup(const string_t *target)
{
    string_t *result = malloc(sizeof(string_t));

    result->value = malloc(sizeof(char ) * target->length);
    memcpy(result->value, target->value, sizeof(char ) * target->length);
    result->length = target->length;

    return result;
}

static inline unsigned int murmur_32_scramble(unsigned int k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

unsigned int murmur3_32(const unsigned char* key, size_t len, unsigned int seed)
{
	unsigned int h = seed;
    unsigned int k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(unsigned int));
        key += sizeof(unsigned int);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}