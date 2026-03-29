#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"

#include "esp_attr.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"

#include "ps2_mouse.h"

const gpio_num_t PS2_MOUSE_GPIO_DATA = GPIO_NUM_5;
const gpio_num_t PS2_MOUSE_GPIO_CLK  = GPIO_NUM_7;

enum
{
  PS2_TX_FIFO_SIZE = 64,

  PS2_CLK_LOW_US  = 18,
  PS2_CLK_HIGH_US = 22,
};

enum ps2_bus_mode_t
{
  PS2_BUS_IDLE = 0,
  PS2_BUS_DEV_TX,
  PS2_BUS_HOST_RX,
  PS2_BUS_HOST_ACK,
};

enum ps2_timer_evt_t
{
  PS2_TIMER_EVT_NONE = 0,
  PS2_TIMER_EVT_DEV_TX_HIGH,
  PS2_TIMER_EVT_DEV_TX_LOW,
  PS2_TIMER_EVT_DEV_TX_DONE,
  PS2_TIMER_EVT_HOST_RX_HIGH,
  PS2_TIMER_EVT_HOST_RX_LOW,
  PS2_TIMER_EVT_HOST_ACK_LOW,
  PS2_TIMER_EVT_HOST_ACK_HIGH,
  PS2_TIMER_EVT_HOST_ACK_RELEASE,
};

gptimer_handle_t ps2_mouse_timer = NULL;
portMUX_TYPE ps2_mouse_lock = portMUX_INITIALIZER_UNLOCKED;

volatile bool ps2_mouse_active = false;

volatile bool ps2_mouse_streaming = false;
volatile bool ps2_mouse_remote_mode = false;
volatile bool ps2_mouse_scaling21 = false;
volatile bool ps2_mouse_expect_param = false;
volatile uint8_t ps2_mouse_pending_cmd = 0;
volatile uint8_t ps2_mouse_sample_rate = 100;
volatile uint8_t ps2_mouse_resolution = 2;

volatile int ps2_mouse_dx_accum = 0;
volatile int ps2_mouse_dy_accum = 0;
volatile uint8_t ps2_mouse_buttons = 0;
volatile bool ps2_mouse_motion_pending = false;

volatile uint8_t ps2_mouse_tx_fifo[PS2_TX_FIFO_SIZE];
volatile unsigned ps2_mouse_tx_rd = 0;
volatile unsigned ps2_mouse_tx_wr = 0;
volatile unsigned ps2_mouse_tx_count = 0;

volatile ps2_bus_mode_t ps2_mouse_bus_mode = PS2_BUS_IDLE;
volatile ps2_timer_evt_t ps2_mouse_timer_evt = PS2_TIMER_EVT_NONE;

volatile uint8_t ps2_mouse_tx_byte = 0;
volatile uint8_t ps2_mouse_tx_parity = 1;
volatile int ps2_mouse_tx_bit = 0;

volatile uint8_t ps2_mouse_rx_byte = 0;
volatile uint8_t ps2_mouse_rx_parity = 1;
volatile int ps2_mouse_rx_bit = 0;
volatile bool ps2_mouse_rx_start_ok = false;
volatile bool ps2_mouse_rx_parity_ok = false;
volatile bool ps2_mouse_rx_stop_ok = false;
volatile uint8_t ps2_mouse_rx_done_byte = 0;
volatile bool ps2_mouse_rx_frame_ok = false;

volatile bool ps2_mouse_rts_seen = false;

void IRAM_ATTR ps2_mouse_clk_low()
{
  gpio_set_level(PS2_MOUSE_GPIO_CLK, 0);
}

void IRAM_ATTR ps2_mouse_clk_release()
{
  gpio_set_level(PS2_MOUSE_GPIO_CLK, 1);
}

void IRAM_ATTR ps2_mouse_data_low()
{
  gpio_set_level(PS2_MOUSE_GPIO_DATA, 0);
}

void IRAM_ATTR ps2_mouse_data_release()
{
  gpio_set_level(PS2_MOUSE_GPIO_DATA, 1);
}

int IRAM_ATTR ps2_mouse_clk_level()
{
  return gpio_get_level(PS2_MOUSE_GPIO_CLK);
}

int IRAM_ATTR ps2_mouse_data_level()
{
  return gpio_get_level(PS2_MOUSE_GPIO_DATA);
}

int IRAM_ATTR ps2_mouse_line_idle()
{
  return ps2_mouse_clk_level() && ps2_mouse_data_level();
}

void IRAM_ATTR ps2_mouse_timer_schedule_isr(ps2_timer_evt_t evt, uint32_t delta_us)
{
  uint64_t now = 0;
  gptimer_alarm_config_t alarm = {};

  if (!ps2_mouse_timer)
    return;

  gptimer_get_raw_count(ps2_mouse_timer, &now);

  ps2_mouse_timer_evt = evt;
  alarm.alarm_count = now + delta_us;
  alarm.flags.auto_reload_on_alarm = false;

  gptimer_set_alarm_action(ps2_mouse_timer, &alarm);
}

void ps2_mouse_timer_schedule_task(ps2_timer_evt_t evt, uint32_t delta_us)
{
  uint64_t now = 0;
  gptimer_alarm_config_t alarm = {};

  if (!ps2_mouse_timer)
    return;

  gptimer_get_raw_count(ps2_mouse_timer, &now);

  portENTER_CRITICAL(&ps2_mouse_lock);
  ps2_mouse_timer_evt = evt;
  portEXIT_CRITICAL(&ps2_mouse_lock);

  alarm.alarm_count = now + delta_us;
  alarm.flags.auto_reload_on_alarm = false;

  gptimer_set_alarm_action(ps2_mouse_timer, &alarm);
}

void IRAM_ATTR ps2_mouse_reset_protocol_state_isr()
{
  ps2_mouse_streaming = false;
  ps2_mouse_remote_mode = false;
  ps2_mouse_scaling21 = false;
  ps2_mouse_expect_param = false;
  ps2_mouse_pending_cmd = 0;
  ps2_mouse_sample_rate = 100;
  ps2_mouse_resolution = 2;

  ps2_mouse_dx_accum = 0;
  ps2_mouse_dy_accum = 0;
  ps2_mouse_buttons = 0;
  ps2_mouse_motion_pending = false;

  ps2_mouse_tx_rd = 0;
  ps2_mouse_tx_wr = 0;
  ps2_mouse_tx_count = 0;

  ps2_mouse_bus_mode = PS2_BUS_IDLE;
  ps2_mouse_timer_evt = PS2_TIMER_EVT_NONE;

  ps2_mouse_tx_byte = 0;
  ps2_mouse_tx_parity = 1;
  ps2_mouse_tx_bit = 0;

  ps2_mouse_rx_byte = 0;
  ps2_mouse_rx_parity = 1;
  ps2_mouse_rx_bit = 0;
  ps2_mouse_rx_start_ok = false;
  ps2_mouse_rx_parity_ok = false;
  ps2_mouse_rx_stop_ok = false;
  ps2_mouse_rx_done_byte = 0;
  ps2_mouse_rx_frame_ok = false;

  ps2_mouse_rts_seen = false;
}

int IRAM_ATTR ps2_mouse_tx_push_isr(uint8_t v)
{
  if (ps2_mouse_tx_count >= PS2_TX_FIFO_SIZE)
    return 0;

  ps2_mouse_tx_fifo[ps2_mouse_tx_wr] = v;
  ps2_mouse_tx_wr = ps2_mouse_tx_wr + 1;
  if (ps2_mouse_tx_wr >= PS2_TX_FIFO_SIZE)
    ps2_mouse_tx_wr = 0;

  ps2_mouse_tx_count = ps2_mouse_tx_count + 1;
  return 1;
}

int IRAM_ATTR ps2_mouse_tx_pop_isr(uint8_t *v)
{
  if (!ps2_mouse_tx_count)
    return 0;

  *v = ps2_mouse_tx_fifo[ps2_mouse_tx_rd];
  ps2_mouse_tx_rd = ps2_mouse_tx_rd + 1;
  if (ps2_mouse_tx_rd >= PS2_TX_FIFO_SIZE)
    ps2_mouse_tx_rd = 0;

  ps2_mouse_tx_count = ps2_mouse_tx_count - 1;
  return 1;
}

int IRAM_ATTR ps2_mouse_clamp_9bit(int v)
{
  if (v > 255)
    return 255;
  if (v < -255)
    return -255;
  return v;
}

void IRAM_ATTR ps2_mouse_build_packet_isr(uint8_t packet[3], bool clear_motion)
{
  int dx = ps2_mouse_clamp_9bit(ps2_mouse_dx_accum);
  int dy = ps2_mouse_clamp_9bit(ps2_mouse_dy_accum);

  packet[0] = 0x08 | (ps2_mouse_buttons & 0x07);

  if (dx < 0)
    packet[0] |= 0x10;

  if (dy < 0)
    packet[0] |= 0x20;

  if (dx != ps2_mouse_dx_accum)
    packet[0] |= 0x40;

  if (dy != ps2_mouse_dy_accum)
    packet[0] |= 0x80;

  packet[1] = (uint8_t)(dx & 0xFF);
  packet[2] = (uint8_t)(dy & 0xFF);

  if (clear_motion)
  {
    ps2_mouse_dx_accum = 0;
    ps2_mouse_dy_accum = 0;
    ps2_mouse_motion_pending = false;
  }
}

void IRAM_ATTR ps2_mouse_queue_motion_if_possible_isr()
{
  uint8_t packet[3];

  if (!ps2_mouse_streaming)
    return;

  if (ps2_mouse_remote_mode)
    return;

  if (!ps2_mouse_motion_pending)
    return;

  if (ps2_mouse_tx_count > (PS2_TX_FIFO_SIZE - 3))
    return;

  ps2_mouse_build_packet_isr(packet, true);

  ps2_mouse_tx_push_isr(packet[0]);
  ps2_mouse_tx_push_isr(packet[1]);
  ps2_mouse_tx_push_isr(packet[2]);
}

int IRAM_ATTR ps2_mouse_get_tx_bit_isr(int bit_index)
{
  if (bit_index == 0)
    return 0;

  if (bit_index >= 1 && bit_index <= 8)
    return (ps2_mouse_tx_byte >> (bit_index - 1)) & 1;

  if (bit_index == 9)
    return ps2_mouse_tx_parity;

  return 1;
}

void IRAM_ATTR ps2_mouse_drive_tx_bit_isr()
{
  if (ps2_mouse_get_tx_bit_isr(ps2_mouse_tx_bit))
    ps2_mouse_data_release();
  else
    ps2_mouse_data_low();
}

void IRAM_ATTR ps2_mouse_try_start_tx_isr()
{
  uint8_t v = 0;

  if (!ps2_mouse_active)
    return;

  if (ps2_mouse_bus_mode != PS2_BUS_IDLE)
    return;

  ps2_mouse_queue_motion_if_possible_isr();

  if (!ps2_mouse_tx_count)
    return;

  if (!ps2_mouse_line_idle())
    return;

  if (!ps2_mouse_tx_pop_isr(&v))
    return;

  ps2_mouse_tx_byte = v;
  ps2_mouse_tx_parity = 1;
  for (int i = 0; i < 8; i++)
    ps2_mouse_tx_parity ^= (uint8_t)((v >> i) & 1);

  ps2_mouse_tx_bit = 0;
  ps2_mouse_bus_mode = PS2_BUS_DEV_TX;

  ps2_mouse_drive_tx_bit_isr();
  ps2_mouse_clk_low();
  ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_HIGH, PS2_CLK_LOW_US);
}

void IRAM_ATTR ps2_mouse_start_host_rx_isr()
{
  if (!ps2_mouse_active)
    return;

  if (ps2_mouse_bus_mode != PS2_BUS_IDLE)
    return;

  ps2_mouse_bus_mode = PS2_BUS_HOST_RX;

  ps2_mouse_rx_byte = 0;
  ps2_mouse_rx_parity = 1;
  ps2_mouse_rx_bit = 0;
  ps2_mouse_rx_start_ok = false;
  ps2_mouse_rx_parity_ok = false;
  ps2_mouse_rx_stop_ok = false;
  ps2_mouse_rx_done_byte = 0;
  ps2_mouse_rx_frame_ok = false;

  ps2_mouse_clk_low();
  ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_HIGH, PS2_CLK_LOW_US);
}

void IRAM_ATTR ps2_mouse_handle_cmd_isr(uint8_t value)
{
  uint8_t packet[3];
  uint8_t status = 0;

  if (ps2_mouse_expect_param)
  {
    ps2_mouse_expect_param = false;

    if (ps2_mouse_pending_cmd == 0xF3)
      ps2_mouse_sample_rate = value;

    if (ps2_mouse_pending_cmd == 0xE8)
      ps2_mouse_resolution = value & 0x03;

    ps2_mouse_pending_cmd = 0;
    ps2_mouse_tx_push_isr(0xFA);
    ps2_mouse_try_start_tx_isr();
    return;
  }

  switch (value)
  {
    case 0xFF:
      ps2_mouse_reset_protocol_state_isr();
      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_tx_push_isr(0xAA);
      ps2_mouse_tx_push_isr(0x00);
    break;

    case 0xF6:
      ps2_mouse_reset_protocol_state_isr();
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xF5:
      ps2_mouse_streaming = false;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xF4:
      ps2_mouse_streaming = true;
      ps2_mouse_remote_mode = false;
      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_queue_motion_if_possible_isr();
    break;

    case 0xF2:
      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_tx_push_isr(0x00);
    break;

    case 0xEB:
      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_build_packet_isr(packet, true);
      ps2_mouse_tx_push_isr(packet[0]);
      ps2_mouse_tx_push_isr(packet[1]);
      ps2_mouse_tx_push_isr(packet[2]);
    break;

    case 0xEA:
      ps2_mouse_remote_mode = false;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xF0:
      ps2_mouse_remote_mode = true;
      ps2_mouse_streaming = false;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xE6:
      ps2_mouse_scaling21 = false;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xE7:
      ps2_mouse_scaling21 = true;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xE9:
      if (ps2_mouse_scaling21)
        status |= 0x10;
      if (ps2_mouse_streaming)
        status |= 0x20;
      if (ps2_mouse_remote_mode)
        status |= 0x40;
      status |= (ps2_mouse_buttons & 0x07);

      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_tx_push_isr(status);
      ps2_mouse_tx_push_isr(ps2_mouse_resolution);
      ps2_mouse_tx_push_isr(ps2_mouse_sample_rate);
    break;

    case 0xF3:
      ps2_mouse_expect_param = true;
      ps2_mouse_pending_cmd = 0xF3;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    case 0xE8:
      ps2_mouse_expect_param = true;
      ps2_mouse_pending_cmd = 0xE8;
      ps2_mouse_tx_push_isr(0xFA);
    break;

    default:
      ps2_mouse_tx_push_isr(0xFE);
    break;
  }

  ps2_mouse_try_start_tx_isr();
}

bool IRAM_ATTR ps2_mouse_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  if (timer)
  {
  }

  if (edata)
  {
  }

  if (user_ctx)
  {
  }

  portENTER_CRITICAL_ISR(&ps2_mouse_lock);

  if (!ps2_mouse_active)
  {
    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return false;
  }

  switch (ps2_mouse_timer_evt)
  {
    case PS2_TIMER_EVT_DEV_TX_HIGH:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_LOW, PS2_CLK_HIGH_US);
      }
    break;

    case PS2_TIMER_EVT_DEV_TX_LOW:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        ps2_mouse_clk_low();
      }
    break;

    case PS2_TIMER_EVT_DEV_TX_DONE:
      ps2_mouse_data_release();
      ps2_mouse_clk_release();
      ps2_mouse_bus_mode = PS2_BUS_IDLE;
      ps2_mouse_try_start_tx_isr();
    break;

    case PS2_TIMER_EVT_HOST_RX_HIGH:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
      {
        ps2_mouse_clk_release();
      }
    break;

    case PS2_TIMER_EVT_HOST_RX_LOW:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
      {
        ps2_mouse_clk_low();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_HIGH, PS2_CLK_LOW_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_LOW:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        ps2_mouse_clk_low();
        ps2_mouse_data_low();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_HIGH, PS2_CLK_LOW_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_HIGH:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_RELEASE, PS2_CLK_HIGH_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_RELEASE:
      ps2_mouse_data_release();
      ps2_mouse_clk_release();
      ps2_mouse_bus_mode = PS2_BUS_IDLE;

      if (ps2_mouse_rx_frame_ok)
        ps2_mouse_handle_cmd_isr(ps2_mouse_rx_done_byte);
      else
        ps2_mouse_try_start_tx_isr();
    break;

    default:
    break;
  }

  portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
  return false;
}

void IRAM_ATTR ps2_mouse_clk_isr(void *arg)
{
  int clk;
  int dat;

  if (arg)
  {
  }

  portENTER_CRITICAL_ISR(&ps2_mouse_lock);

  if (!ps2_mouse_active)
  {
    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return;
  }

  clk = ps2_mouse_clk_level();
  dat = ps2_mouse_data_level();

  if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
  {
    if (!clk)
    {
      ps2_mouse_tx_bit = ps2_mouse_tx_bit + 1;

      if (ps2_mouse_tx_bit <= 10)
      {
        ps2_mouse_drive_tx_bit_isr();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_HIGH, PS2_CLK_LOW_US);
      }
      else
      {
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_DONE, PS2_CLK_LOW_US);
      }
    }

    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return;
  }

  if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
  {
    if (clk)
    {
      if (ps2_mouse_rx_bit == 0)
      {
        ps2_mouse_rx_start_ok = (dat == 0);
      }
      else if (ps2_mouse_rx_bit >= 1 && ps2_mouse_rx_bit <= 8)
      {
        if (dat)
          ps2_mouse_rx_byte |= (uint8_t)(1U << (ps2_mouse_rx_bit - 1));

        ps2_mouse_rx_parity ^= (uint8_t)dat;
      }
      else if (ps2_mouse_rx_bit == 9)
      {
        ps2_mouse_rx_parity_ok = ((uint8_t)dat == ps2_mouse_rx_parity);
      }
      else if (ps2_mouse_rx_bit == 10)
      {
        ps2_mouse_rx_stop_ok = (dat == 1);
        ps2_mouse_rx_done_byte = ps2_mouse_rx_byte;
        ps2_mouse_rx_frame_ok = ps2_mouse_rx_start_ok && ps2_mouse_rx_parity_ok && ps2_mouse_rx_stop_ok;
      }

      ps2_mouse_rx_bit = ps2_mouse_rx_bit + 1;

      if (ps2_mouse_rx_bit <= 10)
      {
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_LOW, PS2_CLK_HIGH_US);
      }
      else
      {
        ps2_mouse_bus_mode = PS2_BUS_HOST_ACK;
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_LOW, PS2_CLK_HIGH_US);
      }
    }

    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return;
  }

  if (ps2_mouse_bus_mode == PS2_BUS_IDLE)
  {
    if (!clk)
    {
      ps2_mouse_rts_seen = true;
    }
    else
    {
      if (ps2_mouse_rts_seen && !dat)
      {
        ps2_mouse_rts_seen = false;
        ps2_mouse_start_host_rx_isr();
        portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
        return;
      }

      ps2_mouse_rts_seen = false;
    }
  }

  portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
}

esp_err_t ps2_mouse_init()
{
  esp_err_t err;
  gpio_config_t io = {};
  gptimer_config_t tcfg = {};
  gptimer_event_callbacks_t tcbs = {};

  io.pin_bit_mask = (1ULL << PS2_MOUSE_GPIO_DATA) | (1ULL << PS2_MOUSE_GPIO_CLK);
  io.mode = GPIO_MODE_INPUT_OUTPUT_OD;
  io.pull_up_en = GPIO_PULLUP_ENABLE;
  io.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io.intr_type = GPIO_INTR_DISABLE;

  err = gpio_config(&io);
  if (err != ESP_OK)
    return err;

  err = gpio_set_pull_mode(PS2_MOUSE_GPIO_DATA, GPIO_PULLUP_ONLY);
  if (err != ESP_OK)
    return err;

  err = gpio_set_pull_mode(PS2_MOUSE_GPIO_CLK, GPIO_PULLUP_ONLY);
  if (err != ESP_OK)
    return err;

  ps2_mouse_data_release();
  ps2_mouse_clk_release();

  tcfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  tcfg.direction = GPTIMER_COUNT_UP;
  tcfg.resolution_hz = 1000000;

  err = gptimer_new_timer(&tcfg, &ps2_mouse_timer);
  if (err != ESP_OK)
    return err;

  tcbs.on_alarm = ps2_mouse_timer_cb;

  err = gptimer_register_event_callbacks(ps2_mouse_timer, &tcbs, NULL);
  if (err != ESP_OK)
    return err;

  err = gptimer_enable(ps2_mouse_timer);
  if (err != ESP_OK)
    return err;

  err = gptimer_start(ps2_mouse_timer);
  if (err != ESP_OK)
    return err;

  err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    return err;

  err = gpio_set_intr_type(PS2_MOUSE_GPIO_CLK, GPIO_INTR_ANYEDGE);
  if (err != ESP_OK)
    return err;

  err = gpio_isr_handler_add(PS2_MOUSE_GPIO_CLK, ps2_mouse_clk_isr, NULL);
  if (err != ESP_OK)
    return err;

  return ESP_OK;
}

void ps2_mouse_deinit()
{
  gpio_intr_disable(PS2_MOUSE_GPIO_CLK);
  gpio_isr_handler_remove(PS2_MOUSE_GPIO_CLK);

  ps2_mouse_data_release();
  ps2_mouse_clk_release();

  if (ps2_mouse_timer)
  {
    gptimer_stop(ps2_mouse_timer);
    gptimer_disable(ps2_mouse_timer);
    gptimer_del_timer(ps2_mouse_timer);
    ps2_mouse_timer = NULL;
  }
}

bool ps2_mouse_is_active()
{
  return ps2_mouse_active;
}

esp_err_t ps2_mouse_send_movement(int dx, int dy, unsigned buttons)
{
  if (!ps2_mouse_active)
    return ESP_ERR_INVALID_STATE;

  portENTER_CRITICAL(&ps2_mouse_lock);

  ps2_mouse_dx_accum += dx;
  ps2_mouse_dy_accum += dy;
  ps2_mouse_buttons = (uint8_t)(buttons & 0x07);
  ps2_mouse_motion_pending = true;

  ps2_mouse_queue_motion_if_possible_isr();
  ps2_mouse_try_start_tx_isr();

  portEXIT_CRITICAL(&ps2_mouse_lock);

  return ESP_OK;
}

int ps2_mouse_parse_i32(const char *s, int *out)
{
  char *endp = NULL;
  long v;

  if (!s || !out)
    return 0;

  v = strtol(s, &endp, 0);
  if (!endp || *endp)
    return 0;

  *out = (int)v;
  return 1;
}

int ps2_mouse_cmd(int argc, char **argv)
{
  esp_err_t err;

  if (argc < 2 || !strcmp(argv[1], "start"))
  {
    if (ps2_mouse_active)
    {
      printf("ps/2 mouse already active\r\n");
      return 0;
    }

    portENTER_CRITICAL(&ps2_mouse_lock);
    ps2_mouse_active = true;
    ps2_mouse_reset_protocol_state_isr();
    portEXIT_CRITICAL(&ps2_mouse_lock);

    err = ps2_mouse_init();
    if (err != ESP_OK)
    {
      ps2_mouse_active = false;
      ps2_mouse_deinit();
      printf("ps/2 mouse init failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    printf("ps/2 mouse started on GPIO5=data, GPIO7=clk\r\n");
    printf("commands: ps2mouse stop | ps2mouse move <dx> <dy> [buttons]\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "stop"))
  {
    if (!ps2_mouse_active)
    {
      printf("ps/2 mouse is not active\r\n");
      return 0;
    }

    portENTER_CRITICAL(&ps2_mouse_lock);
    ps2_mouse_active = false;
    portEXIT_CRITICAL(&ps2_mouse_lock);

    ps2_mouse_deinit();
    printf("ps/2 mouse stopped\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "move"))
  {
    int dx = 0;
    int dy = 0;
    int buttons = 0;

    if (argc < 4)
    {
      printf("Usage: ps2mouse move <dx> <dy> [buttons]\r\n");
      return 1;
    }

    if (!ps2_mouse_parse_i32(argv[2], &dx) || !ps2_mouse_parse_i32(argv[3], &dy))
    {
      printf("Invalid dx/dy\r\n");
      return 1;
    }

    if (argc >= 5 && !ps2_mouse_parse_i32(argv[4], &buttons))
    {
      printf("Invalid buttons\r\n");
      return 1;
    }

    err = ps2_mouse_send_movement(dx, dy, (unsigned)buttons);
    if (err != ESP_OK)
    {
      printf("ps/2 mouse is not active\r\n");
      return 1;
    }

    printf("queued dx=%d dy=%d buttons=%d\r\n", dx, dy, buttons & 7);
    return 0;
  }

  printf("Usage:\r\n");
  printf("  ps2mouse start\r\n");
  printf("  ps2mouse stop\r\n");
  printf("  ps2mouse move <dx> <dy> [buttons]\r\n");
  printf("buttons bitmask: 1=left 2=right 4=middle\r\n");
  return 1;
}
