# Simple CI Makefile for Mini ECU v2 application
# - Only does compilation (no linking)
# - Intended for use on GitHub Actions with arm-none-eabi-gcc

CC      := arm-none-eabi-gcc

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
           -O0 -g3 -ffunction-sections -fdata-sections -Wall \
           -DSTM32F446xx -DUSE_HAL_DRIVER -DDEBUG

INCLUDES := \
  -ICore/Inc \
  -IDrivers/STM32F4xx_HAL_Driver/Inc \
  -IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy \
  -IMiddlewares/Third_Party/FreeRTOS/Source/include \
  -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
  -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
  -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IDrivers/CMSIS/Include

# All C sources we care about
SRCS := \
  $(wildcard Core/Src/*.c) \
  $(wildcard Drivers/STM32F4xx_HAL_Driver/Src/*.c) \
  $(wildcard Middlewares/Third_Party/FreeRTOS/Source/*.c) \
  $(wildcard Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c) \
  $(wildcard Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/*.c)

OBJS := $(patsubst %.c,build/%.o,$(SRCS))

.PHONY: all clean

all: $(OBJS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build
