# Bootloader & OTA Plan (High Level)

This project is intended to grow into a small playground for:
- A **custom bootloader** on STM32F446RE
- **Firmware update** over UART or CAN
- (Optionally) hooks for **secure / authenticated** updates

## Phase 1 – Basic Bootloader Skeleton

- Place a minimal bootloader in early flash (e.g., sectors 0–1).
- Relocate the application to start at `0x08008000`.
- Implement a `JumpToApplication()` routine that:
  - Validates the presence of an app (simple stack-pointer sanity check).
  - De-initializes HAL and interrupts.
  - Sets MSP and PC from the app's vector table.
  - Jumps to the app's Reset_Handler.

## Phase 2 – Boot Mode Decisions

- Add logic to choose between:
  - **Normal boot**: jump directly to application.
  - **Update mode**: stay in bootloader (e.g., if user button held at reset
    or app is invalid).
- Provide minimal user feedback (e.g., LED blink patterns, UART logs).

## Phase 3 – Firmware Update Protocol

- Define a simple, robust protocol over UART (or CAN) to:
  - Receive a new firmware image in chunks.
  - Erase relevant app flash sectors.
  - Program data safely with verification.
- Implement basic integrity checks (CRC or checksum).

## Phase 4 – Hardening & Security Hooks

- Add image header with versioning and length.
- Add stronger integrity validation (CRC-32 or better).
- Add hooks for future authentication (e.g., signatures), even if not fully
  implemented at first.

This document tracks the roadmap; implementation details can be refined as the
project evolves.
