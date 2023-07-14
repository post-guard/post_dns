#include "socket.h"

#include <stdlib.h>
#include <string.h>
#include "logging.h"
#include "message.h"
#include "utils.h"
#include "ipv4_cache.h"
#include "cname_cache.h"
#include "ipv6_cache.h"
#include "config.h"

struct sockaddr_in query_address;
uv_udp_t query_socket;
struct sockaddr_in bind_address;
uv_udp_t bind_socket;

struct sockaddr_in dns_address;

hash_table_t *clients_hash_table;

static void query_socket_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags);

static void bind_socket_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags);

static void message_copy_header_queries(message_t *dest, message_t *src);

static void generate_cname_response(resource_record_t *dest, string_t *domain, string_t *name);

static void generate_ipv4_response(resource_record_t *dest, string_t *name, int address);

static void generate_ipv6_response(resource_record_t *dest, string_t *name, unsigned char *address);

static void send_close(uv_handle_t *handler)
{
    log_debug("释放在发送请求的过程中分配的内存");
    uv_buf_t *buf = handler->data;

    if (buf != NULL)
    {
        free(buf->base);
        free(buf);
    }
}


static void send_response(message_t *message)
{
    uv_udp_send_t *send_handler = malloc(sizeof(uv_udp_send_t));

    string_t *feature_vector = message2feature_string(message);

    struct sockaddr *client_address = (struct sockaddr *) hash_table_get(clients_hash_table, feature_vector);

    if (client_address == NULL)
    {
        log_error("未找到client地址, 发送返回包失败");
        free(feature_vector);
        return;
    }

    message->flags.Z = 0;
    message->additional_count = 0;
    message->authority_count = 0;

    uv_buf_t *buf = message2buf(message);
    bind_socket.data = buf;

    uv_udp_send(send_handler, &bind_socket, buf, 1, client_address, udp_on_send);

    hash_table_remove(clients_hash_table, feature_vector);
    free(feature_vector);
}

static void send_query(message_t *message)
{
    uv_udp_send_t *send_handler = malloc(sizeof(uv_udp_send_t));

    // 在rfc1035中Z必须为0
    message->flags.Z = 0;
    message->additional_count = 0;
    message->authority_count = 0;
    uv_buf_t *buf = message2buf(message);
    query_socket.data = buf;

    uv_udp_send(send_handler, &query_socket, buf, 1, (const struct sockaddr *) &dns_address,
                udp_on_send);
}

void socket_init()
{
    // 初始化DNS服务器端口
    uv_ip4_addr("0.0.0.0", 53, &bind_address);
    int result = uv_udp_bind(&bind_socket, (const struct sockaddr *) &bind_address, 0);
    if (result == 0)
    {
        log_information("绑定DNS服务器socket成功");
    }
    else
    {
        log_error("绑定DNS服务器socket失败: %d", result);
        exit(0);
    }
    bind_socket.close_cb = send_close;

    uv_udp_recv_start(&bind_socket, udp_alloc_buffer, bind_socket_read);
    log_information("开始监听DNS服务器socket");

    // 初始化请求上游服务器的接口
    uv_ip4_addr("0.0.0.0", 0, &query_address);
    result = uv_udp_bind(&query_socket, (const struct sockaddr *) &query_address, 0);
    if (result == 0)
    {
        log_information("绑定查询socket成功");
    }
    else
    {
        log_error("绑定查询socket失败: %d", result);
        exit(0);
    }
    query_socket.close_cb = send_close;

    uv_udp_recv_start(&query_socket, udp_alloc_buffer, query_socket_read);
    log_information("开始监听查询socket");

    // 初始化客户端哈希表
    clients_hash_table = hash_table_new();

    // 初始化dns服务器地址
    uv_ip4_addr(dns_config.upstream_name, 53, &dns_address);
}

void socket_free()
{
    uv_udp_recv_stop(&bind_socket);
    uv_udp_recv_stop(&query_socket);

    hash_table_free(clients_hash_table);
}

static void query_socket_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags)
{
    if (number <= 0)
    {
        log_debug("收到上游服务器错误响应: %zd", number);
        return;
    }

    log_information("收到上游返回数据");

    // 解析收到的消息
    uv_buf_t receive_buf = {
            .base = buf->base,
            .len = number
    };
    message_t *message = buf2message(&receive_buf);
    message_log(message);

    // 将得到的数据放入缓存
    // 仍然假设A AAAA记录只有一个域名
    string_t *ipv4_domain = NULL;
    ipv4_cache_t ipv4_cache;
    ipv4_cache.manual = false;
    ipv4_cache.timestamp = time(NULL);
    ipv4_cache.ttl = 120;
    ipv4_cache.node = NULL;
    string_t *ipv6_domain = NULL;
    ipv6_cache_t ipv6_cache;
    ipv6_cache.manual = false;
    ipv6_cache.timestamp = time(NULL);
    ipv6_cache.ttl = 120;
    ipv6_cache.node = NULL;

    for (int i = 0; i < message->answer_count; i++)
    {
        switch (message->answers[i].type)
        {
            case 1:
            {
                // A
                ipv4_domain = string_dup(message->answers[i].name);
                ipv4_node_t *node = malloc(sizeof(ipv4_node_t));
                node->next = NULL;
                node->address = *(int *) message->answers[i].response_data;

                if (ipv4_cache.node == NULL)
                {
                    ipv4_cache.node = node;
                }
                else
                {
                    ipv4_node_t *p = ipv4_cache.node;

                    while (p->next != NULL)
                    {
                        p = p->next;
                    }
                    p->next = node;
                }
                break;
            }
            case 5:
            {
                // CNAME
                // 读取到CNAME就直接放进缓存中
                cname_cache_t cname_cache;
                cname_cache.name = string_dup((string_t *) message->answers[i].response_data);
                cname_cache.timestamp = time(NULL);
                cname_cache.manual = false;
                cname_cache.ttl = 120;

                string_t *cname_domain = string_dup(message->answers[i].name);
                cname_cache_put(cname_domain, &cname_cache);
                break;
            }
            case 28:
            {
                // AAAA
                ipv6_domain = string_dup(message->answers[i].name);
                ipv6_node_t *node = malloc(sizeof(ipv6_node_t));
                node->next = NULL;
                memcpy(node->address, message->answers[i].response_data, 16);

                if (ipv6_cache.node == NULL)
                {
                    ipv6_cache.node = node;
                }
                else
                {
                    ipv6_node_t *p = ipv6_cache.node;

                    while (p->next != NULL)
                    {
                        p = p->next;
                    }
                    p->next = node;
                }
                break;
            }
        }
    }

    if (ipv4_domain != NULL)
    {
        ipv4_cache_put(ipv4_domain, &ipv4_cache);
    }
    if (ipv6_domain != NULL)
    {
        ipv6_cache_put(ipv6_domain, &ipv6_cache);
    }

    // 发送DNS回复包
    send_response(message);
    message_free(message);
}

static void bind_socket_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags
)
{
    if (number <= 0)
    {
        return;
    }

    char client_address[17];
    uv_ip4_name((const struct sockaddr_in *) address, client_address, 16);
    log_information("DNS服务器收到%s请求", client_address);

    uv_buf_t receive_buf = {
            .base = buf->base,
            .len = number
    };
    message_t *message = buf2message(&receive_buf);
    message_log(message);

    // 将客户端地址放入哈希表
    // 放入哈希表的值由哈希表负责释放
    struct sockaddr *client_socket = malloc(sizeof(struct sockaddr));
    memcpy(client_socket, address, sizeof(struct sockaddr));
    hash_table_put(clients_hash_table, message2feature_string(message), client_socket);

    message_t *back_message = malloc(sizeof(message_t));
    message_copy_header_queries(back_message, message);
    back_message->flags.RCODE = 0;
    back_message->flags.QR = 1;
    back_message->flags.RD = 0;

    // 判断是否命中缓存
    // 第一遍循环
    // 判断是否命中缓存以及计算回复消息中answers的个数
    int answer_count = 0;
    bool cached = true;
    // 判断是否被手动禁止
    bool banned = false;
    for (int i = 0; cached and i < message->query_count; i++)
    {
        switch (message->queries[i].type)
        {
            case 1:
            {
                // A

                // 首先处理CNAME
                string_t *domain = message->queries[i].name;
                cname_cache_t *cname_cache = cname_cache_get(domain);

                while (cname_cache != NULL)
                {
                    answer_count++;
                    domain = cname_cache->name;
                    cname_cache = cname_cache_get(domain);
                }

                ipv4_cache_t *cache = ipv4_cache_get(domain);
                if (cache == NULL)
                {
                    cached = false;
                    break;
                }
                else
                {
                    if ((cache->manual = true and cache->node->address == 0))
                    {
                        banned = true;
                        break;
                    }

                    ipv4_node_t *node = cache->node;

                    while (node != NULL)
                    {
                        answer_count++;
                        node = node->next;
                    }
                }
                break;
            }
            case 5:
            {
                // CNAME
                string_t *domain = message->queries[i].name;
                cname_cache_t *cname_cache = cname_cache_get(domain);

                if (cname_cache == NULL)
                {
                    cached = false;
                    break;
                }

                while (cname_cache != NULL)
                {
                    answer_count++;
                    domain = cname_cache->name;
                    cname_cache = cname_cache_get(domain);
                }
                break;
            }
            case 28:
            {
                // AAAA
                // 首先处理CNAME
                string_t *domain = message->queries[i].name;
                cname_cache_t *cname_cache = cname_cache_get(domain);

                while (cname_cache != NULL)
                {
                    answer_count++;
                    domain = cname_cache->name;
                    cname_cache = cname_cache_get(domain);
                }

                ipv6_cache_t *cache = ipv6_cache_get(domain);
                if (cache == NULL)
                {
                    cached = false;
                    break;
                }
                else
                {
                    ipv6_node_t *node = cache->node;
                    while (node != NULL)
                    {
                        answer_count++;
                        node = node->next;
                    }
                }
                break;
            }
        }
    }

    if (!cached)
    {
        // 没有缓存
        send_query(message);
    }
    else
    {
        if (banned)
        {
            // 被封禁
            back_message->flags.RCODE = 3;
        }
        else
        {
            // 第二遍循环
            // 生成返回的消息
            back_message->answer_count = answer_count;
            back_message->answers = malloc(sizeof(resource_record_t) * answer_count);
            answer_count = 0;

            for (int i = 0; i < message->query_count; i++)
            {
                switch (message->queries[i].type)
                {
                    case 1:
                    {
                        // A
                        // 首先处理CNAME
                        string_t *domain = message->queries[i].name;
                        cname_cache_t *cname_cache = cname_cache_get(domain);

                        while (cname_cache != NULL)
                        {
                            generate_cname_response(&back_message->answers[answer_count], domain,
                                                    cname_cache->name);
                            answer_count++;
                            domain = cname_cache->name;
                            cname_cache = cname_cache_get(domain);
                        }

                        ipv4_cache_t *cache = ipv4_cache_get(domain);
                        if (cache == NULL)
                        {
                            log_warning("?");
                        }
                        else
                        {
                            ipv4_node_t *node = cache->node;

                            while (node != NULL)
                            {
                                generate_ipv4_response(&back_message->answers[answer_count], domain,
                                                       node->address);
                                answer_count++;

                                node = node->next;
                            }
                        }
                        break;
                    }
                    case 5:
                    {
                        // CNAME
                        string_t *domain = message->queries[i].name;
                        cname_cache_t *cname_cache = cname_cache_get(domain);

                        while (cname_cache != NULL)
                        {
                            generate_cname_response(&back_message->answers[answer_count], domain,
                                                    cname_cache->name);
                            answer_count++;
                            domain = cname_cache->name;
                            cname_cache = cname_cache_get(domain);
                        }
                        break;
                    }
                    case 28:
                    {
                        // AAAA
                        // 首先处理CNAME
                        string_t *domain = message->queries[i].name;
                        cname_cache_t *cname_cache = cname_cache_get(domain);

                        while (cname_cache != NULL)
                        {
                            generate_cname_response(&back_message->answers[answer_count], domain,
                                                    cname_cache->name);
                            answer_count++;
                            domain = cname_cache->name;
                            cname_cache = cname_cache_get(domain);
                        }

                        ipv6_cache_t *cache = ipv6_cache_get(domain);
                        if (cache == NULL)
                        {
                            log_warning("?");
                        }
                        else
                        {
                            ipv6_node_t *node = cache->node;

                            while (node != NULL)
                            {
                                generate_ipv6_response(&back_message->answers[answer_count], domain,
                                                       node->address);
                                answer_count++;

                                node = node->next;
                            }
                        }
                        break;
                    }
                }
            }
        }

        send_response(back_message);
    }

    free(back_message);
    message_free(message);
}

static void message_copy_header_queries(message_t *dest, message_t *src)
{
    dest->id = src->id;
    dest->flags.TC = src->flags.TC;
    dest->flags.AA = src->flags.AA;
    dest->flags.RA = src->flags.RA;
    dest->flags.RD = src->flags.RD;
    dest->flags.Opcode = src->flags.Opcode;

    dest->queries = malloc(sizeof(resource_record_t) * src->query_count);
    for (int i = 0; i < src->query_count; i++)
    {
        dest->queries[i].name = string_dup(src->queries[i].name);
        dest->queries[i].type = src->queries[i].type;
        dest->queries[i].class = src->queries[i].class;
    }
    dest->query_count = src->query_count;
    dest->answers = NULL;
    dest->answer_count = 0;
    dest->authorities = NULL;
    dest->authority_count = 0;
    dest->additional = NULL;
    dest->additional_count = 0;
}

static void generate_cname_response(resource_record_t *dest, string_t *domain, string_t *name)
{
    dest->name = string_dup(domain);
    dest->response_data = string_dup(name);
    dest->response_data_length = domain->length;
    dest->type = 5;
    dest->class = 1;
    dest->ttl = 120;
}

static void generate_ipv4_response(resource_record_t *dest, string_t *name, int address)
{
    dest->name = string_dup(name);
    dest->response_data = malloc(4);
    *(int *) dest->response_data = address;
    dest->response_data_length = 4;
    dest->type = 1;
    dest->class = 1;
    dest->ttl = 120;
}

static void generate_ipv6_response(resource_record_t *dest, string_t *name, unsigned char *address)
{
    dest->name = string_dup(name);
    dest->response_data = malloc(16);
    memcpy(dest->response_data, address, 16);
    dest->response_data_length = 16;
    dest->type = 28;
    dest->class = 1;
    dest->ttl = 120;
}