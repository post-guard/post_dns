//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_RESOURCE_RECORD_H
#define POST_DNS_RESOURCE_RECORD_H
#include "string_t.h"
#include "uv.h"

typedef struct {
    string_t *name;
    short type;
    short class;
    int ttl;
    unsigned short response_data_length;
    void *response_data;
} resource_record_t;

typedef struct {
    string_t *name;
    short type;
    short class;
} query_t;

#endif //POST_DNS_RESOURCE_RECORD_H
