#include "socket.h"

#include <stdlib.h>
#include <string.h>
#include "logging.h"
#include "message.h"
#include "utils.h"
#include "ipv4_cache.h"

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

static void send_close(uv_handle_t *handler)
{
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
    uv_ip4_addr("10.3.9.44", 53, &dns_address);
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

    ipv4_cache_t cache;
    string_t *ipv4_name;
    memset(&cache, 0, sizeof(ipv4_cache_t));
    for (int i = 0; i < message->answer_count; i++)
    {
        switch (message->answers[i].type)
        {
            case 1:
            {
                ipv4_name = string_dup(message->answers[i].name);
                cache.address_count++;
            }
        }
    }

    cache.addresses = malloc(sizeof(int) * cache.address_count);
    int ipv4_cache_count = 0;
    for (int i = 0; i < message->answer_count; i++)
    {
        switch (message->answers[i].type)
        {
            case 1:
            {
                cache.addresses[ipv4_cache_count] = *(int *)(message->answers[i].response_data);
            }
        }
    }

    if (cache.address_count > 0)
    {
        ipv4_cache_put(ipv4_name, &cache);
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

    // 判断是否命中缓存
    // 现在假设一个消息中只有一个query
    message_t *back_message = malloc(sizeof(message_t));
    back_message->id = message->id;
    back_message->flags.TC = message->flags.TC;
    back_message->flags.AA = message->flags.AA;
    back_message->flags.RA = message->flags.RA;
    back_message->flags.QR = message->flags.QR;
    back_message->flags.RD = message->flags.RD;
    back_message->flags.Opcode = message->flags.Opcode;

    back_message->queries = malloc(sizeof(resource_record_t) * message->query_count);
    back_message->queries[0].name = string_dup(message->queries[0].name);
    back_message->queries[0].type = message->queries[0].type;
    back_message->queries[0].class = message->queries[0].class;
    back_message->query_count = message->query_count;
    back_message->answers = NULL;
    back_message->answer_count = 0;
    back_message->authorities = NULL;
    back_message->authority_count = 0;
    back_message->additional = NULL;
    back_message->additional_count = 0;

    switch (message->queries[0].type)
    {
        case 1:
        {
            // A
            ipv4_cache_t *cache = ipv4_cache_get(message->queries[0].name);

            if (cache == NULL)
            {
                goto NO_CACHE;
            }

            back_message->answers = malloc(sizeof(resource_record_t) * cache->address_count);
            back_message->answer_count = cache->address_count;

            for (int j = 0; j < cache->address_count; j++)
            {
                resource_record_t *rr = &back_message->answers[j];
                rr->name = string_dup(message->queries[0].name);
                rr->type = 1;
                rr->class = 1;
                rr->ttl = 120;
                rr->response_data = &cache->addresses[0];
                rr->response_data_length = 4;
            }
        }
        default:
            break;
    }

    send_response(back_message);
    message_free(back_message);
    message_free(message);
    return;

    NO_CACHE:
    log_information("递归上游服务器");
    send_query(message);
    message_free(message);
}