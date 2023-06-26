#include <stdlib.h>
#include "uv.h"
#include "logging.h"
#include "socket.h"
#include "ipv4_cache.h"
#include "cname_cache.h"
#include "ipv6_cache.h"

uv_loop_t *loop;
uv_signal_t signal_handler;

static void sigint_callback(uv_signal_t *handle, int signum)
{
    if (signum == SIGINT)
    {
        // 收到control-c
        log_information("退出程序");

        socket_free();
        ipv4_cache_free();
        ipv6_cache_free();
        cname_cache_free();

        exit(0);
    }
}

int main(int argc, char **argv)
{
    log_information("程序启动");

    loop = uv_default_loop();
    uv_udp_init(loop, &query_socket);
    uv_udp_init(loop, &bind_socket);
    uv_signal_init(loop, &signal_handler);

    socket_init();
    uv_signal_start(&signal_handler, sigint_callback, SIGINT);
    ipv4_cache_init();
    ipv6_cache_init();
    cname_cache_init();

    return uv_run(loop, UV_RUN_DEFAULT);
}
