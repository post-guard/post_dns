//
// Created by ricardo on 23-6-23.
//

#ifndef POST_DNS_IPV4_CACHE_H
#define POST_DNS_IPV4_CACHE_H
#include "string_t.h"
#include <time.h>

typedef struct ipv4_node {
    int address;
    struct ipv4_node *next;
} ipv4_node_t;

typedef struct {
    ipv4_node_t *node;
    int ttl;
    time_t timestamp;
    bool manual;
} ipv4_cache_t;

void ipv4_cache_init();

void ipv4_cache_put(string_t *name, ipv4_cache_t *cache);

ipv4_cache_t *ipv4_cache_get(string_t *name);

void ipv4_cache_free();

void ipv4_cache_clear();

void ipv4_read_file(const char *file_name);

#endif //POST_DNS_IPV4_CACHE_H
