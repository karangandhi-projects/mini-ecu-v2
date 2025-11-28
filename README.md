# Mini ECU v2 – STM32F446RE Virtual Vehicle ECU

Mini ECU v2 is a **virtual automotive ECU demo** running on an STM32F446RE (e.g., NUCLEO-F446RE) that simulates a simple vehicle and streams telemetry over CAN, with a FreeRTOS-based architecture and a UART CLI with a live dashboard.

The goal is to look and feel like a small real ECU:
- **Virtual vehicle model** (speed, RPM, coolant temperature)
- **CAN loopback telemetry** (encoded vehicle state)
- **FreeRTOS tasks** for vehicle update, CAN RX, and CLI
- **UART CLI** with commands and a **live “speedometer” line**
- **Logging framework** with levels and module tags
- **User button as accelerator pedal** (hold B1 to “drive” the vehicle)

This project is also the foundation for the next phase:  
👉 adding a **custom bootloader** and eventually **secure OTA firmware updates**.

---

## Features

### Virtual Vehicle Model (`vehicle.c/.h`)

- Represents a simplified **physical vehicle state**:
  - Speed (km/h)
  - Engine RPM
  - Coolant temperature (°C)
- Physics model:
  - Speed naturally **coasts down** over time.
  - RPM follows speed with a **first-order lag** (simulates engine inertia).
  - Coolant temperature **warms up** toward operating temperature under load and cools slightly at idle.
- Provides APIs:
  - `Vehicle_Init()` – initialize state
  - `Vehicle_Update()` – advance physics by `dt_s`
  - `Vehicle_SetTargetSpeed()` – change physical speed (e.g., from CLI or accelerator)
  - `Vehicle_Force()` – direct override for fault injection/testing

---

### CAN Interface (`can_if.c/.h`)

- Wraps STM32 HAL CAN in a small, testable API:
  - Configures **CAN1 in loopback mode** (no external transceiver needed).
  - Sets up a **permissive filter** (accept all IDs into FIFO0).
  - Creates an **RTOS message queue** for CAN RX (`CanRxTask`).
  - Activates interrupts for RX and error events.
- Telemetry format:
  - Standard ID: `0x100`
  - DLC: 6 bytes
  - Payload:
    - Bytes 0–1: speed in 0.1 km/h (uint16, little endian)
    - Bytes 2–3: RPM (uint16, little endian)
    - Bytes 4–5: coolant temperature in 0.1 °C (int16, little endian)
- Functions:
  - `CAN_IF_Init()` – configure filter, start CAN, enable notifications, create RX queue
  - `CAN_IF_SendTelemetry()` – encode/send current `VehicleState_t`
  - `CAN_IF_SetLogging()` – enable/disable CAN RX logging (controlled via CLI)
  - `CAN_IF_GetRxQueueHandle()` – expose RX queue to `CanRxTask`
  - `CAN_IF_ProcessRxMsg()` – decode and log received frames using the logger

---

### UART CLI + Live Dashboard (`cli_if.c/.h`)

- Interrupt-driven RX into a ring buffer.
- Non-blocking CLI task (`CLI_IF_Task`) that:
  - Consumes characters from the ring buffer.
  - Implements a **line-based command parser**.
  - Periodically updates a **live dashboard line** with current vehicle state.
- The dashboard uses **ANSI escape codes** to:
  - Stay pinned at the **top of the terminal**.
  - Show: `SPD / RPM / TEMP` without interfering with typing.

Example dashboard line:

```text
SPD:   50.0 km/h | RPM:  3300 | TEMP:  87.5 C
```

#### Supported CLI commands

- `help`  
  Show command list.

- `veh speed X`  
  Set target vehicle speed to `X` km/h (float).

- `veh cool-hot`  
  Inject a coolant overheat condition (forces high coolant temperature).

- `log on` / `log off`  
  Enable or disable **CAN RX logging** (frames are printed via logger).

The CLI is initialized with:

```c
CLI_IF_Init(&huart2, &g_vehicle);
```

and processed in `CliTask`:

```c
for (;;)
{
    CLI_IF_Task();
    osDelay(10);
}
```

---

### Logging Framework (`log.c/.h`)

- Lightweight logging layer with levels:
  - `LOG_LEVEL_ERROR`
  - `LOG_LEVEL_WARN`
  - `LOG_LEVEL_INFO`
  - `LOG_LEVEL_DEBUG`
- Logs are printed over a configured UART (typically `USART2`) with module tags:

```text
[I][MAIN] Mini ECU – CAN + RTOS Telemetry Node starting...
[I][CAN] CAN_IF initialized successfully
[I][CLI] CLI initialized
```

- Usage:

```c
LOG_INFO("MAIN", "Init complete, creating RTOS tasks...");
LOG_WARN("CAN", "Telemetry TX failed, status=%ld", (long)status);
LOG_DEBUG("CLI", "Command: '%s'", line);
```

- Initialization in `main.c`:

```c
Log_Init(&huart2);
Log_SetLevel(LOG_LEVEL_INFO);
```

---

### FreeRTOS Tasks (`main.c`)

- **VehicleTask**
  - Runs at **10 Hz**.
  - Reads the **accelerator pedal** (B1 button on NUCLEO, active-low).
  - While button pressed:
    - Increases speed by a fixed rate (e.g., ~10 km/h per second).
  - Calls `Vehicle_Update()` to advance physics.
  - Sends CAN telemetry via `CAN_IF_SendTelemetry()`.

- **CliTask**
  - Runs every ~10 ms.
  - Calls `CLI_IF_Task()` to process RX characters and update dashboard.

- **CanRxTask**
  - Blocks on the CAN RX queue.
  - For each received frame:
    - Calls `CAN_IF_ProcessRxMsg()` to log / inspect it.

---

## Hardware Requirements

- **Board:** STM32 NUCLEO-F446RE (or compatible STM32F446-based board).
- **Interfaces:**
  - USB (for programming and UART console via ST-LINK VCP).
  - Onboard **User Button (B1)** – used as accelerator pedal.
  - Onboard **LED (LD2)** – currently unused in the app but available.

> CAN is configured in **loopback mode**, so you do *not* need an external transceiver for basic testing.

---

## Building & Running

### Toolchain

- **STM32CubeIDE**
- Target: `STM32F446RETx`

### Steps

1. Clone the repository:

   ```bash
   git clone https://github.com/<your-username>/mini-ecu-v2.git
   cd mini-ecu-v2
   ```

2. Open the project in **STM32CubeIDE**:
   - `File → Open Projects from File System…`
   - Import the project folder.

3. Build:
   - Select the project.
   - Click the **hammer** (Build).

4. Flash to board:
   - Connect the NUCLEO via USB.
   - Click **Debug** or **Run** in STM32CubeIDE.

5. Open a serial terminal:
   - Baud: `115200`
   - Data bits: `8`
   - Parity: `None`
   - Stop bits: `1`
   - Port: ST-LINK Virtual COM port.

You should see:

- A cleared screen with the live dashboard pinned on the first line.
- A CLI greeting:

  ```text
  CLI ready. Type 'help' and press Enter.
  >
  ```

- Logs like:

  ```text
  [I][MAIN] Mini ECU – CAN + RTOS Telemetry Node starting...
  [I][CAN] CAN_IF initialized successfully
  [I][CLI] CLI initialized
  ```

---

## Project Structure

High-level structure (CubeIDE-style):

```text
Core/
  Inc/
    main.h
    vehicle.h
    can_if.h
    cli_if.h
    log.h
    FreeRTOSConfig.h
  Src/
    main.c
    vehicle.c
    can_if.c
    cli_if.c
    log.c
    freertos.c      # CubeMX-generated
Drivers/
  ...
Middlewares/
  Third_Party/
    FreeRTOS/
  ...
```

---

## Roadmap

**Phase 1 – Current (Mini ECU v2)** ✅  
- Virtual vehicle model  
- CAN loopback telemetry  
- FreeRTOS-based tasks  
- UART CLI with live dashboard  
- Logging framework  
- User button = accelerator pedal  

**Phase 2 – Custom Bootloader (Planned)**  
- Bootloader in lower flash (e.g., sectors 0–1).  
- Application relocated to higher flash (from 0x08008000).  
- Jump-to-application logic with basic validity checks.  
- Boot decision (e.g., button pressed = stay in bootloader).  

**Phase 3 – Firmware Update / OTA (Planned)**  
- Simple UART or CAN-based firmware update protocol.  
- Safe flash erase/program flow.  
- Image validation (checksum / signature hooks).  

---

## License

This project is currently unlicensed.  
You may **not** redistribute or use this code in closed-source or commercial projects without permission.

If you decide to open-source it later, a recommended choice is:
- [MIT License](https://opensource.org/licenses/MIT)
- or [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0)

---

## Author

**Karan Gandhi**  
Embedded / Firmware Engineer – STM32, RTOS, CAN, C  
(NUCLEO-F446RE Virtual ECU Playground)
