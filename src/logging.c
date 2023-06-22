//
// Created by ricardo on 23-6-22.
//
#include "logging.h"
#include "stdio.h"

logging_level_t logging_level = logging_information_level;

void logging_printf(logging_level_t level, const char *str, const char *filename, int line)
{
    if (level >= logging_level)
    {
        const char * head;
        switch (level)
        {
            case logging_debug_level:
                head = "\033[34mDEBUG\033[0m";
                break;
            case logging_information_level:
                head = "\033[37mINFO\033[0m";
                break;
            case logging_warning_level:
                head = "\033[33mWARN\033[0m";
                break;
            case logging_error_level:
                head = "\033[31mERROR\033[0m";
                break;
        }

        printf("[%s] %s:%d: %s\n", head, filename, line, str);
    }
}



