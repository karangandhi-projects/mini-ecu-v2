# Architecture Overview

This document describes the high-level architecture of the **Mini ECU v2** project.

## Layers

1. **Application / Tasks**
   - `VehicleTask` (10 Hz): updates the virtual vehicle physics and sends CAN telemetry.
   - `CliTask` (~100 Hz): runs the UART CLI state machine and refreshes the live dashboard.
   - `CanRxTask`: waits on a CAN RX queue and processes/logs received frames.

2. **Modules**
   - `vehicle.c/.h`:
     - Owns the `VehicleState_t` struct (speed, RPM, coolant temperature).
     - Implements a simple physical model with coasting, engine inertia, and coolant dynamics.
   - `can_if.c/.h`:
     - Abstracts low-level CAN1 configuration (filter, mode, notifications).
     - Provides a telemetry encoding API and a CAN RX message queue.
   - `cli_if.c/.h`:
     - Provides a UART-based CLI with interrupt-driven RX via a ring buffer.
     - Implements commands for manipulating the vehicle state and CAN logging.
   - `log.c/.h`:
     - Central logging framework with levels and module tags, writing to USART2.

3. **HAL / BSP**
   - STM32Cube HAL drivers for GPIO, CAN, UART, clocks.
   - Board support for NUCLEO-F446RE (B1 button, LD2 LED, ST-LINK VCP).

## Data Flow

- `VehicleTask` updates `g_vehicle` and calls `CAN_IF_SendTelemetry(&g_vehicle)`.
- `CAN_IF_SendTelemetry` encodes the state into a CAN frame (StdID 0x100) and transmits.
- In loopback mode, the same node receives its own frames:
  - RX interrupt pushes frames into the CAN RX queue.
  - `CanRxTask` pops frames and calls `CAN_IF_ProcessRxMsg()`.
  - `CAN_IF_ProcessRxMsg()` logs them via `LOG_INFO("CAN", ...)`.
- `CliTask`:
  - Parses commands to adjust `g_vehicle` (e.g., `veh speed 60`).
  - Toggles CAN RX logging (`log on` / `log off`).
  - Updates the live dashboard line.

## RTOS Timing

- `VehicleTask`: 100 ms period (dt = 0.1 s).
- `CliTask`: ~10 ms polling period.
- `CanRxTask`: blocking on RX queue (no periodic wakeup).

This architecture intentionally mirrors a small, modular ECU firmware to serve as a base for future bootloader and OTA work.
