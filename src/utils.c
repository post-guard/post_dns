//
// Created by ricardo on 23-6-24.
//
#include <stdio.h>
#include "utils.h"
#include "stdlib.h"

unsigned short char2Short(char high, char low)
{
    return ((0 | high) << 8) | (low & 0xFF);
}

string_t *inet4address2string(unsigned int address) {
    unsigned char divide[4]={};
    for (int i = 0; i < 4; i++) {
        divide[3 - i]=*((unsigned char *)&address+i);
        // 大端存储
    }

    int resultValuePos = 0;
    char * resultValue = (char *) malloc(20 * sizeof (char));

    for(int i = 0;i < 4;i++) {

        int num = divide[i];

        char numStr[4];
        snprintf(numStr, sizeof(numStr), "%d", num);

        int k = 0;
        while (numStr[k] != '\0') {
            resultValue[resultValuePos++] = numStr[k++];
        }
        if(i < 3) {
            resultValue[resultValuePos++] = '.';
        }
    }

    string_t *result = (string_t *) malloc(sizeof (string_t));
    result->value = resultValue;
    result->length = resultValuePos;

    return result;
}

unsigned int string2inet4address(string_t *address) {
    unsigned short high = ((0 | address->value[0]) << 8) | (address->value[1] & 0xFF);
    unsigned short low = ((0 | address->value[2]) << 8) | (address->value[3] & 0xFF);
    return ((0 | high) << 16) | (low & 0xFFFF);
}

string_t *inet6address2string(const unsigned char * address){

    int resultValuePos = 0;
    char * resultValue = (char *) malloc(60 * sizeof (char));

    for(int i = 0;i < 16;i++) {

        int num = address[i];

        char numStr[3];
        snprintf(numStr, sizeof(numStr), "%.2x", num);

        int k = 0;
        while (numStr[k] != '\0') {
            resultValue[resultValuePos++] = numStr[k++];
        }
        if(i < 15 && i % 2 ==1) {
            resultValue[resultValuePos++] = ':';
        }
    }

    string_t *result = (string_t *) malloc(sizeof (string_t));
    result->value = resultValue;
    result->length = resultValuePos;

    return result;
}

unsigned char * string2inet6address(string_t *address) {

    unsigned char * result = (unsigned char *)malloc(128 * sizeof(unsigned char));
    for(int i = 0 ; i < 128 ; i++) {
        result[i] = address->value[i];
    }
    return result;
}