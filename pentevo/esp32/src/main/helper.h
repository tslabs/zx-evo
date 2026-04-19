
#pragma once

enum
{
  TASK_MAKE_OBJ,
  TASK_DEL_OBJ,
  TASK_KILL_OBJ,
  TASK_LOAD_ELF,
  TASK_LOAD_ELF_OPT,
  TASK_RUN_FUNC0,
  TASK_RUN_FUNC1,
  TASK_RUN_FUNC2,
  TASK_RUN_FUNC3,
  TASK_DEHST,
  TASK_UNZIP,
  TASK_RESET_STREAM_UNZIP,
  TASK_WSCAN,
  TASK_CONN,
  TASK_DISCONN,
  TASK_HTTP_GET,
  TASK_HTTPS_GET,
  TASK_GOPHER_GET,
  TASK_HTTP_STREAM_START,
  TASK_HTTPS_STREAM_START,
  TASK_GOPHER_STREAM_START,
  TASK_STREAM_READ,
  TASK_STREAM_CLOSE,
  TASK_HTTP_STREAM_READ,
  TASK_REBOOT,
  TASK_RESET,
};

#pragma pack(1)
struct NET
{
  bool is_init = false;
  bool is_busy = false;
  u8 state;

  u8 ssid[33];
  u8 pwd[62];
  u8 *url;

  struct
  {
    u8 ip[4];
    u8 own_ip[4];
    u8 mask[4];
    u8 gate[4];
  } ip;
};
#pragma pack()

extern QueueHandle_t helper_queue;
extern struct NET net;

void put_txq(int type);
void put_txq_isr(int type);
void put_rxq(int type);
void put_rxq_isr(int type);
void put_helper_isr(int task);

void helper_task(void *arg);

u32 prepare_tx_data(u8 type, size_t size);
void process_rx_data(u8 type, size_t size);
