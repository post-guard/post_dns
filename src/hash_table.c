//
// Created by ricardo on 23-6-24.
//
#include "hash_table.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "defines.h"

hash_table_t *hash_table_new()
{
    hash_table_t *table = malloc(sizeof(hash_table_t));

    table->capacity = 8;
    table->count = 0;
    table->threshold = table->capacity * LOAD_FACTOR;
    table->lock = malloc(sizeof(uv_rwlock_t));
    table->table = malloc(sizeof(hash_node_t) * 8);
    memset(table->table, 0, sizeof(hash_node_t) * 8);

    uv_rwlock_init(table->lock);

    return table;
}

/**
 * 想哈希表中添加一个节点
 * @param table 哈希表对象
 * @param name 节点名称
 * @param data 节点数据
 */
static void add_node(hash_table_t *table, string_t *name, void *data)
{
    int index = (string_hash_code(name) & INT_MAX) % table->capacity;

    hash_node_t *node = &table->table[index];

    while (node->name != NULL && node->next != NULL)
    {
        node = node->next;
    }

    if (node->name == NULL)
    {
        // 这个节点中没有数据
        node->name = name;
        node->data = data;
        node->next = NULL;
    }
    else
    {
        // 节点中还含有数据
        hash_node_t *newNode = malloc(sizeof(hash_node_t));
        newNode->name = name;
        newNode->data = data;
        newNode->next = NULL;

        node->next = newNode;
    }

    table->count++;
}

/**
 * 扩大哈希表
 * @param table 哈希表指针
 */
static void resize(hash_table_t *table)
{
    int oldCapacity = table->capacity;
    hash_node_t *oldTable = table->table;

    int newCapacity = (oldCapacity << 1) + 1;
    table->capacity = newCapacity;
    table->threshold = newCapacity * LOAD_FACTOR;
    hash_node_t *newTable = malloc(sizeof(hash_table_t) * newCapacity);
    memset(newTable, 0, sizeof(hash_table_t) * newCapacity);
    table->table = newTable;

    for (int i = 0; i < oldCapacity; i++)
    {
        hash_node_t *node = &oldTable[i];

        // 这里的判断条件有点奇怪
        // 因为在表中的数据是不可能为NULL的
        while (node != NULL and node->name != NULL)
        {
            add_node(table, node->name, node->data);

            node = node->next;
        }

        // 释放链接中节点占用的内存
        hash_node_t *p = oldTable[i].next;

        while (p != NULL)
        {
            hash_node_t *temp = p->next;
            free(p);
            p = temp;
        }
    }

    free(oldTable);
}

void hash_table_put(hash_table_t *table, string_t *name, void *data)
{
    if (name == NULL or data == NULL)
    {
        return;
    }
    uv_rwlock_wrlock(table->lock);

    int index = (string_hash_code(name) & INT_MAX) % table->capacity;
    hash_node_t *node = &table->table[index];

    while (node != NULL and node->name != NULL)
    {
        if (string_equal(node->name, name))
        {
            // 在哈希表中已经有对应的节点
            // 直接修改
            void *oldData = node->data;
            node->data = data;
            free(oldData);
            uv_rwlock_wrunlock(table->lock);
            return;
        }

        node = node->next;
    }

    // 哈希表中没有该节点
    add_node(table, name, data);

    if (table->count > table->threshold)
    {
        resize(table);
    }

    uv_rwlock_wrunlock(table->lock);
}

void *hash_table_get(hash_table_t *table, string_t *name)
{
    int index = (string_hash_code(name) & INT_MAX) % table->capacity;

    hash_node_t *node = &table->table[index];

    while (node != NULL and node->name != NULL)
    {
        if (string_equal(node->name, name))
        {
            return node->data;
        }

        node = node->next;
    }

    return NULL;
}

void hash_table_remove(hash_table_t *table, string_t *name)
{
    int index = (string_hash_code(name) & INT_MAX) % table->capacity;

    hash_node_t *head = &table->table[index];
    hash_node_t *node = head;
    hash_node_t *last_node = NULL;

    // 这里是链表删除
    // 所以比较复杂
    while (node->name != NULL)
    {
        if (string_equal(node->name, name))
        {
            uv_rwlock_wrlock(table->lock);
            if (last_node == NULL)
            {
                // 删除头结点上的数据
                if (node->next == NULL)
                {
                    // 没有下一个节点
                    string_free(node->name);
                    node->name = NULL;
                    free(node->data);
                    node->name = NULL;
                    node->data = NULL;
                }
                else
                {
                    // 含有下一个节点
                    node->name = node->next->name;
                    node->data = node->next->data;
                    node->next = node->next->next;
                    free(node->next);
                }
            }
            else
            {
                // 不是头结点上的数据
                last_node->next = node->next;
                string_free(node->name);
                free(node->data);
                free(node);
            }

            uv_rwlock_wrunlock(table->lock);
            return;
        }

        last_node = node;
        node = node->next;
    }
}

void hash_table_free(hash_table_t *table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        hash_node_t *node = &table->table[i];

        if (node->name != NULL)
        {
            string_free(node->name);
            free(node->data);

            node = node->next;

            // 释放链表的内存
            while (node != NULL)
            {
                hash_node_t *temp = node->next;
                free(node->data);
                string_free(node->name);
                free(node);

                node = temp;
            }
        }
    }

    free(table->table);
    uv_rwlock_destroy(table->lock);
    free(table);
}