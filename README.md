# Mini ECU v2 – STM32F446RE + FreeRTOS + Custom Bootloader

Mini ECU v2 is a fully functional **virtual automotive ECU** implemented on the  
**NUCLEO-F446RE** using:

- **STM32 HAL**
- **FreeRTOS**
- **Custom Bootloader (Phase 1 & 2 complete)**
- **CAN loopback telemetry**
- **UART CLI with live dashboard**
- **Virtual vehicle model + accelerator input**
- **Unified logging system**
- **CI workflow via GitHub Actions (ARM-GCC)**

The project simulates a real ECU by processing virtual sensor values, displaying
them over a CLI dashboard, and exchanging CAN messages. A custom bootloader
chain-loads the main application and will later support OTA/FW-update protocols.

---

## 📁 Repository Structure

```
mini-ecu-v2/
│
├── app/
│   └── mini_ecu_v2/          # FreeRTOS-based main ECU application
│
├── bootloader/
│   └── mini_ecu_boot/        # Custom STM32 bootloader (Phase 1 & 2 complete)
│
├── docs/
│   ├── bootloader-plan.md
│   ├── bootloader-usage.md
│   └── architecture.md
│
├── .github/
│   └── workflows/build.yml   # CI builds bootloader + app
│
├── CHANGELOG.md
└── README.md
```

---

# 🚗 **Main ECU Application Features**

### ✅ **Virtual Vehicle Model**
- Speed (km/h)
- Engine RPM
- Coolant temperature
- All values computed in `vehicle.c` and updated via FreeRTOS task

### ✅ **Accelerator (Throttle) Simulation**
The NUCLEO board's **B1 button** acts as a throttle:
- Button pressed → speed increases  
- Button released → speed decays  
- RPM scales with speed

### ✅ **CAN Telemetry (Loopback Mode)**
The project uses **CAN1 loopback**:
- TX: publishes live vehicle telemetry  
- RX: CAN frames queued and processed in a dedicated task  
- Processed values feed the dashboard

### ✅ **UART CLI + Live Dashboard**
A terminal (115200 8N1) shows:
- A **persistent dashboard** printed at the top
- Below it: an interactive CLI

Example:

```
-------------------- MINI ECU V2 --------------------
Speed:  42 km/h    RPM: 2100    Coolant: 87 °C
------------------------------------------------------

> help
```

ANSI cursor control ensures the dashboard always stays pinned at the top.

### ✅ **Logging Framework**
Modules use:

```
LOG_INFO("Vehicle", "Speed updated to %d", speed);
LOG_WARN("CAN", "Invalid DLC: %d", dlc);
LOG_ERROR("CLI", "Unknown command: %s", cmd);
```

Logs are visible in *both app and bootloader*.

---

# 🥾 **Custom Bootloader (Phase 1 & 2 Complete)**

### 📌 Flash Layout (STM32F446RE)

| Region         | Flash Address        | Size     |
|----------------|----------------------|----------|
| Bootloader     | `0x0800 0000`        | 32 KB    |
| Application    | `0x0800 8000`        | 480 KB   |

### 📌 Boot Flow

1. Bootloader starts at reset  
2. Shows UART banner  
3. Reads **B1** (active-low) to decide mode:

#### ✔ Normal Mode  
If B1 **is pressed** during reset:

```
[BOOT] Jumping to application...
```

Bootloader validates the app vector table, remaps VTOR, sets MSP, and jumps.

#### ✔ Bootloader Mode  
If B1 is **not** pressed:

```
[BOOT] B1 is pressed: staying in bootloader.
[BOOT] (Future) OTA / firmware update mode.
```

Bootloader stays active, LED blinks.

### 📌 Safety Checks
- Validates stack pointer range (`0x2000 0000 – 0x2001 FFFF`)
- Disables SysTick & NVIC IRQs before jumping
- Calls `HAL_DeInit()` + `HAL_RCC_DeInit()`
- Application linker offset handled via `VECT_TAB_OFFSET`

---

# 🔧 **Building the Projects**

## ➤ Build using STM32CubeIDE
Just open each project:
- `app/mini_ecu_v2`
- `bootloader/mini_ecu_boot`

Press **Build Project**.

---

## ➤ Build using CI (GitHub Actions)
The repo includes a clean, Linux-friendly Makefile for CI builds.

### App:
```
cd app/mini_ecu_v2
make
```

### Bootloader:
```
cd bootloader/mini_ecu_boot
make
```

These CI Makefiles:
- Compile all modules  
- Do *not* link firmware (CubeIDE handles that)  
- Ensure repo compiles on ARM-GCC without Windows paths  

---

# 🔥 **Flashing Instructions**

### 1️⃣ Flash bootloader
In CubeIDE → Run/Debug Configurations:
- Select bootloader project
- Flash to `0x08000000`

### 2️⃣ Flash application
Your app linker script already places it at `0x08008000`.  
Flash normally using CubeIDE.

### 3️⃣ Reset board
- B1 pressed ➝ Bootloader mode  
- B1 released ➝ Jump to app

---

# 🧪 **Continuous Integration (CI)**

GitHub Actions performs:

- ARM-GCC installation  
- Build of bootloader (Linux Makefile)  
- Build of application (Linux Makefile)  

The workflow ensures:
- Proper include paths  
- No missing sources  
- HAL/FreeRTOS modules compile correctly  

---

# 🛣️ **Roadmap**

### ✔ Completed
- FreeRTOS ECU application  
- CAN loopback telemetry  
- CLI + dashboard  
- Logging framework  
- Virtual vehicle model  
- Bootloader (Phase 1 & 2)

### 🔜 Next Phases

#### **Phase 3 — UART/CAN Firmware Update Protocol**
- Erase App region  
- Stream FW image in chunks  
- CRC validation  
- Flash programming state machine  
- Update progress logs  

#### **Phase 4 — Secure Updates**
- Firmware header structure  
- Versioning  
- Image signatures (HMAC or ECDSA)  
- Revert/fallback strategy  

#### **Phase 5 — Web-UI or PC Tool**
CLI tool for:
- Flashing firmware via UART  
- Viewing logs  
- Sending CAN commands  

---

# 📜 License
MIT 

---

# 🙌 Credits
Designed & developed by **Karan Gandhi**  
Real-time embedded + firmware engineer.
