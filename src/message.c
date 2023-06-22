//
// Created by ricardo on 23-6-16.
// 解析和封装DNS报文
//
#include "message.h"

uv_buf_t *message2buf(message_t *message){

}

message_t *buf2message(const uv_buf_t *buf){

    for(int i = 0; i < 2; i++){
        printf("%.2x", buf->base[i]);
    }

}

void printMessage(message_t *message){

    printf("------DNS报文------\n");
    printf("id: %hd\n",message->id);

    printf("Flags\n");

    printf("QR: %c  ",message->flags.QR+48);
    printf(message->flags.QR==0?"请求\n":"响应\n");

    printf("Opcode: %c  ",message->flags.Opcode+48);
    switch (message->flags.Opcode) {
        case 0:
            printf("标准查询\n");
            break;
        case 1:
            printf("反转查询\n");
            break;
        case 2:
            printf("服务器状态查询\n");
            break;
        default:
            printf("未知查询\n");
    }

    printf("AA: %c  ",message->flags.AA+48);
    if (message->flags.QR==0) {
        printf("请求字段中无作用\n");
    }
    else {
        printf(message->flags.AA==0?"非权威服务器\n":"权威服务器\n");
    }

    printf("TC: %c  ",message->flags.TC+48);
    printf(message->flags.TC==0?"无截断\n":"已截断\n");

    printf("RD: %c  ",message->flags.RD+48);
    printf(message->flags.RD==0?"不期望递归查询\n":"期望递归查询\n");

    printf("RA: %c  ",message->flags.RA+48);
    if (message->flags.QR==0) {
        printf("请求字段中无作用\n");
    }
    else {
        printf(message->flags.RA == 0 ? "服务器不支持递归查询\n" : "服务器支持递归查询\n");
    }

    printf("Z: %c  ",message->flags.Z+48);

    printf("RCODE: %c  ",message->flags.RCODE+48);
    if (message->flags.QR==0) {
        printf("请求字段中无作用\n");
    }
    else {
        switch (message->flags.RCODE) {
            case 0:
                printf("无错误条件\n");
                break;
            case 1:
                printf("请求格式有误，服务器无法解析请求\n");
                break;
            case 2:
                printf("服务器出错\n");
                break;
            case 3:
                printf("请求中的域名不存在\n");
                break;
            case 4:
                printf("服务器不支持该请求类型\n");
                break;
            case 5:
                printf("服务器拒绝执行请求操作\n");
                break;
            default:
                printf("未知报错信息\n");
        }
    }

    printf("QDCOUNT: %hu\n",message->query_count);
    printf("ANCOUNT: %hu\n",message->answer_count);
    printf("NSCOUNT: %hu\n",message->nameserver_count);
    printf("ARCOUNT: %hu\n",message->additional_count);
}