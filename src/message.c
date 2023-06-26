//
// Created by ricardo on 23-6-16.
// 解析和封装DNS报文
//
#include "message.h"
#include "stdlib.h"
#include "string.h"
#include "logging.h"
#include "utils.h"

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
 * 封装报头
 * @param buf 生成的buffer
 * @param message 封装信息
 * @param endPos 这个函数执行完毕后，buf的长度
 */
void message2bufHeader(char *buf, message_t *message, int *endPos);

/**
 * 封装报文Question段
 * @param buf 生成的buffer
 * @param message 封装信息
 * @param endPos 这个函数执行完毕后，buf的长度
 */
void message2bufQuestion(char *buf, message_t *message, int *endPos);

/**
 * 封装报文Answer段
 * @param buf 生成的buffer
 * @param message 封装信息
 * @param endPos 这个函数执行完毕后，buf的长度
 */
void message2bufAnswer(char *buf, message_t *message, int *endPos);


/**
 * 将传入的char中的每一位分离到str数组中
 * @param ch 要传入的char
 * @param bit 输出的八位bit数组指针
 */
void char2bit(char ch, char *bit);

/**
 * 将message中域名部分转换为DNS报文中的域名存储方式(length+mainPart+\0)
 * @param domain_name message中域名部分
 * @warning 使用后记得free掉这个返回值
 * @return 一个DNS报文中的域名字符串
 */
char *name2message(string_t *domain_name);


uv_buf_t *message2buf(message_t *message)
{
    uv_buf_t *buf = (uv_buf_t *) malloc(sizeof(uv_buf_t));
    buf->base = (char *) malloc(1500 * sizeof(char));
    int endPos = 0;

    message2bufHeader(&buf->base[endPos], message, &endPos);
    // 此时endPos = 12;

    message2bufQuestion(&buf->base[endPos], message, &endPos);

    if (message->answer_count > 0)
    {
        message2bufAnswer(&buf->base[endPos], message, &endPos);
    }
    buf->len = endPos;
    return buf;
}

message_t *buf2message(const uv_buf_t *buf)
{
    // 统一使用大端
    // 报头

    message_t *message = (message_t *) malloc(sizeof(message_t));
    // 指示解析完一个字段后，报文下一个到达的字节数
    int endPos = 0;

    buf2messageHeader(buf, message);
    // 因为dns消息的包头大小值固定的
    // 所以这里直接置为12
    endPos = 12;

    // Question段
    if (endPos < buf->len)
    {
        buf2messageQuestion(&buf->base[endPos], message, &endPos);
    }
    // Answer段
    if (message->answer_count > 0)
    {
        buf2messageRR(buf->base, message, &endPos, Answer);
    }

    return message;
}

void message_log(message_t *message)
{
    log_debug("打印DNS包中的信息：");

    for (int i = 0; i < message->query_count; i++)
    {
        char *query_domain = string_print(message->queries[i].name);
        log_debug("DNS查询域名：%s", query_domain);
        free(query_domain);
    }

    for (int i = 0; i < message->answer_count; i++)
    {
        switch (message->answers[i].type)
        {
            case 1:
            {
                // A
                char *domain_name = string_print(message->answers[i].name);
                char *ipv4_address = string_print(
                        inet4address2string(*(int *) message->answers[i].response_data)
                );
                log_debug("域名%s A记录回答：%s", domain_name, ipv4_address);
                free(domain_name);
                free(ipv4_address);
                break;
            }
            case 5:
            {
                // CNAME
                char *domain_name = string_print(message->answers[i].name);
                char *answer_name = string_print((string_t *) message->answers[i].response_data);
                log_debug("域名%s CNAME记录回答%s", domain_name, answer_name);
                free(domain_name);
                free(answer_name);
                break;
            }
            case 28:
            {
                // AAAA
                char *domain_name = string_print(message->answers[i].name);
                char *ipv6_address = string_print(
                        inet6address2string((const unsigned char *) message->answers[i].response_data)
                );
                log_debug("域名%s A记录回答：%s", domain_name, ipv6_address);
                free(domain_name);
                free(ipv6_address);
                break;
            }
            default:
                break;
        }
    }
}

void message_free(message_t *message)
{
    message_log(message);
    for (int i = 0; i < message->query_count; i++)
    {
        string_free(message->queries[i].name);
    }
    if (message->query_count > 0)
    {
        free(message->queries);
    }

    for (int i = 0; i < message->answer_count; i++)
    {
        switch (message->answers[i].type)
        {
            case 1:
            case 28:
                // A 和 AAAA
                free(message->answers[i].response_data);
                string_free(message->answers[i].name);
                break;
            case 5:
                // CNAME
                string_free(message->answers[i].response_data);
                string_free(message->answers[i].name);
                break;
            default:
                break;
        }
    }

    if (message->answer_count > 0)
    {
        free(message->answers);
    }

    free(message);
}


void buf2messageHeader(const uv_buf_t *buf, message_t *message)
{
    message->id = char2Short(buf->base[0], buf->base[1]);
    char *flags = (char *) malloc(16 * sizeof(char));
    char2bit(buf->base[2], &flags[0]);
    char2bit( buf->base[3], &flags[8]);

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
    message->authority_count = (unsigned short) (((0 | buf->base[8]) << 8) | (buf->base[9] & 0xFF));
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
        message->queries[entryNum].name = string_malloc(QValue, QValuePos - 1);
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
            resourceRecordCount = message->authority_count;
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
        resourceRecord[entryNum].name = string_malloc(strcpy(result, Name), namePos - 1);
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

        string_t *RDataResult = (string_t *) malloc(sizeof(string_t));

        //只有对A、CNAME、AAAA有处理，其余类型都直接跳过
        switch (resourceRecord[entryNum].type)
        {
            case 1:
                // A
                RData[RDataPos - 1] = '\0';

                RDataResult->value = (char *) malloc(sizeof(char) * (RDataPos - 1));
                // 去除最后的\0，实际上只有四个字节
                RDataResult->length = RDataPos - 1;
                memcpy(RDataResult->value, RData, RDataPos - 1);

                unsigned int *RDataResultInt = (unsigned int *) malloc(sizeof(unsigned int));
                *RDataResultInt = string2inet4address(RDataResult);
                resourceRecord[entryNum].response_data = (unsigned int *) RDataResultInt;
                break;
            case 5:
                // CNAME
                RData[RDataPos - 1] = '\0';

                RDataResult->value = (char *) malloc(sizeof(char) * RDataPos);
                RDataResult->length = RDataPos - 1;
                // 这个字符串复制后末尾是不要带\0的，所以实际的复制size就是RDataPos - 1

                memcpy(RDataResult->value, RData, RDataPos);
                // 这个字符串复制后末尾是不要带\0的，所以实际的复制size就是RDataPos - 1 当我没说，，
                resourceRecord[entryNum].response_data = RDataResult;
                break;
            case 28:
                // AAAA
                RData[RDataPos - 1] = '\0';

                RDataResult->value = (char *) malloc(sizeof(char) * (RDataPos - 1));
                RDataResult->length = RDataPos - 1;
                // 注意这个length不包括末尾的\0,为16
                memcpy(RDataResult->value, RData, RDataPos - 1);

                resourceRecord[entryNum].response_data = string2inet6address(RDataResult);
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

            char high = (0 | buf[bufPos] & 0x3F);
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

/*short char2Short(char high, char low)
{
    return ((0 | high) << 8) | (low & 0xFF);
}*/

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
    log_debug("NSCOUNT: %hu\n", message->authority_count);
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
                log_debug(
                        labelNum == (sizeof *(message->answers[responseNum].name)) / (sizeof(string_t)) - 1 ? "" : ".");
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
                    /*for (int labelNum = 0; labelNum < (sizeof *(message->answers[responseNum].response_data)) /
                                                      (sizeof(string_t)); labelNum++)
                    {*/
                    /*for (int letterNum = 0;
                         letterNum < message->answers[responseNum].response_data.length; letterNum++)
                    {
                        log_debug("%d",
                               (unsigned char) message->answers[responseNum].response_data.value[letterNum]);
                        log_debug(letterNum == message->answers[responseNum].response_data.length - 1 ? ""
                                                                                                             : ".");
                    }*/
                    //}
                    break;
                case 5:
                    // CNAME
                    /*for (int labelNum = 0; labelNum < (sizeof *(message->answers[responseNum].response_data)) /
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
                    }*/
                    break;
                case 28:
                    // AAAA
                    /*for (int letterNum = 0;
                         letterNum < message->answers[responseNum].response_data->length; letterNum++)
                    {
                        log_debug("%02x", (unsigned char) message->answers[responseNum].response_data->value[letterNum]);
                        if (letterNum % 2 != 0)
                        {
                            log_debug((letterNum == message->answers[responseNum].response_data->length - 1) ? "" : ":");
                        }

                    }*/
                    break;
                default:
                    log_debug("Not analyzed");
            }

            log_debug("\n");
        }
    }
}

/**
 * 十进制转二进制
 * @param dec 十进制数
 * @param bin 存放二进制各个位的数组
 * @param length 二进制数组的长度
 */
void dec2bin(int dec, int *bin, int length)
{
    int pos = 0;
    while (dec > 0)
    {
        bin[pos] = dec % 2;
        dec = dec / 2;
        pos++;
    }

    // 反转数组

    int temp = 0;
    int n = length;
    for (int i = 0; i < n / 2; ++i)
    {
        temp = bin[n - i - 1];
        bin[n - i - 1] = bin[i];
        bin[i] = temp;
    }
}


string_t *message2feature_string(message_t *message)
{
    string_t *result = malloc(sizeof(string_t));
    // id的长度
    result->length = 2;

    for (int i = 0; i < message->query_count; i++)
    {
        // 加上的4是TYPE和CLASS的4个字节
        result->length = result->length + message->queries[i].name->length + 4;
    }

    result->value = malloc(sizeof(char) * result->length);

    int pos = 0;
    for (int i = 0; i < message->query_count; i++)
    {
        memcpy(&result->value[pos], message->queries[i].name->value,
               sizeof(char) * message->queries[i].name->length);
        pos = pos + sizeof(char) * message->queries[i].name->length;
        memcpy(&result->value[pos], &message->queries[i].class, 2);
        pos = pos + 2;
        memcpy(&result->value[pos], &message->queries[i].type, 2);
        pos = pos + 2;
    }
    memcpy(&result->value[pos], &message->id, 2);
    return result;
}

char *name2message(string_t *domain_name)
{
    char *output = (char *) malloc(domain_name->length * 2 + 2);
    char *current = output;
    split_array_t *token = string_split(domain_name,'.');
    for(int i = 0;i < token->length; i++) {
        char *token_print = string_print(token->array[i]);
        sprintf(current, "%c%s", token->array[i]->length, token_print);
        current = current + token->array[i]->length + 1;
        free(token_print);
    }

    *current = 0;
    *(current + 1) = '\0';

    for (int i = 0; i < token->length; i++)
    {
        string_free(token->array[i]);
    }
    free(token);

    return output;
}


void message2bufHeader(char *buf, message_t *message, int *endPos)
{

    int bufPos = 0;

    // id
    memcpy(&buf[bufPos], &(message->id), sizeof(short));
    swap16(&buf[bufPos]);
    // flags
    bufPos += 2;

    char flags[2] = {0, 0};
    // QR Opcode AA TC RD
    int flagsOpcode[4] = {0};
    dec2bin(message->flags.Opcode, flagsOpcode, 4);

    int flagsBitA[8] = {message->flags.QR, flagsOpcode[0],
                        flagsOpcode[1], flagsOpcode[2],
                        flagsOpcode[3], message->flags.AA,
                        message->flags.TC, message->flags.RD};
    // RA Z RCODE
    int flagsRCode[4] = {0};
    dec2bin(message->flags.RCODE, flagsRCode, 4);
    int flagsBitB[8] = {message->flags.RA, message->flags.Z,
                        message->flags.Z, message->flags.Z,
                        flagsRCode[0], flagsRCode[1],
                        flagsRCode[2], flagsRCode[3]};

    for (int i = 0; i < 8; i++)
    {
        flags[0] |= (flagsBitA[7 - i] << i);
        flags[1] |= (flagsBitB[7 - i] << i);
    }

    memcpy(&buf[bufPos], flags, 2 * sizeof(char));

    // QDCOUNT
    bufPos += 2;
    memcpy(&buf[bufPos], &(message->query_count), sizeof(short));
    swap16(&buf[bufPos]);
    // ANCOUNT
    bufPos += 2;
    memcpy(&buf[bufPos], &(message->answer_count), sizeof(short));
    swap16(&buf[bufPos]);
    // NSCOUNT
    bufPos += 2;
    memcpy(&buf[bufPos], &(message->authority_count), sizeof(short));
    swap16(&buf[bufPos]);
    // ARCOUNT
    bufPos += 2;
    memcpy(&buf[bufPos], &(message->additional_count), sizeof(short));
    swap16(&buf[bufPos]);

    *endPos = bufPos + 2;
}


void message2bufQuestion(char *buf, message_t *message, int *endPos)
{

    int bufPos = 0;
    for (int entryNum = 0; entryNum < message->query_count; entryNum++)
    {
        // QNAME
        char QNAME[message->queries[entryNum].name->length + 2];
        char *transfer = name2message(message->queries[entryNum].name);
        memcpy(QNAME, transfer, message->queries[entryNum].name->length + 2);
        free(transfer);

        memcpy(&buf[bufPos], QNAME, message->queries[entryNum].name->length + 2);



        // QTYPE
        bufPos += message->queries[entryNum].name->length + 2;
        memcpy(&buf[bufPos], &(message->queries[entryNum].type), sizeof(short));
        swap16(&buf[bufPos]);

        // QCLASS
        bufPos += 2;
        memcpy(&buf[bufPos], &(message->queries[entryNum].class), sizeof(short));
        swap16(&buf[bufPos]);

        *endPos += bufPos + 2;
    }
}


void message2bufAnswer(char *buf, message_t *message, int *endPos)
{

    int bufPos = 0;

    for (int entryNum = 0; entryNum < message->answer_count; entryNum++)
    {
        // NAME
        char NAME[message->answers[entryNum].name->length + 2];
        char *transfer = name2message(message->answers[entryNum].name);
        memcpy(NAME, transfer, message->answers[entryNum].name->length + 2);
        free(transfer);

        memcpy(&buf[bufPos], NAME, message->answers[entryNum].name->length + 2);



        // TYPE
        bufPos += message->answers[entryNum].name->length + 2;
        memcpy(&buf[bufPos], &(message->answers[entryNum].type), sizeof(short));
        swap16(&buf[bufPos]);

        // CLASS
        bufPos += 2;
        memcpy(&buf[bufPos], &(message->answers[entryNum].class), sizeof(short));
        swap16(&buf[bufPos]);

        // TTL
        bufPos += 2;
        memcpy(&buf[bufPos], &(message->answers[entryNum].ttl), sizeof(int));
        swap32(&buf[bufPos]);

        // RDLENGTH
        switch (message->answers[entryNum].type)
        {
            case 1:
                // A
                bufPos += 4;
                memcpy(&buf[bufPos], &(message->answers[entryNum].response_data_length), sizeof(short));
                swap16(&buf[bufPos]);
                break;
            case 5:
                // CNAME
                bufPos += 4;
                unsigned short domain_length = ((string_t *) message->answers[entryNum].response_data)->length + 2;
                memcpy(&buf[bufPos], &domain_length, sizeof(short));
                swap16(&buf[bufPos]);
                break;
            case 28:
                // AAAA
                bufPos += 4;
                memcpy(&buf[bufPos], &(message->answers[entryNum].response_data_length), sizeof(short));
                swap16(&buf[bufPos]);
                break;
        }

        // RDATA
        bufPos += 2;
        switch (message->answers[entryNum].type)
        {
            case 1:
                // A
                memcpy(&buf[bufPos], message->answers[entryNum].response_data, sizeof(int));
                swap32(&buf[bufPos]);
                bufPos += message->answers[entryNum].response_data_length;
                break;
            case 5:
                // CNAME
            {
                string_t *data = (string_t *) message->answers[entryNum].response_data;
                char CNAME[data->length + 2];
                char *transferCNAME = name2message(data);
                memcpy(CNAME, transferCNAME, data->length + 2);
                free(transferCNAME);

                memcpy(&buf[bufPos], CNAME, data->length + 2);
                bufPos += data->length + 2;
                break;
            }
            case 28:
                // AAAA
                memcpy(&buf[bufPos], message->answers[entryNum].response_data, 16);
                bufPos += message->answers[entryNum].response_data_length;
                break;
        }


    }
    *endPos += bufPos;
    // 最后会停在bufPos的最后一位的下一位
}