#include "socket.h"

#include <stdlib.h>
#include <string.h>
#include "logging.h"
#include "message.h"
#include "utils.h"

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

    struct sockaddr *client_address = (struct sockaddr*) hash_table_get(clients_hash_table, feature_vector);

    if (client_address == NULL)
    {
        log_error("未找到client地址, 发送返回包失败");
        return;
    }

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
    int result = uv_udp_bind(&bind_socket, (const struct sockaddr*)&bind_address, 0);
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
    result = uv_udp_bind(&query_socket, (const struct sockaddr *)&query_address, 0);
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

    // 发送DNS回复包
    send_response(message);
    //message_free(message);
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
    uv_ip4_name((const struct sockaddr_in*)address, client_address, 16);
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

    log_information("递归上游服务器");
    send_query(message);
    //message_free(message);
}