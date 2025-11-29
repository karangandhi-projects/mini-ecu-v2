/**
  ******************************************************************************
  * @file    main.c
  * @brief   Bootloader entry point for Mini ECU v2.
  *
  * @details
  * This is a minimal, blocking bootloader for the STM32F446RE-based
  * "Mini ECU v2" project. It is intended to live in the first 32 KB of
  * flash (sectors 0 and 1) and to:
  *
  *   1. Initialize the HAL and basic peripherals (clock, GPIO, USART2).
  *   2. Check whether a user application appears to be present at
  *      APP_START_ADDR (0x08008000).
  *   3. If a valid app is detected:
  *        - Clean up interrupts and SysTick.
  *        - Remap the vector table to the app base.
  *        - Set MSP and PC from the app's vector table.
  *        - Jump to the application's Reset_Handler.
  *   4. If no valid app is found:
  *        - Stay in a simple error loop, blinking the LD2 LED.
  *
  * The bootloader currently does not implement any update protocol; it is
  * only a clean "chain loader". Future phases will add:
  *   - Boot decision logic (e.g., hold B1 at reset to stay in bootloader).
  *   - UART- or CAN-based firmware update protocol.
  *   - Image validation (checksums, signatures).
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025.
  * All rights reserved.
  *
  * This software is provided as-is, without any warranty.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief Function pointer type for application entry point.
  *
  * This is used to represent the application's Reset_Handler as a callable
  * function after the bootloader sets up the MSP and vector table.
  */
typedef void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/**
  * @brief Perform a clean jump from the bootloader to the user application.
  *
  * @details
  * This function:
  *   - Reads the initial stack pointer and reset vector from APP_START_ADDR.
  *   - Performs a basic sanity check on the stack pointer (must be in SRAM).
  *   - De-initializes HAL, RCC, and SysTick.
  *   - Disables all interrupts.
  *   - Sets the vector table offset (VTOR) to APP_START_ADDR.
  *   - Sets MSP to the application's initial stack pointer.
  *   - Calls the application's Reset_Handler (never returns on success).
  *
  * If the application does not appear valid, this function returns and the
  * bootloader can take appropriate fallback action (e.g., stay in an error
  * loop or enter update mode).
  */
static void JumpToApplication(void);

/**
  * @brief Check if the user button (B1) is currently pressed.
  *
  * @retval 1 if pressed, 0 otherwise.
  *
  * @note On NUCLEO-F446RE, B1 is active-high on PC13 when using the default
  *       board configuration. Adjust the logic if your hardware differs.
  */
static uint8_t Boot_IsButtonPressed(void)
{
  GPIO_PinState state = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
  return (state == GPIO_PIN_SET) ? 1U : 0U;
}

/**
  * @brief Print a simple ASCII message over UART2.
  *
  * @param msg  Null-terminated C-string (no printf formatting here).
  */
static void Boot_Print(const char *msg)
{
  if (msg == NULL)
  {
    return;
  }
  size_t len = strlen(msg);
  if (len == 0U)
  {
    return;
  }
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)len, 100);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void JumpToApplication(void)
{
  uint32_t appStack = *(__IO uint32_t *)APP_START_ADDR;
  uint32_t appReset = *(__IO uint32_t *)(APP_START_ADDR + 4U);

  /*-------------------------------------------------------------------------
   * Sanity check: the initial stack pointer should point into SRAM.
   * For STM32F446RE:
   *   - SRAM1: 0x2000 0000 .. 0x2001 FFFF (128 KB)
   * If this check fails, we assume there is no valid application image.
   *-----------------------------------------------------------------------*/
  if ((appStack < 0x20000000U) || (appStack > 0x20020000U))
  {
    /* No apparently valid app; do not jump. */
    return;
  }

  /* De-initialize peripherals and HAL to leave a clean state. */
  HAL_RCC_DeInit();
  HAL_DeInit();

  /* Disable SysTick to avoid unwanted interrupts after the jump. */
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL  = 0U;

  /* Disable all NVIC interrupts and clear any pending flags. */
  for (uint8_t i = 0U; i < 8U; ++i)
  {
    NVIC->ICER[i] = 0xFFFFFFFFU;
    NVIC->ICPR[i] = 0xFFFFFFFFU;
  }

  /* Remap vector table to the application base address. */
  SCB->VTOR = APP_START_ADDR;

  /* Set the main stack pointer to the application's initial stack. */
  __set_MSP(appStack);

  /* Jump to the application's Reset_Handler. */
  pFunction appEntry = (pFunction)appReset;
  appEntry();

  /* Should never reach here if the application is valid. */
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point for the bootloader.
  * @retval int Not used (function does not normally return).
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock. */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals. */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  /* Basic banner so you know bootloader is alive. */
    Boot_Print("\r\n[BOOT] Mini ECU v2 bootloader\r\n");
    Boot_Print("[BOOT] Hold B1 during reset to stay in bootloader.\r\n");

    /* Small delay so user has time to keep button pressed after reset if needed. */
    HAL_Delay(10);

    /* Decide boot mode:
     *  - If B1 is pressed at startup  -> stay in bootloader (future update mode).
     *  - Otherwise                    -> attempt to jump to application.
     */
    if (Boot_IsButtonPressed())
    {
      Boot_Print("[BOOT] B1 is pressed: staying in bootloader.\r\n");
      Boot_Print("[BOOT] (Future) OTA / firmware update mode.\r\n");

      /* Phase 2: no update protocol yet, just blink LED + print messages. */
      while (1)
      {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(300);
      }
    }
    else
    {
      Boot_Print("[BOOT] B1 not pressed: attempting to jump to application...\r\n");
      JumpToApplication();
    }

    /* If we reach this point, the application was not considered valid. */
    Boot_Print("[BOOT] No valid application found. Staying in error loop.\r\n");
    while (1)
    {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      HAL_Delay(500);
    }
  /* USER CODE END 2 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  *
  * @note
  * This is a simple HSI-based clock configuration suitable for the
  * bootloader. The application is free to reconfigure the clock tree
  * as desired once it takes over.
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  *
  * @details
  * Configures:
  *   - LD2 (user LED) as push-pull output for basic status indication.
  *   - B1 (user button) as input interrupt (future use for boot mode).
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin   = LD2_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin (user button, for future boot mode selection) */
  GPIO_InitStruct.Pin  = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  *
  * @details
  * Configures USART2 for basic logging/diagnostics at 115200 8N1.
  * Currently unused by the bootloader logic, but kept ready for
  * future phases (e.g., UART-based firmware update protocol).
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance          = USART2;
  huart2.Init.BaudRate     = 115200;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  *
  * @details
  * In this minimal bootloader, Error_Handler simply stops execution
  * with interrupts disabled. For debugging, you can set a breakpoint
  * here or add LED blink patterns.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    /* Stay here; optionally toggle LD2 for visual indication. */
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_Delay(250);
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
  /* You can add debug prints here if needed. */
}
#endif /* USE_FULL_ASSERT */
