//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_RESOURCE_RECORD_H
#define POST_DNS_RESOURCE_RECORD_H
#include "string_t.h"
#include "name_space_definitions.h"
#include "uv.h"

typedef struct {
    string_t *name;
    dns_type type;
    dns_class class;
    int ttl;
    unsigned short response_data_length;
    void *response_data;
} resource_record_t;

uv_buf_t *resource_record2buf(resource_record_t *rr);

resource_record_t *buf2resource_record(uv_buf_t *buf);

#endif //POST_DNS_RESOURCE_RECORD_H
