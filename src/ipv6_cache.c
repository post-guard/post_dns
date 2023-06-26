//
// Created by ricardo on 23-6-26.
//
#include "ipv6_cache.h"

#include <stdlib.h>
#include "hash_table.h"
#include "logging.h"
#include "utils.h"

hash_table_t *ipv6_table;

void ipv6_cache_init()
{
    ipv6_table = hash_table_new();
    log_information("初始化ipv6缓存表");
}

void ipv6_cache_put(string_t *name, ipv6_cache_t *cache)
{
    ipv6_cache_t *old_cache = ipv6_cache_get(name);

    if (old_cache == NULL)
    {
        old_cache = malloc(sizeof(ipv6_cache_t));
        old_cache->ttl = cache->ttl;
        old_cache->timestamp = time(NULL);
        old_cache->node = cache->node;
        old_cache->manual = false;

        hash_table_put(ipv6_table, name, old_cache);
    }
    else
    {
        old_cache->ttl = cache->ttl;
        old_cache->timestamp = time(NULL);
        old_cache->manual = false;
        free(old_cache->node);
        old_cache->node = cache->node;
    }

    // 打印添加缓存日志
    char *domain_print = string_print(name);
    ipv6_node_t *node = cache->node;
    while (node != NULL)
    {
        string_t *address = inet6address2string(node->address);
        char *address_print = string_print(address);
        log_information("AAAA记录缓存添加%s-%s", domain_print, address_print);
        free(address_print);
        string_free(address);

        node = node->next;
    }
    free(domain_print);
}

ipv6_cache_t *ipv6_cache_get(string_t *name)
{
    return hash_table_get(ipv6_table, name);
}

void ipv6_cache_free()
{
    hash_table_free(ipv6_table);
}

void ipv6_cache_clear()
{
    time_t now = time(NULL);
    // 记录过时需要删除的缓存
    string_t *result[ipv6_table->count];
    int count = 0;

    for (int i = 0; i < ipv6_table->capacity; i++)
    {
        hash_node_t *node = &ipv6_table->table[i];

        while (node != NULL and node->name != NULL)
        {
            ipv6_cache_t *cache = node->data;

            if (cache->timestamp + cache->ttl < now)
            {
                // 顺便free点结构体内部的缓存
                ipv6_node_t *p = cache->node;
                while (p != NULL)
                {
                    ipv6_node_t *next_p = p->next;
                    free(p);
                    p = next_p;
                }

                result[count] = node->name;
                count++;
            }

            node = node->next;
        }
    }

    for (int i = 0; i < count; i++)
    {
        hash_table_remove(ipv6_table, result[i]);
    }
}

