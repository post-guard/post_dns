//
// Created by ricardo on 23-6-22.
//

#ifndef POST_DNS_LOGGING_H
#define POST_DNS_LOGGING_H

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

typedef enum {
    logging_debug_level,
    logging_information_level,
    logging_warning_level,
    logging_error_level
} logging_level_t;

extern logging_level_t logging_level;

void __attribute__((format(printf, 4, 5)))
logging_printf(logging_level_t level, const char * filename, int line, const char *fmt, ...);

char *bytes2hex(const char *data, int length);

#define log_debug(fmt, ...) logging_printf(logging_debug_level, __FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define log_information(fmt, ...) logging_printf(logging_information_level, __FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) logging_printf(logging_warning_level, __FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) logging_printf(logging_error_level, __FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)

#endif //POST_DNS_LOGGING_H
