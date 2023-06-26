//
// Created by ricardo on 23-6-26.
//

#ifndef POST_DNS_IPV6_CACHE_H
#define POST_DNS_IPV6_CACHE_H
#include "string_t.h"
#include <time.h>

typedef struct ipv6_node {
    unsigned char address[16];
    struct ipv6_node *next;
} ipv6_node_t;

typedef struct {
    ipv6_node_t *node;
    int ttl;
    time_t timestamp;
    bool manual;
} ipv6_cache_t;

void ipv6_cache_init();

void ipv6_cache_put(string_t *name, ipv6_cache_t *cache);

ipv6_cache_t *ipv6_cache_get(string_t *name);

void ipv6_cache_free();

void ipv6_cache_clear();
#endif //POST_DNS_IPV6_CACHE_H
