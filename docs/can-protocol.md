# CAN Telemetry Protocol

Mini ECU v2 currently uses a single CAN telemetry frame to broadcast the
virtual vehicle state. CAN is configured in **loopback mode** on `CAN1`.

## Frame: Vehicle Telemetry

- **Identifier**: Standard ID `0x100`
- **DLC**: 6 bytes
- **Direction**: TX from ECU (and RX back due to loopback)

### Payload Layout

| Byte | Signal              | Type   | Unit       | Encoding                     |
|------|---------------------|--------|------------|------------------------------|
| 0-1  | Speed               | uint16 | 0.1 km/h   | Little-endian, 0.1 km/h LSB |
| 2-3  | Engine RPM          | uint16 | RPM        | Little-endian                |
| 4-5  | Coolant temperature | int16  | 0.1 °C     | Little-endian, 0.1 °C LSB   |

### Encoding

- Speed (km/h) → `speed_0p1 = (uint16)(speed_kph * 10.0f)`
- RPM → directly as `uint16`
- Coolant temperature (°C) → `temp_0p1 = (int16)(temp_c * 10.0f)`

### Example

If the vehicle state is:

- Speed = 50.0 km/h
- RPM   = 3300
- Temp  = 87.5 °C

Then:

- `speed_0p1 = 500` → `0x01F4` → bytes `[F4 01]`
- `rpm       = 3300` → `0x0CE4` → bytes `[E4 0C]`
- `temp_0p1  = 875`  → `0x036B` → bytes `[6B 03]`

Payload (hex):

```text
F4 01 E4 0C 6B 03
```

## RX Handling

- RX interrupt (`HAL_CAN_RxFifo0MsgPendingCallback`) reads the frame and pushes it
  into an RTOS message queue (`CanRxTask`).
- `CanRxTask` receives `CAN_IF_Msg_t` items and passes them to
  `CAN_IF_ProcessRxMsg()`.
- `CAN_IF_ProcessRxMsg()` logs the frame when RX logging is enabled.

This simple protocol can be extended later to include additional frames such as
diagnostic responses, actuator commands, or firmware update control messages.
