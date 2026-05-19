#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_vfs_dev.h" // Old headers for the aliasing functions

#include "driver/uart_vfs.h"

#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "linenoise/linenoise.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "main.h"
#include "ft8xx.h"
#include "console.h"
#include "xm_cpp.h"
#include "tsf.h"
#include "fatfs.h"
#include "cmd.h"

#ifdef CONFIG_ESP32_WIFI_ENABLED
#include "wifi.h"
#endif

#if SOC_SDMMC_HOST_SUPPORTED
#include "sdmmc.h"
#endif

#define PROMPT_STR "zifi32"
#define CONSOLE_UART_RX_BUF_SIZE 1024
#define CONSOLE_PROMPT_MAX_LEN 32

char prompt_buf[CONSOLE_PROMPT_MAX_LEN];
const char *prompt = prompt_buf;

void console_register_commands()
{
  esp_console_register_help_command();
  esp_console_register_system_commands();
#ifdef CONFIG_ESP32_WIFI_ENABLED
  esp_console_register_wifi_commands();
#endif
  ft_console_register_system_commands();
#if SOC_SDMMC_HOST_SUPPORTED
  sdmmc_console_register_system_commands();
#endif
  xm_console_register_system_commands();
  tsf_console_register_system_commands();
  fat_console_register_system_commands();
  cmd_console_register_system_commands();
}

void console_update_prompt()
{
  if (linenoiseIsDumbMode())
  {
    snprintf(prompt_buf, sizeof(prompt_buf), PROMPT_STR "> ");
    return;
  }

#if CONFIG_LOG_COLORS
  snprintf(prompt_buf, sizeof(prompt_buf), LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR);
#else
  snprintf(prompt_buf, sizeof(prompt_buf), PROMPT_STR "> ");
#endif
}

void initialize_console()
{
  // Drain stdout before reconfiguring it
  fflush(stdout);
  fsync(fileno(stdout));

  // Disable buffering on stdin
  setvbuf(stdin, NULL, _IONBF, 0);
  log_sram_used(__FILE_NAME__ ": setvbuf");

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);

  /* Move the caret to the beginning of the next line on '\n' */
  uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

  // Install UART driver for interrupt-driven reads and writes
  ESP_ERROR_CHECK(uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, CONSOLE_UART_RX_BUF_SIZE, 0, 0, NULL, 0));
  log_sram_used(__FILE_NAME__ ": uart_driver_install");

  // Tell VFS to use UART driver
  uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

  // Initialize the console
  esp_console_config_t console_config =
  {
    .max_cmdline_length = 256,
    .max_cmdline_args   = 8,
#if CONFIG_LOG_COLORS
    .hint_color         = atoi(LOG_COLOR_CYAN)
#endif
  };
  ESP_ERROR_CHECK(esp_console_init(&console_config));

  // Configure linenoise line completion library.
  // Single-line mode sends less data on each key press and is much more stable on UART.
  linenoiseSetMultiLine(0);

  // Detect whether the terminal supports escape sequences.
  if (linenoiseProbe() != 0)
    linenoiseSetDumbMode(1);

  // Tell linenoise where to get command completions.
  linenoiseSetCompletionCallback(&esp_console_get_completion);

  // Hints trigger additional redraw traffic on every key press.
  // Disabling them makes repeated backspace handling much more stable.
  // linenoiseSetHintsCallback((linenoiseHintsCallback*)&esp_console_get_hint);

  // Set command history size
  linenoiseHistorySetMaxLen(100);

  // Set command maximum length
  linenoiseSetMaxLineLen(console_config.max_cmdline_length);

  // Don't return empty lines
  linenoiseAllowEmpty(false);

  console_update_prompt();

  // Register commands
  console_register_commands();

  log_sram_used(__FILE_NAME__ ": initialize_console end");
}

void console_task(void *arg)
{
  _delay_ms(30);

  printf("\n"
         "Type 'help' to get the list of commands.\n"
         "Use UP/DOWN arrows to navigate through command history.\n"
         "Press TAB when typing command name to auto-complete.\n");

  while (true)
  {
    char* line = linenoise(prompt);

    if (line == NULL)
      continue;

    if (strlen(line) > 0)
      linenoiseHistoryAdd(line);

    int ret;
    esp_err_t err = esp_console_run(line, &ret);

    if (err == ESP_ERR_NOT_FOUND)
      printf("Unrecognized command\n");
    else if (err == ESP_ERR_INVALID_ARG)
    {
    }
    else if (err == ESP_OK && ret != ESP_OK)
      printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
    else if (err != ESP_OK)
      printf("Internal error: %s\n", esp_err_to_name(err));

    linenoiseFree(line);
  }
}
