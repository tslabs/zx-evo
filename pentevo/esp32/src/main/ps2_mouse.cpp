#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
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

  PS2_CLK_LOW_US = 35,
  PS2_CLK_HIGH_US = 35,
  PS2_TX_HIGH_HOLD_US = 15,
  PS2_TX_SETUP_US = 20,
  PS2_TX_INTERBYTE_US = 80,
  PS2_HOST_RX_START_US = 30,
  PS2_HOST_RX_SAMPLE_US = 18,
  PS2_HOST_ACK_SETUP_US = 15,
  PS2_HOST_INHIBIT_US = 120,
  PS2_TX_RETRY_US = 200,
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
  PS2_TIMER_EVT_DEV_TX_FALL,
  PS2_TIMER_EVT_DEV_TX_RISE,
  PS2_TIMER_EVT_DEV_TX_NEXT,
  PS2_TIMER_EVT_DEV_TX_DONE,
  PS2_TIMER_EVT_HOST_RX_FALL,
  PS2_TIMER_EVT_HOST_RX_RISE,
  PS2_TIMER_EVT_HOST_RX_SAMPLE,
  PS2_TIMER_EVT_HOST_ACK_PREPARE,
  PS2_TIMER_EVT_HOST_ACK_FALL,
  PS2_TIMER_EVT_HOST_ACK_RISE,
  PS2_TIMER_EVT_HOST_ACK_DONE,
  PS2_TIMER_EVT_TX_RETRY,
};

gptimer_handle_t ps2_mouse_timer = NULL;
portMUX_TYPE ps2_mouse_lock = portMUX_INITIALIZER_UNLOCKED;

bool ps2_mouse_gpio_isr_service_ready = false;

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
volatile bool ps2_mouse_tx_resume_valid = false;
volatile uint8_t ps2_mouse_tx_resume_byte = 0;

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

volatile uint32_t ps2_mouse_clk_low_since_us = 0;

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

uint32_t IRAM_ATTR ps2_mouse_get_time_us_isr()
{
  uint64_t now = 0;

  if (!ps2_mouse_timer)
    return 0;

  gptimer_get_raw_count(ps2_mouse_timer, &now);
  return (uint32_t)now;
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

void IRAM_ATTR ps2_mouse_reset_device_state_isr()
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
}

void IRAM_ATTR ps2_mouse_reset_protocol_state_isr()
{
  ps2_mouse_reset_device_state_isr();

  ps2_mouse_tx_rd = 0;
  ps2_mouse_tx_wr = 0;
  ps2_mouse_tx_count = 0;
  ps2_mouse_tx_resume_valid = false;
  ps2_mouse_tx_resume_byte = 0;

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

  ps2_mouse_clk_low_since_us = 0;
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
  if (ps2_mouse_tx_resume_valid)
  {
    *v = ps2_mouse_tx_resume_byte;
    ps2_mouse_tx_resume_valid = false;
    return 1;
  }

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

void IRAM_ATTR ps2_mouse_drive_tx_bit_isr(int bit_index)
{
  if (ps2_mouse_get_tx_bit_isr(bit_index))
    ps2_mouse_data_release();
  else
    ps2_mouse_data_low();
}

void IRAM_ATTR ps2_mouse_prepare_tx_byte_isr(uint8_t v)
{
  ps2_mouse_tx_byte = v;
  ps2_mouse_tx_parity = 1;
  for (int i = 0; i < 8; i++)
    ps2_mouse_tx_parity ^= (uint8_t)((v >> i) & 1);

  ps2_mouse_tx_bit = 0;
  ps2_mouse_bus_mode = PS2_BUS_DEV_TX;

  ps2_mouse_clk_release();
  ps2_mouse_drive_tx_bit_isr(0);
  ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_FALL, PS2_TX_SETUP_US);
}

int IRAM_ATTR ps2_mouse_host_inhibit_active_isr()
{
  uint32_t now;

  if (ps2_mouse_clk_level())
    return 0;

  now = ps2_mouse_get_time_us_isr();
  if ((uint32_t)(now - ps2_mouse_clk_low_since_us) < PS2_HOST_INHIBIT_US)
    return 0;

  return 1;
}

void IRAM_ATTR ps2_mouse_dev_tx_abort_isr()
{
  if (ps2_mouse_bus_mode != PS2_BUS_DEV_TX)
    return;

  ps2_mouse_tx_resume_valid = true;
  ps2_mouse_tx_resume_byte = ps2_mouse_tx_byte;
  ps2_mouse_data_release();
  ps2_mouse_clk_release();
  ps2_mouse_bus_mode = PS2_BUS_IDLE;
  ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_TX_RETRY, PS2_TX_RETRY_US);
}

void IRAM_ATTR ps2_mouse_try_start_tx_isr()
{
  uint8_t v = 0;

  if (!ps2_mouse_active)
    return;

  if (ps2_mouse_bus_mode != PS2_BUS_IDLE)
    return;

  ps2_mouse_queue_motion_if_possible_isr();

  if (!ps2_mouse_tx_resume_valid && !ps2_mouse_tx_count)
    return;

  if (!ps2_mouse_line_idle())
  {
    ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_TX_RETRY, PS2_TX_RETRY_US);
    return;
  }

  if (!ps2_mouse_tx_pop_isr(&v))
    return;

  ps2_mouse_prepare_tx_byte_isr(v);
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
  ps2_mouse_rx_start_ok = true;
  ps2_mouse_rx_parity_ok = false;
  ps2_mouse_rx_stop_ok = false;
  ps2_mouse_rx_done_byte = 0;
  ps2_mouse_rx_frame_ok = false;

  ps2_mouse_data_release();
  ps2_mouse_clk_release();
  ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_FALL, PS2_HOST_RX_START_US);
}

void IRAM_ATTR ps2_mouse_host_rx_sample_isr()
{
  int dat = ps2_mouse_data_level();

  if (ps2_mouse_rx_bit >= 0 && ps2_mouse_rx_bit <= 7)
  {
    if (dat)
      ps2_mouse_rx_byte |= (uint8_t)(1U << ps2_mouse_rx_bit);

    ps2_mouse_rx_parity ^= (uint8_t)dat;
  }
  else if (ps2_mouse_rx_bit == 8)
  {
    ps2_mouse_rx_parity_ok = ((uint8_t)dat == ps2_mouse_rx_parity);
  }
  else if (ps2_mouse_rx_bit == 9)
  {
    ps2_mouse_rx_stop_ok = (dat == 1);
    ps2_mouse_rx_done_byte = ps2_mouse_rx_byte;
    ps2_mouse_rx_frame_ok = ps2_mouse_rx_start_ok && ps2_mouse_rx_parity_ok && ps2_mouse_rx_stop_ok;
  }

  ps2_mouse_rx_bit = ps2_mouse_rx_bit + 1;
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
      ps2_mouse_reset_device_state_isr();
      ps2_mouse_tx_push_isr(0xFA);
      ps2_mouse_tx_push_isr(0xAA);
      ps2_mouse_tx_push_isr(0x00);
    break;

    case 0xF6:
      ps2_mouse_reset_device_state_isr();
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

bool IRAM_ATTR ps2_mouse_timer_cb(gptimer_handle_t, const gptimer_alarm_event_data_t *, void *)
{
  portENTER_CRITICAL_ISR(&ps2_mouse_lock);

  if (!ps2_mouse_active)
  {
    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return false;
  }

  switch (ps2_mouse_timer_evt)
  {
    case PS2_TIMER_EVT_DEV_TX_FALL:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        if (ps2_mouse_host_inhibit_active_isr())
        {
          ps2_mouse_dev_tx_abort_isr();
          break;
        }

        ps2_mouse_clk_low();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_RISE, PS2_CLK_LOW_US);
      }
    break;

    case PS2_TIMER_EVT_DEV_TX_RISE:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_NEXT, PS2_TX_HIGH_HOLD_US);
      }
    break;

    case PS2_TIMER_EVT_DEV_TX_NEXT:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        if (ps2_mouse_host_inhibit_active_isr())
        {
          ps2_mouse_dev_tx_abort_isr();
          break;
        }

        ps2_mouse_tx_bit = ps2_mouse_tx_bit + 1;
        if (ps2_mouse_tx_bit <= 10)
        {
          ps2_mouse_drive_tx_bit_isr(ps2_mouse_tx_bit);
          ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_FALL, PS2_TX_SETUP_US);
        }
        else
        {
          ps2_mouse_data_release();
          ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_DEV_TX_DONE, PS2_TX_SETUP_US);
        }
      }
    break;

    case PS2_TIMER_EVT_DEV_TX_DONE:
      if (ps2_mouse_bus_mode == PS2_BUS_DEV_TX)
      {
        ps2_mouse_data_release();
        ps2_mouse_clk_release();
        ps2_mouse_bus_mode = PS2_BUS_IDLE;
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_TX_RETRY, PS2_TX_INTERBYTE_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_RX_FALL:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
      {
        ps2_mouse_data_release();
        ps2_mouse_clk_low();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_RISE, PS2_CLK_LOW_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_RX_RISE:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
      {
        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_SAMPLE, PS2_HOST_RX_SAMPLE_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_RX_SAMPLE:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_RX)
      {
        ps2_mouse_host_rx_sample_isr();

        if (ps2_mouse_rx_bit <= 9)
        {
          ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_RX_FALL, PS2_CLK_HIGH_US - PS2_HOST_RX_SAMPLE_US);
        }
        else
        {
          ps2_mouse_bus_mode = PS2_BUS_HOST_ACK;
          ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_PREPARE, PS2_CLK_HIGH_US - PS2_HOST_RX_SAMPLE_US);
        }
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_PREPARE:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        if (ps2_mouse_rx_frame_ok)
          ps2_mouse_data_low();
        else
          ps2_mouse_data_release();

        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_FALL, PS2_HOST_ACK_SETUP_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_FALL:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        ps2_mouse_clk_low();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_RISE, PS2_CLK_LOW_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_RISE:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        ps2_mouse_clk_release();
        ps2_mouse_timer_schedule_isr(PS2_TIMER_EVT_HOST_ACK_DONE, PS2_CLK_HIGH_US);
      }
    break;

    case PS2_TIMER_EVT_HOST_ACK_DONE:
      if (ps2_mouse_bus_mode == PS2_BUS_HOST_ACK)
      {
        ps2_mouse_data_release();
        ps2_mouse_clk_release();
        ps2_mouse_bus_mode = PS2_BUS_IDLE;

        if (ps2_mouse_rx_frame_ok)
          ps2_mouse_handle_cmd_isr(ps2_mouse_rx_done_byte);
        else
          ps2_mouse_try_start_tx_isr();
      }
    break;

    case PS2_TIMER_EVT_TX_RETRY:
      if (ps2_mouse_bus_mode == PS2_BUS_IDLE)
        ps2_mouse_try_start_tx_isr();
    break;

    default:
    break;
  }

  portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
  return false;
}

void IRAM_ATTR ps2_mouse_clk_isr(void *)
{
  int clk;
  int dat;
  uint32_t now;
  uint32_t low_time;

  portENTER_CRITICAL_ISR(&ps2_mouse_lock);

  if (!ps2_mouse_active)
  {
    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return;
  }

  clk = ps2_mouse_clk_level();
  dat = ps2_mouse_data_level();
  now = ps2_mouse_get_time_us_isr();

  if (!clk)
  {
    ps2_mouse_clk_low_since_us = now;
    portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
    return;
  }

  low_time = now - ps2_mouse_clk_low_since_us;

  if (ps2_mouse_bus_mode == PS2_BUS_IDLE)
  {
    if (low_time >= PS2_HOST_INHIBIT_US && !dat)
    {
      ps2_mouse_start_host_rx_isr();
      portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
      return;
    }
  }

  portEXIT_CRITICAL_ISR(&ps2_mouse_lock);
}

void IRAM_ATTR ps2_mouse_data_isr(void *)
{
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
  if (err != ESP_OK) return err;

  err = gpio_set_pull_mode(PS2_MOUSE_GPIO_DATA, GPIO_PULLUP_ONLY);
  if (err != ESP_OK) return err;

  err = gpio_set_pull_mode(PS2_MOUSE_GPIO_CLK, GPIO_PULLUP_ONLY);
  if (err != ESP_OK) return err;

  ps2_mouse_data_release();
  ps2_mouse_clk_release();

  tcfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  tcfg.direction = GPTIMER_COUNT_UP;
  tcfg.resolution_hz = 1000000;

  err = gptimer_new_timer(&tcfg, &ps2_mouse_timer);
  if (err != ESP_OK) return err;

  tcbs.on_alarm = ps2_mouse_timer_cb;

  err = gptimer_register_event_callbacks(ps2_mouse_timer, &tcbs, NULL);
  if (err != ESP_OK) return err;

  err = gptimer_enable(ps2_mouse_timer);
  if (err != ESP_OK) return err;

  err = gptimer_start(ps2_mouse_timer);
  if (err != ESP_OK) return err;

  if (!ps2_mouse_gpio_isr_service_ready)
  {
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (err == ESP_ERR_INVALID_STATE)
      ps2_mouse_gpio_isr_service_ready = true;
    else if (err != ESP_OK)
      return err;
    else
      ps2_mouse_gpio_isr_service_ready = true;
  }

  err = gpio_set_intr_type(PS2_MOUSE_GPIO_CLK, GPIO_INTR_ANYEDGE);
  if (err != ESP_OK) return err;

  err = gpio_set_intr_type(PS2_MOUSE_GPIO_DATA, GPIO_INTR_ANYEDGE);
  if (err != ESP_OK) return err;

  err = gpio_isr_handler_add(PS2_MOUSE_GPIO_CLK, ps2_mouse_clk_isr, NULL);
  if (err != ESP_OK) return err;

  err = gpio_isr_handler_add(PS2_MOUSE_GPIO_DATA, ps2_mouse_data_isr, NULL);
  if (err != ESP_OK) return err;

  ps2_mouse_clk_low_since_us = 0;

  return ESP_OK;
}

void ps2_mouse_deinit()
{
  gpio_intr_disable(PS2_MOUSE_GPIO_CLK);
  gpio_intr_disable(PS2_MOUSE_GPIO_DATA);
  gpio_isr_handler_remove(PS2_MOUSE_GPIO_CLK);
  gpio_isr_handler_remove(PS2_MOUSE_GPIO_DATA);

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

  if (!ps2_mouse_streaming || ps2_mouse_remote_mode)
  {
    portEXIT_CRITICAL(&ps2_mouse_lock);
    return ESP_ERR_INVALID_STATE;
  }

  ps2_mouse_dx_accum += dx;
  ps2_mouse_dy_accum += dy;
  ps2_mouse_buttons = (uint8_t)(buttons & 0x07);
  ps2_mouse_motion_pending = true;

  ps2_mouse_queue_motion_if_possible_isr();
  ps2_mouse_try_start_tx_isr();

  portEXIT_CRITICAL(&ps2_mouse_lock);
  return ESP_OK;
}

esp_err_t ps2_mouse_start()
{
  esp_err_t err;

  if (ps2_mouse_active)
    return ESP_OK;

  portENTER_CRITICAL(&ps2_mouse_lock);
  ps2_mouse_active = true;
  ps2_mouse_reset_protocol_state_isr();
  portEXIT_CRITICAL(&ps2_mouse_lock);

  err = ps2_mouse_init();
  if (err != ESP_OK)
  {
    portENTER_CRITICAL(&ps2_mouse_lock);
    ps2_mouse_active = false;
    portEXIT_CRITICAL(&ps2_mouse_lock);
    ps2_mouse_deinit();
    return err;
  }

  portENTER_CRITICAL(&ps2_mouse_lock);
  ps2_mouse_streaming = true;
  ps2_mouse_remote_mode = false;
  ps2_mouse_expect_param = false;
  ps2_mouse_pending_cmd = 0;
  portEXIT_CRITICAL(&ps2_mouse_lock);

  ESP_LOGI("ps2_mouse", "ps/2 mouse started on GPIO%u=data, GPIO%u=clk, streaming enabled", PS2_MOUSE_GPIO_DATA, PS2_MOUSE_GPIO_CLK);

  return ESP_OK;
}
