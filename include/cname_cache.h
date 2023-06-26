//
// Created by ricardo on 23-6-26.
//

#ifndef POST_DNS_CNAME_CACHE_H
#define POST_DNS_CNAME_CACHE_H
#include <time.h>
#include "string_t.h"

typedef struct {
    string_t *name;
    int ttl;
    time_t timestamp;
    bool manual;
} cname_cache_t;

void cname_cache_init();

void cname_cache_put(string_t *name, cname_cache_t *cache);

cname_cache_t *cname_cache_get(string_t *name);

void cname_cache_free();

void cname_cache_clear();
#endif //POST_DNS_CNAME_CACHE_H
