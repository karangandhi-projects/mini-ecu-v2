# Bootloader & OTA Plan

This project is intended to grow into a small playground for:
- A **custom bootloader** on STM32F446RE
- **Firmware update** over UART or CAN
- Hooks for **secure / authenticated** updates

## Current Status (v0.3.0)

The following pieces are implemented:

- **Custom bootloader** (`mini_ecu_boot`) in the first 32 KB of flash.
- Application relocated to `0x08008000` with vector table offset.
- `JumpToApplication()` logic with:
  - Stack-pointer range validation.
  - Clean shutdown of HAL, SysTick, and interrupts before jump.
  - VTOR remap to the application base.
- Boot mode selection:
  - **B1 pressed at reset** -> stay in bootloader (future update / diagnostic mode).
  - **B1 not pressed** -> jump to application if valid, otherwise blink error pattern.
- Basic UART2 logging from the bootloader for visibility.

## Phase Breakdown

### Phase 1 – Basic Bootloader Skeleton ✅

- Place a minimal bootloader in early flash (sectors 0–1).
- Relocate the application to start at `0x08008000`.
- Implement a `JumpToApplication()` routine that:
  - Validates the presence of an app (simple stack-pointer sanity check).
  - De-initializes HAL and interrupts.
  - Sets MSP and PC from the app's vector table.
  - Jumps to the app's Reset_Handler.

### Phase 2 – Boot Mode Decisions + UART Logs ✅

- Add logic to choose between:
  - **Normal boot**: jump directly to application.
  - **Bootloader mode**: stay in bootloader if B1 is held at reset or if app is invalid.
- Add simple UART2 boot logs so the behaviour is visible over serial.

### Phase 3 – Firmware Update Protocol (Planned)

- Define a simple, robust protocol over UART (or CAN) to:
  - Receive a new firmware image in chunks.
  - Erase relevant app flash sectors.
  - Program data safely with verification.
- Implement integrity checks (CRC or checksum) across the received image.
- Add a minimal command set, e.g.:
  - `INFO` – report bootloader/app version, layout.
  - `ERASE` – erase application flash region.
  - `WRITE` – program a chunk of data.
  - `DONE` – finalize and validate the image.

### Phase 4 – Hardening & Security Hooks (Future)

- Add image header with versioning, length, and CRC.
- Add hooks for digital signatures (even if not fully enforced initially).
- Consider dual-bank or A/B slot strategies (if ported to an MCU that supports them).

This plan is intentionally incremental so that each phase remains buildable and
testable on real hardware.
