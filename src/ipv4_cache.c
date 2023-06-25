//
// Created by ricardo on 23-6-23.
//
#include "ipv4_cache.h"

#include <stdlib.h>
#include "hash_table.h"
#include "logging.h"
#include "utils.h"

hash_table_t *cache_table;

void ipv4_cache_init()
{
    log_information("初始化ipv4缓存表");
    cache_table = hash_table_new();
}

void ipv4_cache_put(string_t *name, ipv4_cache_t cache)
{
    ipv4_cache_t *node = malloc(sizeof(ipv4_cache_t));

    node->addresses = cache.addresses;
    node->ttl = cache.ttl;

    char *domain_print = string_t_print(name);
    for (int i = 0; i < node->address_count; i++)
    {
        string_t *address = inet4address2string(node->addresses[i]);
        char *address_print = string_t_print(address);
        log_information("A记录缓存添加%s-%s", domain_print, address_print);
        free(address_print);
        string_t_free(address);
    }
    free(domain_print);

    hash_table_put(cache_table, name, node);
}

ipv4_cache_t ipv4_cache_get(string_t *name)
{
    return *(ipv4_cache_t *) hash_table_get(cache_table, name);
}

void ipv4_cache_free()
{
    hash_table_free(cache_table);
}

void ipv4_cache_clear()
{
    time_t now = time(NULL);
    // 记录过时需要删除的缓存
    string_t *result[cache_table->count];
    int count = 0;

    for (int i = 0; i < cache_table->capacity; i++)
    {
        hash_node_t *node = &cache_table->table[i];

        while (node != NULL and node->name != NULL)
        {
            ipv4_cache_t *cache = node->data;

            if (cache->timestamp + cache->ttl < now)
            {
                // 顺便free点结构体内部的缓存
                free(cache->addresses);
                cache->addresses = NULL;
                cache->address_count = 0;

                result[count] = node->name;
                count++;
            }

            node = node->next;
        }
    }

    for (int i = 0; i < count; i++)
    {
        hash_table_remove(cache_table, result[i]);
    }
}