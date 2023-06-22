//
// Created by ricardo on 23-6-22.
//

#ifndef POST_DNS_LOGGING_H
#define POST_DNS_LOGGING_H

typedef enum {
    logging_debug_level,
    logging_information_level,
    logging_warning_level,
    logging_error_level
} logging_level_t;

extern logging_level_t logging_level;

void logging_printf(logging_level_t level, const char *str, const char * filename, int line);

#define log_debug(str) logging_printf(logging_debug_level, str, __FILE_NAME__, __LINE__)
#define log_information(str) logging_printf(logging_information_level, str, __FILE_NAME__, __LINE__)
#define log_warning(str) logging_printf(logging_warning_level, str, __FILE_NAME__, __LINE__)
#define log_error(str) logging_printf(logging_error_level, str, __FILE_NAME__, __LINE__)

#endif //POST_DNS_LOGGING_H
