//
// Created by ricardo on 23-6-6.
//

#ifndef POST_DNS_NAME_SPACE_DEFINITIONS_H
#define POST_DNS_NAME_SPACE_DEFINITIONS_H

/**
 * dns type字段的定义枚举
 */
typedef enum {
    A = 1,
    NS,
    MD,
    MF,
    CNAME,
    SOA,
    MB,
    MG,
    MR,
    NULL,
    WKS,
    PTR,
    HINFO,
    MINFO,
    MX,
    TXT
} dns_type;

/**
 * dns class字段的定义枚举
 */
typedef enum {
    IN = 1,
    CS,
    CH,
    HS
} dns_class;



#endif //POST_DNS_NAME_SPACE_DEFINITIONS_H
