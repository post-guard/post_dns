#include "stdlib.h"
#include "uv.h"
#include "stdio.h"

int main()
{
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    uv_run(loop, UV_RUN_DEFAULT);

    // QUIT
    uv_loop_close(loop);
    free(loop);

    printf("Hello, libuv!\n");
    return 0;
}