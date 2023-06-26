//
// Created by ricardo on 23-6-26.
//
#include <stddef.h>
#include <stdlib.h>
#include "config.h"

dns_config_t dns_config;

dns_config_t config_init(int argc, char **argv)
{
    dns_config_t config;
    config.logging_level = logging_information_level;
    config.ipv4_config_file = NULL;
    config.ipv6_config_file = NULL;
    config.cname_config_file = NULL;
    config.upstream_name = "10.3.9.44";

    log_debug("读取命令行配置参数");
    for (int i = 1; i < argc; i++)
    {
        if (*argv[i] == '-')
        {
            switch (argv[i][1])
            {
                case 'h':
                    config_help_print();
                    exit(0);
                case 's':
                    config.upstream_name = argv[i + 1];
                    log_information("设置上游服务器: %s", config.upstream_name);
                    i++;
                    break;
                case '4':
                    config.ipv4_config_file = argv[i + 1];
                    log_information("读取ipv4配置文件: %s", config.ipv4_config_file);
                    i++;
                    break;
                case '6':
                    config.ipv6_config_file = argv[i + 1];
                    log_information("读取ipv6配置文件: %s", config.ipv6_config_file);
                    i++;
                    break;
                case 'c':
                    config.cname_config_file = argv[i + 1];
                    log_information("读取cname配置文件: %s", config.cname_config_file);
                    i++;
                    break;
                case 'l':
                {
                    logging_level_t level = *argv[i+1] - 48;
                    if (level > 3)
                    {
                        log_warning("错误的日志配置：%d", level);
                    }
                    else
                    {
                        logging_level = level;
                    }
                }
                default:
                    log_information("未知的配置选项: %s", argv[i]);
                    break;
            }
        }
        else
        {
            log_information("未知的配置选项: %s", argv[i]);
        }
    }

    return config;
}

void config_help_print()
{
    log_information("-h 打印帮助信息");
    log_information("-s [server_address] 设置上游服务器地址");
    log_information("-4 [file_name] ipv4配置文件");
    log_information("-6 [file_name] ipv6配置文件");
    log_information("-c [file_name] cname配置文件");
    log_information("-l [0/1/2/3] 设置日志等级 0-debug 1-info 2-warn 3-error");
}