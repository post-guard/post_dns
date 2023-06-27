//
// Created by ricardo on 23-6-23.
//
#include "ipv4_cache.h"

#include <stdlib.h>
#include "hash_table.h"
#include "logging.h"
#include "utils.h"

hash_table_t *ipv4_table;

void ipv4_cache_init()
{
    log_information("初始化ipv4缓存表");
    ipv4_table = hash_table_new();
}

void ipv4_cache_put(string_t *name, ipv4_cache_t *cache)
{
    ipv4_cache_t *old_cache = ipv4_cache_get(name);

    if (old_cache == NULL)
    {
        old_cache = malloc(sizeof(ipv4_cache_t));
        old_cache->ttl = cache->ttl;
        old_cache->timestamp = time(NULL);
        old_cache->node = cache->node;
        old_cache->manual = false;

        hash_table_put(ipv4_table, name, old_cache);
    }
    else
    {
        if (old_cache->manual == true)
        {
            // 手动维护的缓存不清理
            return;
        }

        old_cache->ttl = cache->ttl;
        old_cache->timestamp = time(NULL);
        old_cache->manual = false;
        free(old_cache->node);
        old_cache->node = cache->node;
    }

    // 打印添加缓存日志
    char *domain_print = string_print(name);
    ipv4_node_t *node = cache->node;
    while (node != NULL)
    {
        string_t *address = inet4address2string(node->address);
        char *address_print = string_print(address);
        log_information("A记录缓存添加%s-%s", domain_print, address_print);
        free(address_print);
        string_free(address);

        node = node->next;
    }
    free(domain_print);
}

ipv4_cache_t *ipv4_cache_get(string_t *name)
{
    return hash_table_get(ipv4_table, name);
}


void ipv4_cache_free()
{
    hash_table_free(ipv4_table);
}

void ipv4_cache_clear()
{
    time_t now = time(NULL);
    // 记录过时需要删除的缓存
    string_t *result[ipv4_table->count];
    int count = 0;

    for (int i = 0; i < ipv4_table->capacity; i++)
    {
        hash_node_t *node = &ipv4_table->table[i];

        while (node != NULL and node->name != NULL)
        {
            ipv4_cache_t *cache = node->data;

            if (cache->manual == false and cache->timestamp + cache->ttl < now)
            {
                // 顺便free点结构体内部的缓存
                ipv4_node_t *p = cache->node;
                while (p != NULL)
                {
                    ipv4_node_t *next_p = p->next;
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
        hash_table_remove(ipv4_table, result[i]);
    }
}

void ipv4_read_file(const char *file_name)
{
    if (file_name == NULL)
    {
        return;
    }

    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        log_warning("读取ipv4配置文件失败");
        return;
    }

    while (true)
    {
        char buf[1024];
        char *r = fgets(buf, 1024, file);
        if (r == NULL)
        {
            break;
        }

        // 行的末尾有一个换行符
        // 需要去掉
        string_t *result = string_malloc(buf, strlen(buf) - 1);
        split_array_t *array = string_split(result, ' ');
        if (array->length == 2)
        {
            char address[array->array[1]->length + 1];
            address[array->array[1]->length + 1] = '\0';
            memcpy(address, array->array[1]->value, array->array[1]->length);

            struct sockaddr_in address_in;
            uv_ip4_addr(address, 0, &address_in);

            ipv4_cache_t cache = {
                    .timestamp = -1,
                    .manual = true,
                    .ttl = 1,
            };
            cache.node = malloc(sizeof(ipv4_node_t));
            swap32(&address_in.sin_addr.s_addr);
            cache.node->address = address_in.sin_addr.s_addr;
            cache.node->next = NULL;
            ipv4_cache_put(string_dup(array->array[0]), &cache);
        }
        else
        {
            log_warning("非法的ipv4配置: %s", buf);
        }

        for (int i = 0; i < array->length; i++)
        {
            string_free(array->array[i]);
        }
        free(array);
        string_free(result);
    }

    fclose(file);
}