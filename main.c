#include "stdlib.h"
#include "message.h"
#include "uv.h"
#include "stdio.h"
#include "string.h"



uv_udp_t bind_socket;

static void udp_alloc_buffer(uv_handle_t *handle, size_t suggest_size, uv_buf_t *buf)
{
    buf->base = malloc(suggest_size);
    buf->len = suggest_size;
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
    for (int i = 0; i < number; i++)
    {
        printf("%.2x", buf->base[i]);
    }
    printf("\n");
    buf2message(buf);
    printf("bye\n");
}

int main()
{
    uv_loop_t *loop = uv_default_loop();

    uv_udp_init(loop, &bind_socket);

    struct sockaddr_in bind_address;
    uv_ip4_addr("127.0.0.1", 53, &bind_address);
    uv_udp_bind(&bind_socket, (const struct sockaddr *)&bind_address, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&bind_socket, udp_alloc_buffer, udp_on_read);

    return uv_run(loop, UV_RUN_DEFAULT);
}