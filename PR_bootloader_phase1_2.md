# Mini ECU v2 – Bootloader Phase 1 & 2

## Summary

This PR adds a custom bootloader for the Mini ECU v2 project and wires it up
to the existing application. The bootloader supports a clean jump to the app
and a simple boot mode selection mechanism using the NUCLEO user button (B1).

## Changes

- Added a **bootloader project** (`mini_ecu_boot`) intended to live in the first
  32 KB of flash (sectors 0–1).
- Implemented `JumpToApplication()` in the bootloader:
  - Reads initial stack pointer and Reset_Handler from `APP_START_ADDR` (0x08008000).
  - Validates that the stack pointer is within the SRAM range.
  - De-initializes HAL and RCC, disables SysTick and all NVIC interrupts.
  - Remaps the vector table to `APP_START_ADDR` via `SCB->VTOR`.
  - Sets MSP and jumps to the application's Reset_Handler.
- Updated the **application linker script** to:
  - Start at `0x08008000` with 480 KB FLASH.
  - Use `VECT_TAB_OFFSET = 0x00008000` so the vector table matches the new base.
- Added **boot mode selection**:
  - If B1 is pressed at reset -> stay in bootloader (future update mode).
  - Otherwise -> attempt to jump to the application.
- Added **basic UART2 boot logs** (115200 8N1) that show:
  - Bootloader banner.
  - Instructions about holding B1 to stay in bootloader.
  - Whether the bootloader is jumping to app or staying in bootloader.
- Updated documentation:
  - `CHANGELOG.md` with version **0.3.0**.
  - `docs/bootloader-plan.md` detailing bootloader phases.
  - `docs/bootloader-usage.md` describing flash layout, boot modes, and logs.

## Testing

- [x] Bootloader builds successfully in STM32CubeIDE.
- [x] Application builds successfully with FLASH origin at 0x08008000.
- [x] Flashed bootloader to 0x08000000 and app to 0x08008000.
- [x] On reset **without** B1 pressed:
  - `[BOOT]` messages appear briefly.
  - Mini ECU v2 app starts and shows CLI + dashboard.
- [x] On reset **with** B1 pressed:
  - Bootloader prints that it is staying in bootloader.
  - LED (LD2) blinks in the bootloader loop; app does not start.
- [x] When app is erased or missing:
  - Bootloader reports no valid application and stays in error loop blinking LD2.

## Notes

- This PR completes **Bootloader Phase 1 & 2**:
  - Chainloading from bootloader to app.
  - Boot decision via B1 + UART logs.
- No firmware update protocol is implemented yet; that will be handled in a
  future PR (Phase 3).

