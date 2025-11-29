/**
 * @file    log.h
 * @brief   Lightweight logging framework for Mini ECU.
 *
 * Logs messages over a UART (typically USART2) with levels and module tags.
 * Format:
 *   [I][CLI] CLI initialized
 */

#ifndef LOG_H
#define LOG_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} log_level_t;

/**
 * @brief Initialize the logging system with a UART handle.
 *
 * @param[in] huart Pointer to the UART handle used for logging output.
 */
void Log_Init(UART_HandleTypeDef *huart);

/**
 * @brief Set the global log level. Messages below this level are dropped.
 */
void Log_SetLevel(log_level_t level);

/**
 * @brief Get the current global log level.
 */
log_level_t Log_GetLevel(void);

/**
 * @brief Core logging function (printf-style).
 *
 * Most code should use LOG_ERROR/LOG_WARN/LOG_INFO/LOG_DEBUG macros.
 *
 * @param[in] level  Severity.
 * @param[in] module Short module tag (e.g., "CLI", "CAN").
 * @param[in] fmt    printf-style format string.
 * @param[in] ...    Arguments.
 */
void Log_Write(log_level_t level, const char *module, const char *fmt, ...);

/* Convenience macros */
#define LOG_ERROR(mod, fmt, ...) \
    Log_Write(LOG_LEVEL_ERROR, (mod), (fmt), ##__VA_ARGS__)

#define LOG_WARN(mod, fmt, ...) \
    Log_Write(LOG_LEVEL_WARN,  (mod), (fmt), ##__VA_ARGS__)

#define LOG_INFO(mod, fmt, ...) \
    Log_Write(LOG_LEVEL_INFO,  (mod), (fmt), ##__VA_ARGS__)

#define LOG_DEBUG(mod, fmt, ...) \
    Log_Write(LOG_LEVEL_DEBUG, (mod), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
