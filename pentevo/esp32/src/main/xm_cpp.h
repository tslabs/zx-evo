
#pragma once

enum
{
  XM_TASK_INIT,
  XM_TASK_SILENCE,
  XM_TASK_RENDER,
  XM_TASK_STOP,
};

typedef struct
{
  u8 id;
  void *arg;
} XM_TASK;

void xm_task(void *arg);
void initialize_xm();

extern QueueHandle_t xm_queue;
