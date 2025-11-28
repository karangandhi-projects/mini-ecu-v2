# Changelog

All notable changes to this project will be documented in this file.

The format loosely follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.2.0] - 2025-11-28

### Added
- Vehicle accelerator feature using the NUCLEO user button (B1) as a virtual pedal.
- Live CLI dashboard pinned to the top of the terminal using ANSI escape codes.
- Logging framework (`log.c/.h`) with levels (ERROR/WARN/INFO/DEBUG) and module tags.
- Doxygen-style documentation and improved comments for:
  - `vehicle.c/.h`
  - `can_if.c/.h`
  - `cli_if.c/.h`
  - `main.c` RTOS tasks.

### Changed
- Replaced ad-hoc UART prints with centralized `LOG_*` macros.
- Refactored CAN interface into `CAN_IF` abstraction with RX queue and logging.

## [0.1.0] - 2025-11-XX

### Added
- Initial Mini ECU application:
  - Basic vehicle model (speed, RPM, coolant temp).
  - CAN loopback telemetry on CAN1.
  - FreeRTOS tasks (VehicleTask, CliTask, CanRxTask).
  - UART CLI with basic commands and status display.
