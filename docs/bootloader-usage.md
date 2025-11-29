# Bootloader Usage

This document explains how to use the Mini ECU v2 bootloader and how it interacts
with the application firmware.

## Flash Layout

On STM32F446RE (512 KB flash), the project uses this layout:

- **Bootloader**: 0x0800 0000 – 0x0800 7FFF (32 KB, sectors 0–1)
- **Application**: 0x0800 8000 – 0x0807 FFFF (480 KB, sectors 2–7)

The application firmware is linked to start at **0x08008000**, and the vector
table is offset by 32 KB (VECT_TAB_OFFSET = 0x00008000).

The bootloader always starts first after reset (vectors at 0x08000000). It then
decides whether to stay in bootloader mode or jump to the application.

## Boot Modes

On reset, the bootloader performs the following steps:

1. Initialize HAL, clocks, GPIO, and USART2.
2. Print a small banner over USART2 (115200 8N1), e.g.:
   ```text
   [BOOT] Mini ECU v2 bootloader
   [BOOT] Hold B1 during reset to stay in bootloader.
   ```
3. Sample the **B1 user button** on the NUCLEO-F446RE:
   - If **B1 is pressed** (active-low) at startup:
     - Stay in bootloader mode.
     - Print:
       ```text
       [BOOT] B1 is pressed: staying in bootloader.
       [BOOT] (Future) OTA / firmware update mode.
       ```
     - Blink LD2 in a loop (e.g., every ~300 ms).
   - If **B1 is not pressed**:
     - Attempt to jump to the application image at 0x08008000.
     - Print:
       ```text
       [BOOT] B1 not pressed: attempting to jump to application...
       ```

If the application is not valid (e.g., missing or corrupted vector table), the
bootloader prints a message, then falls back to an error loop blinking LD2.

## Valid Application Detection

The bootloader checks the first two words at APP_START_ADDR:

- `*(APP_START_ADDR + 0)` -> initial stack pointer value
- `*(APP_START_ADDR + 4)` -> application's Reset_Handler address

A simple sanity check is applied:

- The initial stack pointer must lie within the valid SRAM range:
  - 0x2000 0000 .. 0x2001 FFFF on STM32F446RE.

If this check fails, the bootloader considers the application invalid and does
not jump.

## Jump Sequence

When a valid application is found, the bootloader:

1. Calls `HAL_RCC_DeInit()` and `HAL_DeInit()` to clean up HAL state.
2. Disables SysTick and all NVIC interrupts.
3. Sets `SCB->VTOR` to the application base (`APP_START_ADDR`).
4. Sets MSP (`__set_MSP`) to the application's initial stack pointer.
5. Calls the application's Reset_Handler function pointer.

The application then proceeds exactly as if it had started directly at reset,
with its own clock configuration and FreeRTOS startup.

## Serial Console Expectations

When you connect at **115200 8N1** to USART2 and reset the board:

- If **booting the app**:
  - You will briefly see `[BOOT]` messages.
  - Then the main Mini ECU v2 logs and CLI banner appear.
- If **staying in bootloader** (B1 pressed):
  - You will see only `[BOOT]` messages and the LED blink; the application
    will not start.

This behaviour is intentional and is the foundation for future UART-based
firmware update commands.
