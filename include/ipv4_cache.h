//
// Created by ricardo on 23-6-23.
//

#ifndef POST_DNS_IPV4_CACHE_H
#define POST_DNS_IPV4_CACHE_H
#include "string_t.h"
#include <time.h>

typedef struct {
    int *addresses;
    int address_count;
    int ttl;
    time_t timestamp;
    bool manual;
} ipv4_cache_t;

void ipv4_cache_init();

void ipv4_cache_put(string_t *name, ipv4_cache_t *cache);

ipv4_cache_t *ipv4_cache_get(string_t *name);

void ipv4_cache_free();

void ipv4_cache_clear();

#endif //POST_DNS_IPV4_CACHE_H
