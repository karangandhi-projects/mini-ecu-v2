# Contributing to Mini ECU v2

Thanks for your interest in contributing! This repo is primarily a learning and demo
project for embedded firmware (STM32F4, FreeRTOS, CAN, CLI, bootloaders). Even if
it's mostly used by a single developer, having a clean workflow makes changes safer.

## Branching Model

- `main` – stable branch; should always build and run on hardware.
- `bootloader-dev` – work-in-progress branch for the custom bootloader & OTA features.
- Feature branches – for specific tasks, e.g.:
  - `feat/add-gearbox-model`
  - `feat/bootloader-uart-update`
  - `fix/can-telemetry-scaling`

## Commit Messages

Try to keep commit messages:
- Short but descriptive in the subject line.
- Optionally followed by a blank line and more detail.

Examples:

- `feat: add accelerator pedal using B1`
- `refactor: move CAN encode logic into CAN_IF`
- `fix: correct temperature scaling in telemetry frame`

## Pull Requests

Before opening a PR:

1. Make sure the project builds in STM32CubeIDE.
2. Run `make -j` locally (if you use the Makefile build).
3. Update:
   - `CHANGELOG.md` if behaviour changed.
   - `docs/` if CLI commands, CAN protocol, or architecture changed.
4. Add or update Doxygen-style comments for any new public functions.

Use the PR template provided by `.github/pull_request_template.md` and make sure
the checklist is satisfied as much as possible.

## Coding Style

- C code: follow a simple, STM32/embedded-friendly style:
  - 2 or 4 spaces, no tabs (up to you; keep consistent per file).
  - Brace on the same line as `if`, `for`, `while`, function declarations.
  - Explicit `uint8_t`, `uint16_t`, etc. for fixed-width types.
- Avoid magic numbers in the code; prefer named constants or `#define`s.
- Keep module responsibilities clear:
  - `vehicle.*` – physics / state.
  - `can_if.*` – CAN abstraction and telemetry encoding.
  - `cli_if.*` – UART CLI and dashboard.
  - `log.*` – logging only.

## Doxygen & Documentation

When adding new functions or modules:
- Add a brief Doxygen comment at the top of the file (`@file`, `@brief`).
- Document public functions with `@brief` and parameter descriptions.
- Update or add docs under `docs/` if it affects architecture, CLI, or protocols.

## Tests

There is currently no automated unit test framework in this repo, but you can:
- Add small test harnesses (e.g., simple C files) that exercise modules on host
  using a native compiler, or
- Describe how you tested on hardware in the PR (UART logs, CAN dumps, etc.).

## Questions

If something is unclear or you want feedback on an idea (e.g., bootloader design,
OTA protocol), open a **feature request** issue or draft PR and describe your idea.
