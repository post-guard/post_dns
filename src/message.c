//
// Created by ricardo on 23-6-16.
// 解析和封装DNS报文
//
#include "message.h"

uv_buf_t *message2buf(message_t *message){

}

message_t *buf2message(const uv_buf_t *buf){

    // 统一使用大端
    message_t message;

    message.id = ( ( 0 | buf->base[0] ) << 8 ) | buf->base[1];

    char *flags = (char *)malloc(16*sizeof(char));;
    char2bit(buf->base[2],&flags[0]);
    char2bit(buf->base[3],&flags[8]);

    message.flags.QR = flags[0];
    message.flags.Opcode = (flags[1] << 3) + (flags[2] << 2) + (flags[3] << 1) + flags[4];
    message.flags.AA = flags[5];
    message.flags.TC = flags[6];
    message.flags.RD = flags[7];
    message.flags.RA = flags[8];
    message.flags.Z = (flags[9] << 2) + (flags[10] << 1) + flags[11];
    message.flags.RCODE = (flags[12] << 3) + (flags[13] << 2) + (flags[14] << 1) + flags[15];

    printMessage(&message);
    free(flags);
}

void printMessage(message_t *message){

    printf("------DNS Message------\n");
    printf("id: %x\n",message->id);

    printf("Flags\n");

    printf("QR: %c  ",message->flags.QR+48);
    printf(message->flags.QR==0?"Query\n":"Response\n");

    printf("Opcode: %c  ",message->flags.Opcode+48);
    switch (message->flags.Opcode) {
        case 0:
            printf("Standard\n");
            break;
        case 1:
            printf("Inverse\n");
            break;
        case 2:
            printf("Server status request\n");
            break;
        default:
            printf("Unknown\n");
    }

    printf("AA: %c  ",message->flags.AA+48);
    if (message->flags.QR==0) {
        printf("Useless in query\n");
    }
    else {
        printf(message->flags.AA==0?"Server is not an authority\n":"Server is an authority\n");
    }

    printf("TC: %c  ",message->flags.TC+48);
    printf(message->flags.TC==0?"Message is not truncated\n":"Message is truncated\n");

    printf("RD: %c  ",message->flags.RD+48);
    printf(message->flags.RD==0?"Not recursion Desired\n":"Recursion Desired\n");

    printf("RA: %c  ",message->flags.RA+48);
    if (message->flags.QR==0) {
        printf("Useless in query\n");
    }
    else {
        printf(message->flags.RA == 0 ? "Server is not support for recursive query\n" : "Server is support for recursive query\n");
    }

    printf("Z: %c  \n",message->flags.Z+48);

    printf("RCODE: %c  ",message->flags.RCODE+48);
    if (message->flags.QR==0) {
        printf("Useless in query\n");
    }
    else {
        switch (message->flags.RCODE) {
            case 0:
                printf("No error condition\n");
                break;
            case 1:
                printf("Format error\n");
                break;
            case 2:
                printf("Server failure\n");
                break;
            case 3:
                printf("Name Error\n");
                break;
            case 4:
                printf("Not Implemented\n");
                break;
            case 5:
                printf("Refused\n");
                break;
            default:
                printf("Unknown error\n");
        }
    }

    printf("QDCOUNT: %hu\n",message->query_count);
    printf("ANCOUNT: %hu\n",message->answer_count);
    printf("NSCOUNT: %hu\n",message->nameserver_count);
    printf("ARCOUNT: %hu\n",message->additional_count);
}

void char2bit(char ch,char *bit){
    // 将char中的每一位分离到str数组中

    for (int j = 0;j < 8;j++)
    {
        /*
         * 化成二进制时，从右数是第几位就向右边移动几位（注意：下标从零开始）然后在与上1
         * 7-j是因为按大端存储
        */
        bit[7-j] = (ch >> j) & 1;
    }

}
