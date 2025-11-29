/**
 * @file    cli_if.c
 * @brief   Minimal UART CLI with live dashboard for Mini ECU.
 *
 * Design:
 *   - UART RX interrupt fills a small ring buffer.
 *   - CLI_IF_Task() runs at thread level:
 *       * consumes bytes and interprets commands
 *       * periodically prints a "speedometer" line with current values
 *
 * Live dashboard:
 *   - Shows Speed / RPM / Coolant temperature in a single line.
 *   - Uses '\r' to overwrite the same line repeatedly.
 *   - Currently uses the "true" VehicleState_t values; later we can
 *     switch to decoded CAN RX telemetry if desired.
 */

#include "cli_if.h"
#include "can_if.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>   /* atof */

/* --------------------------------------------------------------------------
 * Local state
 * -------------------------------------------------------------------------- */

/** UART used for CLI interaction. */
static UART_HandleTypeDef *s_cliUart = NULL;

/** Vehicle state used by the live dashboard and commands. */
static VehicleState_t *s_vehicle = NULL;

/** Single-byte RX buffer used by HAL UART interrupt. */
static uint8_t s_rxByte = 0U;

/** Ring buffer for RX bytes from ISR to task. */
#define CLI_RX_BUF_SIZE 64U
static uint8_t s_cliBuf[CLI_RX_BUF_SIZE];
static volatile uint8_t s_cliHead = 0U;
static volatile uint8_t s_cliTail = 0U;

/* --------------------------------------------------------------------------
 * Local helpers
 * -------------------------------------------------------------------------- */

static void cli_uart_print(const char *s)
{
    if ((s == NULL) || (s_cliUart == NULL))
        return;

    size_t len = strlen(s);
    if (len == 0U)
        return;

    (void)HAL_UART_Transmit(s_cliUart, (uint8_t *)s,
                            (uint16_t)len, HAL_MAX_DELAY);
}

/**
 * @brief Push a character into the ring buffer (ISR context).
 */
static void cli_push(uint8_t c)
{
    uint8_t next = (uint8_t)((s_cliHead + 1U) % CLI_RX_BUF_SIZE);

    if (next != s_cliTail)
    {
        s_cliBuf[s_cliHead] = c;
        s_cliHead = next;
    }
    /* else: buffer full, drop byte */
}

/**
 * @brief Pop a character from the ring buffer (task context).
 *
 * @return 1 if a character is available, 0 if buffer is empty.
 */
static int cli_pop(uint8_t *c)
{
    if (s_cliHead == s_cliTail)
        return 0;

    *c = s_cliBuf[s_cliTail];
    s_cliTail = (uint8_t)((s_cliTail + 1U) % CLI_RX_BUF_SIZE);
    return 1;
}

/**
 * @brief Update the live dashboard line with current values.
 *
 * Uses ANSI escape codes to:
 *   - Save the current cursor position
 *   - Move to the top-left of the screen (row 1, col 1)
 *   - Overwrite the dashboard line
 *   - Restore the cursor position so typing isn't disturbed
 */
static void cli_update_dashboard(void)
{
    if ((s_cliUart == NULL) || (s_vehicle == NULL))
        return;

    char buf[128];
    (void)snprintf(buf, sizeof(buf),
                   "SPD: %6.1f km/h | RPM: %5u | TEMP: %5.1f C   ",
                   s_vehicle->speed_kph,
                   (unsigned)s_vehicle->engine_rpm,
                   s_vehicle->coolant_temp_c);

    /* Save cursor position */
    cli_uart_print("\x1b[s");
    /* Go to top-left (row 1, col 1) */
    cli_uart_print("\x1b[H");
    /* Print dashboard and clear to end of line */
    cli_uart_print(buf);
    cli_uart_print("\x1b[K");
    /* Restore cursor position */
    cli_uart_print("\x1b[u");
}


/**
 * @brief Handle a single character (line assembly + command parsing).
 */
static void cli_handle_char(uint8_t c)
{
    static char   line[32];
    static uint8_t idx = 0U;

    /* Handle end-of-line (CR or LF). */
    if ((c == '\r') || (c == '\n'))
    {
        if (idx == 0U)
        {
            /* Empty line: just re-print prompt marker. */
            cli_uart_print("\r\n> ");
            return;
        }

        line[idx] = '\0';
        idx = 0U;

        LOG_DEBUG("CLI", "Command: '%s'", line);

        /* ---------------- Commands ---------------- */

        if ((strcmp(line, "h") == 0) || (strcmp(line, "help") == 0))
        {
            cli_uart_print(
                "\r\nCommands:\r\n"
                "  help          - show this help\r\n"
                "  veh speed X   - set target speed to X km/h\r\n"
                "  veh cool-hot  - inject coolant overheat\r\n"
                "  log on        - enable CAN RX logging\r\n"
                "  log off       - disable CAN RX logging\r\n> "
            );
        }
        else if (strncmp(line, "veh speed ", 10) == 0)
        {
            float v = (float)atof(&line[10]);  /* simple parser */

            if (s_vehicle != NULL)
            {
                Vehicle_SetTargetSpeed(s_vehicle, v);
                LOG_INFO("CLI", "Set target speed to %.1f km/h", v);
                cli_uart_print("\r\nOK: speed updated\r\n> ");
            }
            else
            {
                cli_uart_print("\r\nNo vehicle bound to CLI.\r\n> ");
            }
        }
        else if (strcmp(line, "veh cool-hot") == 0)
        {
            if (s_vehicle != NULL)
            {
                Vehicle_Force(s_vehicle,
                              s_vehicle->speed_kph,
                              s_vehicle->engine_rpm,
                              115.0f);
                LOG_WARN("CLI", "Injected coolant overheat");
                cli_uart_print("\r\nInjected: coolant overheat\r\n> ");
            }
            else
            {
                cli_uart_print("\r\nNo vehicle bound to CLI.\r\n> ");
            }
        }
        else if (strcmp(line, "log on") == 0)
        {
            CAN_IF_SetLogging(1U);
            LOG_INFO("CLI", "CAN RX logging enabled");
            cli_uart_print("\r\nCAN RX logging: ON\r\n> ");
        }
        else if (strcmp(line, "log off") == 0)
        {
            CAN_IF_SetLogging(0U);
            LOG_INFO("CLI", "CAN RX logging disabled");
            cli_uart_print("\r\nCAN RX logging: OFF\r\n> ");
        }
        else
        {
            cli_uart_print("\r\nUnknown command. Try 'help'.\r\n> ");
        }

        return;
    }

    /* Normal character: accumulate into line buffer and echo. */
    if (idx < (sizeof(line) - 1U))
    {
        line[idx++] = (char)c;

        if (s_cliUart != NULL)
        {
            (void)HAL_UART_Transmit(s_cliUart, &c, 1U, HAL_MAX_DELAY);
        }
    }
    /* else: line full, extra chars are ignored */
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void CLI_IF_Init(UART_HandleTypeDef *huart, VehicleState_t *vehicle)
{
    s_cliUart = huart;
    s_vehicle = vehicle;
    s_cliHead = 0U;
    s_cliTail = 0U;

    if (s_cliUart != NULL)
    {
        /* Start RX interrupt */
        (void)HAL_UART_Receive_IT(s_cliUart, &s_rxByte, 1U);

        /* Clear screen and home the cursor */
        cli_uart_print("\x1b[2J\x1b[H");

        /* Draw initial dashboard on top line */
        cli_update_dashboard();

        /* Print greeting + prompt on next line */
        cli_uart_print("\r\nCLI ready. Type 'help' and press Enter.\r\n> ");

        LOG_INFO("CLI", "CLI initialized");
    }
}


void CLI_IF_Task(void)
{
    uint8_t c;

    /* Drain RX bytes and feed parser. */
    while (cli_pop(&c))
    {
        cli_handle_char(c);
    }

    /* Periodic dashboard refresh.
     * Assumes this function is called every ~10 ms from CliTask.
     */
    static uint32_t dashCounter = 0U;
    dashCounter++;
    if (dashCounter >= 50U)  /* ~500 ms at 10 ms tick */
    {
        dashCounter = 0U;
        cli_update_dashboard();
    }
}

/* --------------------------------------------------------------------------
 * HAL callback override (ISR context)
 * -------------------------------------------------------------------------- */

/**
 * @brief UART RX complete callback override for CLI UART.
 *
 * Called by HAL when a byte is received. We push the byte into the
 * ring buffer and re-arm the next RX interrupt.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == s_cliUart)
    {
        cli_push(s_rxByte);
        (void)HAL_UART_Receive_IT(s_cliUart, &s_rxByte, 1U);
    }
}
