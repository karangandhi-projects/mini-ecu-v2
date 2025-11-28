# CLI Commands

The UART CLI runs on USART2 (115200 8N1) and provides basic control and introspection
for the virtual ECU.

## General

- `help`  
  Show a list of available commands.

## Vehicle Control

- `veh speed X`  
  Set the **target speed** to `X` km/h (float). The vehicle model will move
  toward this speed with its built-in dynamics.

- `veh cool-hot`  
  Inject a coolant overheat condition by forcing the coolant temperature high.

Additionally, the NUCLEO **B1 user button** is treated as an accelerator pedal:
- While pressed (active-low), the speed increases by a fixed rate.
- When released, the vehicle coasts down naturally via `Vehicle_Update()`.

## Logging

- `log on`  
  Enable CAN RX logging. Each received frame is printed via the logging framework.

- `log off`  
  Disable CAN RX logging.

## Live Dashboard

A live dashboard line is pinned at the top of the terminal using ANSI escape codes,
showing:

```text
SPD:   xx.x km/h | RPM:  xxxxx | TEMP:  xxx.x C
```

This line is periodically refreshed without interfering with typed commands.
