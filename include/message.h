//
// Created by ricardo on 23-6-16.
//

#ifndef POST_DNS_MESSAGE_H
#define POST_DNS_MESSAGE_H

#include "resource_record.h"

typedef struct
{
    char QR;
    char Opcode;
    char AA;
    char TC;
    char RD;
    char RA;
    char Z;
    char RCODE;
} flags_t;

typedef struct
{
    short id;
    flags_t flags;
    unsigned short query_count;
    unsigned short answer_count;
    unsigned short authority_count;
    unsigned short additional_count;
    query_t *queries;
    resource_record_t *answers;
    resource_record_t *authorities;
    resource_record_t *additional;
} message_t;

enum RR_TYPE
{
    Header, Question, Answer, Authority, Additional
};

uv_buf_t *message2buf(message_t *message);

message_t *buf2message(const uv_buf_t *buf);

void printMessage(message_t *message);

/**
 * 使用一个dns消息中的id和queries部分作为一个特征向量标记一个请求
 * @param message dns消息
 * @return 该dns消息的特征向量
 */
string_t *message2feature_string(message_t *message);

void message_log(message_t *message);

void message_free(message_t *message);

#endif //POST_DNS_MESSAGE_H
