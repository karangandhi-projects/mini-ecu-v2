/**
 * @file    log.c
 * @brief   Lightweight logging framework implementation.
 *
 * Uses blocking HAL_UART_Transmit() for now.
 */

#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef *s_logUart = NULL;
static log_level_t         s_logLevel = LOG_LEVEL_INFO;

void Log_Init(UART_HandleTypeDef *huart)
{
    s_logUart = huart;
}

void Log_SetLevel(log_level_t level)
{
    s_logLevel = level;
}

log_level_t Log_GetLevel(void)
{
    return s_logLevel;
}

void Log_Write(log_level_t level, const char *module, const char *fmt, ...)
{
    if (level > s_logLevel)
        return;

    if (s_logUart == NULL)
        return;

    char msgBuf[128];
    char outBuf[160];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    va_end(args);

    if (n < 0)
        return;

    if ((size_t)n >= sizeof(msgBuf))
    {
        n = (int)sizeof(msgBuf) - 1;
        msgBuf[n] = '\0';
    }

    char levelChar = 'I';
    switch (level)
    {
        case LOG_LEVEL_ERROR: levelChar = 'E'; break;
        case LOG_LEVEL_WARN:  levelChar = 'W'; break;
        case LOG_LEVEL_INFO:  levelChar = 'I'; break;
        case LOG_LEVEL_DEBUG: levelChar = 'D'; break;
        default:              levelChar = '?'; break;
    }

    if (module == NULL)
        module = "GEN";

    n = snprintf(outBuf, sizeof(outBuf), "[%c][%s] %s\r\n",
                 levelChar, module, msgBuf);
    if (n <= 0)
        return;

    size_t len = (size_t)n;
    if (len > sizeof(outBuf))
        len = sizeof(outBuf);

    (void)HAL_UART_Transmit(s_logUart, (uint8_t *)outBuf,
                            (uint16_t)len, 10U);
}
