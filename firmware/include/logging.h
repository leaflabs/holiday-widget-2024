#ifndef __LOGGING_H__
#define __LOGGING_H__
#include "uart_logger.h"

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2

#define LOG_LEVEL LOG_LEVEL_ERROR

#define LOG_ERR(fmt, ...) \
    (uart_logger_send("[ERROR]: " fmt "\r\n", ##__VA_ARGS__))

#if LOG_LEVEL == LOG_LEVEL_DEBUG || LOG_LEVEL == LOG_LEVEL_INFO
#define LOG_INF(fmt, ...) \
    (uart_logger_send("[INFO]: " fmt "\r\n", ##__VA_ARGS__))
#else
#define LOG_INF(fmt, ...) asm(" nop")
#endif

#if LOG_LEVEL == LOG_LEVEL_DEBUG
#define LOG_DBG(fmt, ...) \
    (uart_logger_send("[DEBUG]: " fmt "\r\n", ##__VA_ARGS__))
#else
#define LOG_DBG(fmt, ...) asm(" nop")
#endif

#endif /*__LOGGING_H__*/