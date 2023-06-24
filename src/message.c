//
// Created by ricardo on 23-6-16.
// 解析和封装DNS报文
//
#include "message.h"
#include "stdlib.h"
#include "string.h"
#include "logging.h"

/**
 * 采用递归方式在buffer中查找NAME字段
 * @param bufPos 要跳转到的起始字节
 * @param Name 组成NAME的数组
 * @param namePos 组成NAME的数组的当前大小
 * @return int 跳转前的所在字节
 */
int createName(const char *buf, int bufPos, char[], int *namePos);

/**
 * 创建响应数据字段
 * @param buf 收到的数据
 * @param bufPos 当前处理到的位置
 * @param RData 响应数据
 * @param RDataPos 当前处理到的位置
 * @param type 响应数据类型
 * @param length 响应数据的长度
 * @return 处理之后的缓冲区位置
 */
int createRData(const char *buf, int bufPos, char RData[], int *RDataPos, int type, int length);

/**
 * 解析消息头
 */
void buf2messageHeader(const uv_buf_t *buf, message_t *message);

/**
 * 解析消息查询部分
 * @param buf
 * @param message
 * @param endPos
 * @return
 */
void buf2messageQuestion(const char *buf, message_t *message, int *endPos);

/**
 * 解析消息恢复部分
 * @param buf
 * @param message
 * @param endPos
 * @param type
 * @return
 */
void buf2messageRR(const char *buf, message_t *message, const int *endPos, enum RR_TYPE type);

/**
 * 将传入的char中的每一位分离到str数组中
 * @param ch 要传入的char
 * @param bit 输出的八位bit数组指针
 */
void char2bit(char ch, char *bit);

/**
 *  将两个char拼接成一个short
 */
short char2Short(char high, char low);

uv_buf_t *message2buf(message_t *message)
{
    return NULL;
}

message_t *buf2message(const uv_buf_t *buf)
{
    // 统一使用大端
    // 报头

    message_t *message = malloc((sizeof(message_t)));
    // 指示解析完一个字段后，报文下一个到达的字节数
    int endPos = 0;

    buf2messageHeader(buf, message);
    // 因为dns消息的包头大小值固定的
    // 所以这里直接置为12
    endPos = 12;

    // Question段
    if (buf->base[endPos] != 0)
    {
        buf2messageQuestion(&buf->base[endPos], message, &endPos);
    }
    // Answer段
    if (buf->base[endPos] != 0)
    {
        buf2messageRR(buf->base, message, &endPos, Answer);
    }

    return message;
}


void buf2messageHeader(const uv_buf_t *buf, message_t *message)
{
    message->id = char2Short(buf->base[0], buf->base[1]);
    char *flags = (char *) malloc(16 * sizeof(char));
    char2bit(buf->base[2], &flags[0]);
    char2bit(buf->base[3], &flags[8]);

    message->flags.QR = flags[0];
    message->flags.Opcode = (flags[1] << 3) + (flags[2] << 2) + (flags[3] << 1) + flags[4];
    message->flags.AA = flags[5];
    message->flags.TC = flags[6];
    message->flags.RD = flags[7];
    message->flags.RA = flags[8];
    message->flags.Z = (flags[9] << 2) + (flags[10] << 1) + flags[11];
    message->flags.RCODE = (flags[12] << 3) + (flags[13] << 2) + (flags[14] << 1) + flags[15];

    message->query_count = (unsigned short) (((0 | buf->base[4]) << 8) | (buf->base[5] & 0xFF));
    message->answer_count = (unsigned short) (((0 | buf->base[6]) << 8) | (buf->base[7] & 0xFF));
    message->nameserver_count = (unsigned short) (((0 | buf->base[8]) << 8) | (buf->base[9] & 0xFF));
    message->additional_count = (unsigned short) (((0 | buf->base[10]) << 8) | (buf->base[11] & 0xFF));

    free(flags);
}

void buf2messageQuestion(const char *buf, message_t *message, int *endPos)
{
    int bufPos = 0;

    message->queries = (query_t *) malloc(message->query_count * sizeof(query_t));

    for (int entryNum = 0; entryNum < message->query_count; entryNum++)
    {
        // QNAME
        int QValuePos = 0;
        char QValue[512] = {};

        while (buf[bufPos] != 0)
        {
            int letterLength = (unsigned char) buf[bufPos];

            bufPos++;

            for (int letterNum = 0; letterNum < letterLength; letterNum++)
            {
                QValue[QValuePos + letterNum] = buf[bufPos];
                bufPos++;
            }
            QValue[QValuePos + letterLength] = '.';
            QValuePos += letterLength + 1;
        }
        
        QValue[QValuePos - 1] = '\0';
        char QResult[QValuePos];
        message->queries[entryNum].name = string_t_malloc(strcpy(QResult, QValue), QValuePos - 1);
        // 注意这个length不包括末尾的\0
        bufPos++;

        // QTYPE
        message->queries[entryNum].type = char2Short(buf[bufPos], buf[bufPos + 1]);
        bufPos += 2;

        // QCLASS
        message->queries[entryNum].class = char2Short(buf[bufPos], buf[bufPos + 1]);
        bufPos += 2;
    }

    // 更新末尾指针
    *endPos += bufPos;
}

void buf2messageRR(const char *buf, message_t *message, const int *endPos, enum RR_TYPE type)
{

    int bufPos = *endPos;
    resource_record_t *resourceRecord;
    unsigned short resourceRecordCount = 0;

    switch (type)
    {
        case Answer:
            resourceRecordCount = message->answer_count;
            resourceRecord = (resource_record_t *) malloc(resourceRecordCount * sizeof(resource_record_t));
            message->answers = resourceRecord;
            break;
        case Authority:
            resourceRecordCount = message->nameserver_count;
            resourceRecord = (resource_record_t *) malloc(resourceRecordCount * sizeof(resource_record_t));
            message->authorities = resourceRecord;
            break;
        case Additional:
            resourceRecordCount = message->additional_count;
            resourceRecord = (resource_record_t *) malloc(resourceRecordCount * sizeof(resource_record_t));
            message->additional = resourceRecord;
            break;
        default:;
    }

    for (int entryNum = 0; entryNum < resourceRecordCount; entryNum++)
    {
        // NAME
        char Name[512] = {};
        int namePos = 0;

        bufPos = createName(buf, bufPos, Name, &namePos);

        Name[namePos - 1] = '\0';
        char result[namePos];
        resourceRecord[entryNum].name = string_t_malloc(strcpy(result, Name), namePos - 1);
        // 注意这个length不包括末尾的\0
        bufPos++;

        //TYPE
        resourceRecord[entryNum].type = char2Short(buf[bufPos], buf[bufPos + 1]);
        bufPos += 2;

        //CLASS
        resourceRecord[entryNum].class = char2Short(buf[bufPos], buf[bufPos + 1]);
        bufPos += 2;

        //TTL
        resourceRecord[entryNum].ttl = ((0 | char2Short(buf[bufPos], buf[bufPos + 1])) << 16) |
                                       (char2Short(buf[bufPos + 2], buf[bufPos + 3]) & 0xFFFF);
        // 这里把两个short合成一个int
        bufPos += 4;

        //RDLENGTH
        resourceRecord[entryNum].response_data_length = char2Short(buf[bufPos], buf[bufPos + 1]);
        bufPos += 2;

        //RDATA
        char RData[512] = {};
        int RDataPos = 0;

        bufPos = createRData(buf, bufPos, RData, &RDataPos, resourceRecord[entryNum].type,
                             resourceRecord[entryNum].response_data_length);
        bufPos++;

        //只有对A、CNAME、AAAA有处理，其余类型都直接跳过
        switch (resourceRecord[entryNum].type)
        {
            case 1:
            case 5:
            case 28:
                RData[RDataPos - 1] = '\0';
                //char RDataResult[RDataPos];
                string_t *RDataResult = (string_t *) malloc(sizeof(string_t));

                RDataResult->value = (char *) malloc(sizeof(char) * RDataPos);
                RDataResult->length = RDataPos - 1;
                // 注意这个length不包括末尾的\0
                memcpy(RDataResult->value, RData, RDataPos);
                resourceRecord[entryNum].response_data = RDataResult;
                break;
        }
    }
}

int createRDataAddress(const char *buf, int bufPos, char RData[], int *RDataPos, int length)
{
    int currentLength = 0;
    while (currentLength < length)
    {
        int letterLength = length;

        for (int letterNum = 0; letterNum < letterLength; letterNum++)
        {
            RData[*RDataPos + letterNum] = buf[bufPos];
            bufPos++;
            currentLength++;
        }

        *RDataPos += letterLength + 1;
    }
    return bufPos - 1;
    // 要减一否则这里的bufPos会因为上面的for多1
}

int createName(const char *buf, int bufPos, char Name[], int *namePos)
{
    while (buf[bufPos] != 0)
    {
        if ((buf[bufPos] & 0xC0) == 0xC0)
        {
            // 检测到报文跳转指针

            char high = (0 | buf[bufPos] & 0x3F) << 8;
            bufPos++;

            // 指向要跳转的位置
            unsigned short offset = char2Short(high, buf[bufPos]);
            //createName(buf,(unsigned char) buf[bufPos],Name,namePos);
            createName(buf, offset, Name, namePos);
            return bufPos;
        }

        int letterLength = (unsigned char) buf[bufPos];

        bufPos++;

        for (int letterNum = 0; letterNum < letterLength; letterNum++)
        {
            Name[*namePos + letterNum] = buf[bufPos];
            bufPos++;
        }
        Name[*namePos + letterLength] = '.';
        *namePos += letterLength + 1;
    }
    return bufPos; // 这里不需要减1，因为这里的bufPos自然停在最后的0x00
}

int createRData(const char *buf, int bufPos, char RData[], int *RDataPos, int type, int length)
{

    switch (type)
    {
        case 1:
            // A
            return createRDataAddress(buf, bufPos, RData, RDataPos, length);
        case 5:
            // CNAME
            return createName(buf, bufPos, RData, RDataPos);
        case 28:
            // AAAA
            return createRDataAddress(buf, bufPos, RData, RDataPos, length);
        default:
            // 其他类型直接返回到达这个RData末尾的下标
            return bufPos + length - 1;
    }
}

void char2bit(char ch, char *bit)
{

    for (int j = 0; j < 8; j++)
    {
        /*
         * 化成二进制时，从右数是第几位就向右边移动几位（注意：下标从零开始）然后在与上1
         * 7-j是因为按大端存储
        */
        bit[7 - j] = (ch >> j) & 1;
    }
}

short char2Short(char high, char low)
{
    return ((0 | high) << 8) | (low & 0xFF);
}

/**
 * 打印DNS message报文内容
 * @param message 要打印的报文
 */
void printMessage(message_t *message)
{

    log_debug("------DNS Message------\n");
    log_debug("id: %x\n", (unsigned short) message->id);

    log_debug("[Flags]\n");

    log_debug("QR: %c  ", message->flags.QR + 48);
    log_debug(message->flags.QR == 0 ? "Query\n" : "Response\n");

    log_debug("Opcode: %c  ", message->flags.Opcode + 48);
    switch (message->flags.Opcode)
    {
        case 0:
            log_debug("Standard\n");
            break;
        case 1:
            log_debug("Inverse\n");
            break;
        case 2:
            log_debug("Server status request\n");
            break;
        default:
            log_debug("Unknown\n");
    }

    log_debug("AA: %c  ", message->flags.AA + 48);
    if (message->flags.QR == 0)
    {
        log_debug("Useless in query\n");
    }
    else
    {
        log_debug(message->flags.AA == 0 ? "Server is not an authority\n" : "Server is an authority\n");
    }

    log_debug("TC: %c  ", message->flags.TC + 48);
    log_debug(message->flags.TC == 0 ? "Message is not truncated\n" : "Message is truncated\n");

    log_debug("RD: %c  ", message->flags.RD + 48);
    log_debug(message->flags.RD == 0 ? "Not recursion Desired\n" : "Recursion Desired\n");

    log_debug("RA: %c  ", message->flags.RA + 48);
    if (message->flags.QR == 0)
    {
        log_debug("Useless in query\n");
    }
    else
    {
        log_debug(message->flags.RA == 0 ? "Server is not support for recursive query\n"
                                      : "Server is support for recursive query\n");
    }

    log_debug("Z: %c  \n", message->flags.Z + 48);

    log_debug("RCODE: %c  ", message->flags.RCODE + 48);
    if (message->flags.QR == 0)
    {
        log_debug("Useless in query\n");
    }
    else
    {
        switch (message->flags.RCODE)
        {
            case 0:
                log_debug("No error condition\n");
                break;
            case 1:
                log_debug("Format error\n");
                break;
            case 2:
                log_debug("Server failure\n");
                break;
            case 3:
                log_debug("Name Error\n");
                break;
            case 4:
                log_debug("Not Implemented\n");
                break;
            case 5:
                log_debug("Refused\n");
                break;
            default:
                log_debug("Unknown error\n");
        }
    }

    log_debug("QDCOUNT: %hu\n", message->query_count);
    log_debug("ANCOUNT: %hu\n", message->answer_count);
    log_debug("NSCOUNT: %hu\n", message->nameserver_count);
    log_debug("ARCOUNT: %hu\n", message->additional_count);

    log_debug("[QUESTION]\n");
    for (int queryNum = 0; queryNum < message->query_count; queryNum++)
    {
        log_debug("[Query %d]\n", queryNum);

        log_debug("QNAME: ");
        for (int labelNum = 0; labelNum < (sizeof *(message->queries[queryNum].name)) / (sizeof(string_t)); labelNum++)
        {
            for (int letterNum = 0; letterNum < message->queries[queryNum].name[labelNum].length; letterNum++)
            {
                log_debug("%c", message->queries[queryNum].name[labelNum].value[letterNum]);
            }
            log_debug(labelNum == (sizeof *(message->queries[queryNum].name)) / (sizeof(string_t)) - 1 ? "" : ".");
        }
        log_debug("\n");
    }

    log_debug("QTYPE: %hu\n", message->queries->type);
    log_debug("QCLASS: %hu\n", message->queries->class);

    if (message->answer_count > 0)
    {
        log_debug("[ANSWER]\n");
        for (int responseNum = 0; responseNum < message->answer_count; responseNum++)
        {
            log_debug("[RESPONSE %d]\n", responseNum);

            log_debug("NAME: ");
            for (int labelNum = 0;
                 labelNum < (sizeof *(message->answers[responseNum].name)) / (sizeof(string_t)); labelNum++)
            {
                for (int letterNum = 0; letterNum < message->answers[responseNum].name[labelNum].length; letterNum++)
                {
                    log_debug("%c", message->answers[responseNum].name[labelNum].value[letterNum]);
                }
                log_debug(labelNum == (sizeof *(message->answers[responseNum].name)) / (sizeof(string_t)) - 1 ? "" : ".");
            }
            log_debug("\n");

            log_debug("TYPE: %hu\n", message->answers[responseNum].type);
            log_debug("CLASS: %hu\n", message->answers[responseNum].class);
            log_debug("TTL: %d\n", message->answers[responseNum].ttl);
            log_debug("RDLENGTH: %hu\n", message->answers[responseNum].response_data_length);

            log_debug("RData: ");
            switch (message->answers[responseNum].type)
            {
                case 1:
                    // A
                    for (int labelNum = 0; labelNum < (sizeof *(message->answers[responseNum].response_data)) /
                                                      (sizeof(string_t)); labelNum++)
                    {
                        for (int letterNum = 0;
                             letterNum < message->answers[responseNum].response_data[labelNum].length; letterNum++)
                        {
                            log_debug("%d",
                                   (unsigned char) message->answers[responseNum].response_data[labelNum].value[letterNum]);
                            log_debug(letterNum == message->answers[responseNum].response_data[labelNum].length - 1 ? ""
                                                                                                                 : ".");
                        }
                    }
                    break;
                case 5:
                    // CNAME
                    for (int labelNum = 0; labelNum < (sizeof *(message->answers[responseNum].response_data)) /
                                                      (sizeof(string_t)); labelNum++)
                    {
                        for (int letterNum = 0;
                             letterNum < message->answers[responseNum].response_data[labelNum].length; letterNum++)
                        {
                            log_debug("%c", message->answers[responseNum].response_data[labelNum].value[letterNum]);
                        }
                        log_debug(labelNum ==
                               (sizeof *(message->answers[responseNum].response_data)) / (sizeof(string_t)) - 1 ? ""
                                                                                                                : ".");
                    }
                    break;
                case 28:
                    // AAAA
                    for (int letterNum = 0;
                         letterNum < message->answers[responseNum].response_data->length; letterNum++)
                    {
                        log_debug("%02x", (unsigned char) message->answers[responseNum].response_data->value[letterNum]);
                        if (letterNum % 2 != 0)
                        {
                            log_debug((letterNum == message->answers[responseNum].response_data->length - 1) ? "" : ":");
                        }

                    }
                    break;
                default:
                    log_debug("Not analyzed");
            }

            log_debug("\n");
        }
    }
}