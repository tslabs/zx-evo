
#pragma once

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

