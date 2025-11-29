# Mini ECU v2 – Architecture Overview

This document describes the high-level architecture of the **Mini ECU v2** project:
a virtual automotive ECU running on the **STM32F446RE NUCLEO** board, with a
custom bootloader and a FreeRTOS-based application.

---

## 1. System Overview

The system is split into two main firmware images:

1. **Bootloader (`mini_ecu_boot`)**
   - Lives in the first 32 KB of flash (`0x0800 0000 – 0x0800 7FFF`).
   - Initializes basic hardware (clocks, GPIO, UART).
   - Decides whether to:
     - Stay in bootloader mode (for future update/diagnostic features).
     - Jump to the main application at `0x0800 8000`.

2. **Application (`mini_ecu_v2`)**
   - Starts at `0x0800 8000`.
   - Uses **FreeRTOS** to run multiple tasks:
     - Vehicle model and state update.
     - CAN transmit/receive and processing.
     - UART CLI + live dashboard.
     - Logging via UART.

The two together simulate a small real-time ECU with a safe boot chain.

---

## 2. Hardware Architecture

**Target board:**
- STM32 NUCLEO-F446RE (STM32F446RE, Cortex-M4F @ 180 MHz, 512 KB Flash, 128 KB RAM).

**Peripherals used:**
- **USART2** (TX/RX):
  - Connected to the ST-LINK virtual COM port.
  - Used for logs, CLI commands, and the live dashboard.
- **CAN1**:
  - Configured in **loopback mode** (no physical bus required).
  - Used to simulate CAN telemetry (TX and RX on the same device).
- **GPIO:**
  - `B1` (User button):
    - In the **app**: acts as a virtual accelerator pedal.
    - In the **bootloader**: selects boot mode (stay in bootloader vs. jump to app).
  - `LD2` (User LED):
    - In the **bootloader**: indicates bootloader mode or error (blink patterns).
    - In the **app**: available for future status indication.

---

## 3. Firmware Layout & Memory Map

### 3.1 Flash

| Region         | Start Address | Size   | Used By          |
|----------------|---------------|--------|------------------|
| Bootloader     | 0x0800 0000   | 32 KB  | `mini_ecu_boot`  |
| Application    | 0x0800 8000   | 480 KB | `mini_ecu_v2`    |

- The application linker script is configured with:
  - `FLASH ORIGIN = 0x08008000`
  - `VECT_TAB_OFFSET = 0x00008000`

### 3.2 RAM

- Standard STM32F446RE SRAM (128 KB, 0x2000 0000 – 0x2001 FFFF).
- Used by both bootloader and application (never concurrently active).

---

## 4. Bootloader Architecture

**Key file:** `bootloader/mini_ecu_boot/Core/Src/main.c`

### Responsibilities

- Initialize HAL and a minimal clock setup (HSI-based).
- Configure GPIO (LD2, B1) and USART2.
- Print a boot banner and instructions over UART.
- Sample **B1**:
  - **Pressed at reset** → stay in bootloader (future update mode).
  - **Not pressed** → attempt to jump to application.
- Validate the application image:
  - Reads initial stack pointer and Reset_Handler from `APP_START_ADDR`.
  - Verifies stack pointer is within valid SRAM range.
- Perform a **clean jump**:
  - De-initialize HAL/RCC.
  - Disable SysTick and all NVIC interrupts.
  - Set `SCB->VTOR` to `APP_START_ADDR`.
  - Set MSP and jump to the application Reset_Handler.

If the app is invalid, the bootloader:
- Reports the issue over UART.
- Enters an LED-blink error loop.

### Planned Extensions

- UART and/or CAN-based firmware update protocol.
- Simple bootloader CLI commands (INFO, ERASE, WRITE, DONE).
- Firmware image validation via header + CRC.

---

## 5. Application Architecture

**Key modules:**

- `main.c` / `main.h`:
  - Initializes HAL, clocks, FreeRTOS, and middleware.
  - Creates the main RTOS tasks:
    - Vehicle task.
    - CAN RX task.
    - CLI task.
- `vehicle.c` / `vehicle.h`:
  - Implements a virtual vehicle model (speed, RPM, coolant temperature).
  - Uses B1 button as a **virtual accelerator**:
    - Button pressed → speed increases per tick.
    - Button released → speed decays.
  - Updates internal state periodically (e.g., every 100 ms).
- `can_if.c` / `can_if.h`:
  - CAN1 configuration (loopback mode).
  - CAN transmission of vehicle telemetry frames.
  - CAN reception with a FreeRTOS queue feeding the CAN RX task.
- `cli_if.c` / `cli_if.h`:
  - UART-based CLI interface.
  - Maintains a persistent **text dashboard** at the top of the terminal using
    ANSI escape codes and cursor control.
  - Parses CLI commands and prints logs / state.
- `log.c` / `log.h`:
  - Central logging helpers:
    - `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_DEBUG`.
  - Tags each log with a module name (e.g., "Vehicle", "CAN", "CLI").

### 5.1 Task-Level View

A simplified view of the main FreeRTOS tasks:

```text
+----------------------+    +----------------------+    +----------------------+
|   VehicleTask        |    |    CanRxTask         |    |    CliTask           |
+----------------------+    +----------------------+    +----------------------+
| - Reads B1 (throttle)|    | - Waits on CAN RX Q  |    | - Handles UART RX    |
| - Updates speed/RPM  |    | - Parses CAN frames  |    | - Updates dashboard  |
| - Updates coolant    |    | - Updates model/diag |    | - Executes commands  |
+----------------------+    +----------------------+    +----------------------+
           ^                           ^                          |
           |                           |                          |
           |                           +-----------+--------------+
           |                                       |
           +--------------------------- CAN1 (loopback) ---------+
```

All tasks share state via structured data (vehicle model) and appropriate
synchronization (e.g., queues).

---

## 6. Data Flows

### 6.1 Vehicle Data Flow

1. **VehicleTask** computes:
   - `speed`, `rpm`, `coolant_temp`.
2. CAN TX logic sends telemetry frames periodically over **CAN1** (loopback).
3. **CanRxTask** receives frames from loopback and:
   - Validates them.
   - Updates any derived or diagnostic state.
4. **CliTask** renders the latest state on the dashboard.

### 6.2 Logging Flow

- Each module calls `LOG_xxx` macros.
- Logs are formatted and sent over **USART2**.
- In the CLI, logs appear interleaved with dashboard updates and command feedback.

---

## 7. CLI & Dashboard Architecture

The CLI + dashboard is implemented in `cli_if.c`:

- Uses ANSI escape sequences to:
  - Clear the screen.
  - Move the cursor to row 0.
  - Reprint the dashboard on each update.
- The lower part of the terminal is reserved for:
  - User input line.
  - Command responses.
  - Log messages.

This gives the feel of a **live instrument cluster** + interactive console.

---

## 8. CI / Build Architecture

- **GitHub Actions** workflow:
  - Installs `arm-none-eabi-gcc` on Ubuntu.
  - Builds:
    - App: `app/mini_ecu_v2` (custom Makefile).
    - Bootloader: `bootloader/mini_ecu_boot` (custom Makefile).
- CI Makefiles:
  - Compile all C sources with proper include paths.
  - Do not perform final linking—CubeIDE's own build system is responsible
    for producing .elf/.bin when building locally.

This ensures platform-independent compilation checks for all source code.

---

## 9. Future Extensions

Planned architecture enhancements:

1. **Bootloader Update Protocol**
   - UART- or CAN-based firmware transfers.
   - Flash erase/programming state machine.
   - Image headers and CRC validation.

2. **Security Hooks**
   - Versioned image metadata.
   - Optional digital signature checks.
   - Secure bootloader entry conditions.

3. **Diagnostics & Tools**
   - PC-side CLI or GUI tool for:
     - Flashing new firmware.
     - Viewing real-time telemetry.
     - Sending custom CAN/CLI commands.
