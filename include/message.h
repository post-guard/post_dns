//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_MESSAGE_H
#define POST_DNS_MESSAGE_H
#include "resource_record.h"
typedef struct {
    char QR;
    char Opcode;
    char AA;
    char TC;
    char RD;
    char RA;
    char Z;
    char RCODE;
} flags_t;

typedef struct {
    short id;
    flags_t flags;
    unsigned short query_count;
    unsigned short answer_count;
    unsigned short nameserver_count;
    unsigned short additional_count;
    resource_record_t *queries;
    resource_record_t *answers;
    resource_record_t *authorities;
    resource_record_t *additional;
} message_t;

uv_buf_t *message2buf(message_t *message);

message_t *buf2message(const uv_buf_t *buf);

void printMessage(message_t *message);

void char2bit(char ch,char *bit);

void printfUnsignedStr(const char *base,int length);
#endif //POST_DNS_MESSAGE_H
