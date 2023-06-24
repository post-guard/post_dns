//
// Created by ricardo on 23-6-24.
//

#ifndef POST_DNS_HASH_TABLE_H
#define POST_DNS_HASH_TABLE_H
#include "uv.h"
#include "string_t.h"

#define LOAD_FACTOR 0.75f

typedef struct hash_node {
    string_t *name;
    void *data;
    struct hash_node* next;
} hash_node_t;

typedef struct {
    hash_node_t *table;
    uv_rwlock_t *lock;
    int count;
    int capacity;
    int threshold;
} hash_table_t;

/**
 * 新建哈希表
 * @return
 */
hash_table_t *hash_table_new();

/**
 * 向哈希表中放入数据
 * @param table
 * @param name 数据名称
 * @param data 数据指针
 */
void hash_table_put(hash_table_t *table, string_t *name, void *data);

/**
 * 从哈希表中取出数据
 * @param table
 * @param name 名称
 * @return 数据指针
 */
void *hash_table_get(hash_table_t *table, string_t *name);

/**
 * 从哈希表中移除一个数据
 * @param table
 * @param name 数据名称
 */
void hash_table_remove(hash_table_t *table, string_t *name);

/**
 * 释放哈希表
 * @param table
 */
void hash_table_free(hash_table_t *table);


#endif //POST_DNS_HASH_TABLE_H
