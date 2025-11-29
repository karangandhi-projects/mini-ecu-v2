/**
 * @file    cli_if.h
 * @brief   UART-based command line interface for Mini ECU.
 *
 * This CLI provides:
 *   - Interrupt-driven RX into a ring buffer.
 *   - A simple line-based command parser.
 *   - A live "dashboard" line showing speed/RPM/temp.
 */

#ifndef CLI_IF_H
#define CLI_IF_H

#include "main.h"
#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the CLI interface.
 *
 * @param[in] huart   UART handle used for CLI I/O (e.g., &huart2).
 * @param[in] vehicle Pointer to the main VehicleState_t instance
 *                    whose values will be shown on the live dashboard.
 */
void CLI_IF_Init(UART_HandleTypeDef *huart, VehicleState_t *vehicle);

/**
 * @brief Periodic CLI processing; to be called from a dedicated RTOS task.
 *
 * Responsibilities:
 *   - Drain RX ring buffer and feed characters into the line parser.
 *   - Periodically refresh the live "speedometer" line.
 *
 * This function is non-blocking and should be called regularly, e.g.
 * every 10 ms from a task that also calls osDelay(10).
 */
void CLI_IF_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* CLI_IF_H */
