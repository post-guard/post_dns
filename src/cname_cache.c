//
// Created by ricardo on 23-6-26.
//
#include "cname_cache.h"

#include <stdlib.h>
#include "hash_table.h"
#include "logging.h"

hash_table_t *cname_table;

void cname_cache_init()
{
    log_information("初始化cname缓存");
    cname_table = hash_table_new();
}

void cname_cache_put(string_t *name, cname_cache_t *cache)
{
    cname_cache_t *item = malloc(sizeof(cname_cache_t));

    item->name = cache->name;
    item->ttl = cache->ttl;
    item->timestamp = time(NULL);
    item->manual = false;

    char *key = string_print(name);
    char *value = string_print(item->name);
    log_information("cname缓存表添加%s->%s", key, value);
    free(key);
    free(value);

    hash_table_put(cname_table, name, item);
}

cname_cache_t *cname_cache_get(string_t *name)
{
    return hash_table_get(cname_table, name);
}

void cname_cache_free()
{
    hash_table_free(cname_table);
}

void cname_cache_clear()
{

}


