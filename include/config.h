//
// Created by ricardo on 23-6-26.
//

#ifndef POST_DNS_CONFIG_H
#define POST_DNS_CONFIG_H
#include "logging.h"

typedef struct {
    char *upstream_name;
    char *ipv4_config_file;
    char *ipv6_config_file;
    char *cname_config_file;
    logging_level_t logging_level;
} dns_config_t;

extern dns_config_t dns_config;

dns_config_t config_init(int argc, char **argv);

void config_help_print();

#endif //POST_DNS_CONFIG_H
