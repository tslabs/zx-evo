#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "main.h"
#include "esp_err.h"
#include "xm.h"

enum
{
  XM_TASK_INIT,
  XM_TASK_PLAY,
  XM_TASK_STOP,
  XM_TASK_SET_POS,
};

typedef struct
{
  u8 task;
  int handle;
} XM_TASK;

typedef struct
{
  u8 task;
  xm_context_t *ctx;
} PLAYER_TASK;

extern QueueHandle_t xm_queue;
extern int master_volume;
extern int curr_xm_handle;

int xm_cmd(int argc, char **argv);
void xm_task(void *arg);
void initialize_xm();
void xm_console_register_system_commands();
int xm_load_play_file(const char *path, bool quiet = false);
int xm_play_cmd(int handle, bool quiet = false);
int xm_stop_cmd(bool quiet = false);
esp_err_t xm_host_stream_start(size_t module_size);
void xm_host_stream_process_rx_data(const u8 *data, size_t size);
