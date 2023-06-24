#include "stdlib.h"
#include "message.h"
#include "uv.h"
#include "stdio.h"
#include "logging.h"
#include "string.h"

uv_udp_t bind_socket;
uv_udp_t query_socket;
uv_loop_t *loop;

static void udp_alloc_buffer(uv_handle_t *handle, size_t suggest_size, uv_buf_t *buf)
{
    buf->base = malloc(suggest_size);
    buf->len = suggest_size;
}

static void udp_on_send(uv_udp_send_t *req, int status)
{
    if (status)
    {
        log_error("Send Error");
    }

    free(req->bufs);
    free(req);
}

static void udp_on_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags)
{
    if (number <= 0)
    {
        // 实际收到的包长度可能为零
        return;
    }

    // 获得发送方地址
    // 这里获得的buf长度有问题 是65535
    printf("\nhello\n");
    char sender[17] = {};
    uv_ip4_name((struct sockaddr_in *)address, sender, 16);
    printf("Receive data from %s\n", sender);
    printf("Data length: %zu\n", number);
    printf("Data: ");

    buf->base[number] = '\0';
    // 修改buffer末尾加入一个字符串结束标志
    printfUnsignedStr(buf->base,number);

    printf("\n");
    buf2message(buf);
    printf("bye\n");

    log_information("查询上游DNS");
    uv_udp_send_t *send_handler = malloc(sizeof (uv_udp_send_t));
    struct sockaddr_in dns_address;
    uv_ip4_addr("10.3.9.44", 53, &dns_address);

    uv_buf_t *send_buf = malloc(sizeof (uv_buf_t));
    udp_alloc_buffer(NULL, number, send_buf);
    memcpy(send_buf->base, buf->base, number);
    send_buf->len = number;

    uv_udp_send(send_handler, &query_socket, send_buf, 1, (const struct sockaddr*)&dns_address, udp_on_send);
}

static void query_on_read(
        uv_udp_t *request, ssize_t number, const uv_buf_t *buf, const struct sockaddr *address, unsigned flags)
{
    if (number <= 0)
    {
        return;
    }

    log_information("Receiving data from upstream: ");
    printfUnsignedStr(buf->base, number);
    printf("\n");

    log_information("开始分析数据:");
    buf->base[number] = '\0';
    buf2message(buf);
    printf("\n");

}

int main()
{
    loop = uv_default_loop();

    uv_udp_init(loop, &bind_socket);

    struct sockaddr_in bind_address;
    uv_ip4_addr("127.0.0.1", 53, &bind_address);
    uv_udp_bind(&bind_socket, (const struct sockaddr *)&bind_address, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&bind_socket, udp_alloc_buffer, udp_on_read);

    uv_udp_init(loop, &query_socket);
    struct sockaddr_in query_address;
    uv_ip4_addr("0.0.0.0", 0, &query_address);
    uv_udp_bind(&query_socket, (const struct sockaddr*)&query_address, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&query_socket, udp_alloc_buffer, query_on_read);

    return uv_run(loop, UV_RUN_DEFAULT);
}