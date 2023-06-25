//
// Created by ricardo on 23-6-24.
//

#ifndef POST_DNS_SOCKET_H
#define POST_DNS_SOCKET_H
#include "uv.h"
#include "hash_table.h"

extern uv_udp_t query_socket;
extern uv_udp_t bind_socket;

void socket_init();

void socket_free();


#endif //POST_DNS_SOCKET_H
