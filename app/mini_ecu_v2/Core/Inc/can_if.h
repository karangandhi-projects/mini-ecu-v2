/**
 * @file    can_if.h
 * @brief   CAN interface abstraction for the Mini ECU project.
 *
 * This module wraps the STM32 HAL CAN driver behind a small API that is
 * easy to test and reason about. It:
 *
 *   - Configures CAN1 (filter, notifications, start).
 *   - Owns an RX message queue consumed by CanRxTask.
 *   - Encodes VehicleState_t into a compact telemetry frame.
 *   - Provides optional logging of received frames over UART via Log_*.
 *
 * In this project CAN is configured in LOOPBACK mode, so every transmitted
 * frame is received back by the same node. This makes it ideal as a
 * self-contained demo or "virtual ECU" without extra hardware.
 */

#ifndef CAN_IF_H
#define CAN_IF_H

#include "main.h"
#include "cmsis_os2.h"
#include "vehicle.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple container for received CAN frames.
 *
 * This is the payload type stored in the RX message queue.
 * It mirrors the STM32 HAL RX structures but is decoupled so
 * the application layer does not depend on HAL types directly.
 */
typedef struct
{
    uint32_t StdId;   /**< 11-bit standard identifier (if IDE == 0). */
    uint32_t ExtId;   /**< 29-bit extended identifier (if IDE == 1). */
    uint8_t  IDE;     /**< Identifier type: 0 = standard, 1 = extended. */
    uint8_t  RTR;     /**< Frame type: 0 = data, 1 = remote. */
    uint8_t  DLC;     /**< Data length code (0..8). */
    uint8_t  Data[8]; /**< Payload bytes (unused bytes undefined). */
} CAN_IF_Msg_t;

/**
 * @brief Initialize the CAN interface module.
 *
 * Responsibilities:
 *   - Configure a permissive filter (accept all IDs into FIFO0).
 *   - Start CAN1 peripheral.
 *   - Enable RX and error-related interrupts.
 *   - Create the RX message queue for CanRxTask.
 *
 * @return HAL_OK on success, or a HAL error code if any HAL call fails.
 */
HAL_StatusTypeDef CAN_IF_Init(void);

/**
 * @brief Encode and transmit a telemetry frame based on vehicle state.
 *
 * Frame format (StdId = 0x100, DLC = 6):
 *   Byte 0-1 : speed in 0.1 km/h units (uint16, little-endian).
 *   Byte 2-3 : engine RPM (uint16, little-endian).
 *   Byte 4-5 : coolant temperature in 0.1 °C (int16, little-endian).
 *
 * @param[in] vs Pointer to the current "true" VehicleState_t.
 */
void CAN_IF_SendTelemetry(const VehicleState_t *vs);

/**
 * @brief Enable or disable CAN RX logging.
 *
 * When enabled, CAN_IF_ProcessRxMsg() emits a formatted line via
 * the logging framework for each received frame.
 *
 * @param[in] enable Non-zero to enable logging, zero to disable.
 */
void CAN_IF_SetLogging(uint8_t enable);

/**
 * @brief Get the handle of the RX message queue used by CanRxTask.
 *
 * @return osMessageQueueId_t of the RX queue, or NULL if init failed.
 */
osMessageQueueId_t CAN_IF_GetRxQueueHandle(void);

/**
 * @brief Process a received CAN message at thread level.
 *
 * Called by CanRxTask after popping a message from the queue.
 *
 * @param[in] msg Pointer to a valid CAN_IF_Msg_t instance.
 */
void CAN_IF_ProcessRxMsg(const CAN_IF_Msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* CAN_IF_H */
