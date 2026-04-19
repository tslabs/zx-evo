
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "miniz.h"
#include "spi_slave.h"
#include "mem_obj.h"
#include "elf.cpp.h"
#include "depack.h"

#ifdef CONFIG_ESP32_WIFI_ENABLED
#include "wifi.h"
#endif

#include "helper.h"
#include "http_client.h"
#include "gopher_client.h"
#include "stream_client.h"

const char TAG[] = "helper";

EXT_RAM_BSS_ATTR u8 url_buf[1024];

QueueHandle_t helper_queue;

NET net;

#include <stdio.h>
#include <string.h>
#include "miniz.h"
#include "esp_log.h"

void IRAM_ATTR put_helper_isr(int task)
{
  xQueueSendFromISR(helper_queue, &task, 0);
  portYIELD_FROM_ISR();
}

void *f_malloc(size_t x)
{
#ifdef VERBOSE
  printf("f_malloc %u\r\n", x);
#endif

  return malloc_spiram(x);
}

void f_free(void *p)
{
#ifdef VERBOSE
  printf("f_free\r\n");
#endif

  free(p);
}

void *f_realloc(void *p, size_t x)
{
#ifdef VERBOSE
  printf("f_realloc %u\r\n", x);
#endif

  auto m = malloc_spiram(x);
  memcpy(m, p, x);
  free(p);

  return m;
}

void helper_task(void *arg)
{
  net.url = url_buf;
  memset(net.url, 0, 1024);

  while (1)
  {
    int task;

    if (xQueueReceive(helper_queue, &task, portMAX_DELAY))
    {
      switch (task)
      {
        case TASK_RUN_FUNC0:
        case TASK_RUN_FUNC1:
        case TASK_RUN_FUNC2:
        case TASK_RUN_FUNC3:
        {
          int handle = rd_reg8(ESP_REG_LIB_HANDLE);
          int func = rd_reg8(ESP_REG_FUNC);
          int arg = rd_reg32(ESP_REG_ARG);

          if (!check_handle(handle) || (mem_obj[handle].type != OBJ_TYPE_LIB))
          {
            set_status(ESP_ERR_INV_LIB_HANDLE);
            break;
          }

          int arr1h = 0;
          int arr2h = 0;
          int arr3h = 0;

          if (task >= TASK_RUN_FUNC1)
          {
            arr1h = rd_reg8(ESP_REG_ARR1_HANDLE);

            if (!check_handle(arr1h) || ((mem_obj[arr1h].type != OBJ_TYPE_DATA) && (mem_obj[arr1h].type != OBJ_TYPE_DATAF)))
            {
              set_status(ESP_ERR_INV_ARG_HANDLE);
#ifdef VERBOSE
              printf("Handle error, arr1h %d, type %d\r\n", arr1h, mem_obj[arr1h].type);
#endif
              break;
            }
          }

          if (task >= TASK_RUN_FUNC2)
          {
            arr2h = rd_reg8(ESP_REG_ARR2_HANDLE);

            if (!check_handle(arr2h) || ((mem_obj[arr2h].type != OBJ_TYPE_DATA) && (mem_obj[arr2h].type != OBJ_TYPE_DATAF)))
            {
              set_status(ESP_ERR_INV_ARG_HANDLE);
              break;
            }
          }

          if (task == TASK_RUN_FUNC3)
          {
            arr3h = rd_reg8(ESP_REG_ARR3_HANDLE);

            if (!check_handle(arr3h) || ((mem_obj[arr3h].type != OBJ_TYPE_DATA) && (mem_obj[arr3h].type != OBJ_TYPE_DATAF)))
            {
              set_status(ESP_ERR_INV_ARG_HANDLE);
              break;
            }
          }

          void *entry = mem_obj[handle].entry;
          void *arr1 = mem_obj[arr1h].addr;
          void *arr2 = mem_obj[arr2h].addr;
          void *arr3 = mem_obj[arr3h].addr;
          int rc = 0;

          switch (task)
          {
            case TASK_RUN_FUNC0:
            {
              typedef int (*F0)(int func, int arg);
              F0 f = (F0)entry;
              rc = f(func, arg);
            }
            break;

            case TASK_RUN_FUNC1:
            {
              typedef int (*F1)(int func, int arg, void *arr1);
              F1 f = (F1)entry;
              rc = f(func, arg, arr1);
            }
            break;

            case TASK_RUN_FUNC2:
            {
              typedef int (*F2)(int func, int arg, void *arr1, void *arr2);
              F2 f = (F2)entry;
              rc = f(func, arg, arr1, arr2);
            }
            break;

            case TASK_RUN_FUNC3:
            {
              typedef int (*F3)(int func, int arg, void *arr1, void *arr2, void *arr3);
              F3 f = (F3)entry;
              rc = f(func, arg, arr1, arr2, arr3);
            }
            break;
          }

#ifdef VERBOSE
          printf("RC: 0x%08lX\r\n", (u32)rc);
#endif

          wr_reg32(ESP_REG_RETVAL, rc);

          set_status(ESP_ST_READY);
        }
        break;

        case TASK_LOAD_ELF:
        case TASK_LOAD_ELF_OPT:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);
          int opt = (task == TASK_LOAD_ELF_OPT) ? rd_reg8(ESP_REG_OPT) : 0;

          if (!check_handle(handle) || (mem_obj[handle].type != OBJ_TYPE_ELF))
          {
            set_status(ESP_ERR_INV_ELF_HANDLE);
            break;
          }

          int new_size = load_elf((u8*)mem_obj[handle].addr, &mem_obj[handle], opt);

          if (new_size == -1)
            set_status(ESP_ERR_OUT_OF_MEMORY);
          else
          {
            // Delete ELF from memory and change object type to library
            if (mem_obj[handle].addr) free(mem_obj[handle].addr);

            mem_obj[handle].addr = (void*)-1;
            mem_obj[handle].size = new_size;
            mem_obj[handle].type = OBJ_TYPE_LIB;
            mem_obj[handle].state = LIB_OBJ_ST_READY;
            wr_reg8(ESP_REG_LIB_HANDLE, handle);

            set_status(ESP_ST_READY);
          }
        }
        break;

        case TASK_MAKE_OBJ:
        {
          size_t obj_size = rd_reg32(ESP_REG_DATA_SIZE);
          int obj_type = rd_reg8(ESP_REG_OBJ_TYPE);

          int obj_hdl = make_obj(obj_size, obj_type); // set_status is done there

          if (obj_hdl >= 0)
          {
            wr_reg8(ESP_REG_OBJ_HANDLE, obj_hdl);
            wr_reg32(ESP_REG_DATA_OFFSET, 0);
            wr_reg32(ESP_REG_DATA_SIZE, min(obj_size, DMA_BUF_SIZE));

            set_status(ESP_ST_READY);
          }
        }
        break;

        case TASK_DEL_OBJ:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);

          if (!check_handle(handle))
          {
            set_status(ESP_ERR_INV_HANDLE);
            ESP_LOGE(TAG, "Wrong handle! (%02X)", handle);
            break;
          }

          auto rc = delete_obj(handle);

          if (!rc)
          {
            set_status(ESP_ERR_OBJ_NOT_DELETED);
            ESP_LOGE(TAG, "Object cannot be deleted! (%02X)", handle);
          }
          else
          {
            set_status(ESP_ST_READY);

#ifdef VERBOSE
            printf("Object deleted: handle %02X\r\n", handle);
#endif
          }
        }
        break;

        case TASK_KILL_OBJ:
        {
          for (int i = 0; i < OBJ_HANDLES_MAX; i++)
          {
            if (check_handle(i))
            {
              auto rc = delete_obj(i);

              if (!rc)
                ESP_LOGE(TAG, "Object cannot be deleted! (%02X)", i);
#ifdef VERBOSE
              else
                printf("Object deleted: handle %02X\r\n", i);
#endif
            }
          }

          set_status(ESP_ST_READY);
        }
        break;

        case TASK_DEHST:
        {
          int hsrc = rd_reg8(ESP_REG_OBJ_HANDLE);
          u32 ssize = rd_reg32(ESP_REG_DATA_SIZE);
          int dtype = rd_reg8(ESP_REG_OBJ_TYPE);

          if (!check_handle(hsrc))
          {
            set_status(ESP_ERR_INV_HST_HANDLE);
            break;
          }

          if (mem_obj[hsrc].type != OBJ_TYPE_HST)
          {
            set_status(ESP_ERR_INV_OBJ_TYPE);
            break;
          }

          // Check source size
          if (!ssize || (ssize > 65536))
          {
            set_status(ESP_ERR_INV_SIZE);
            break;
          }

          // Unpack the HST
          u8 *tmp = (u8*)malloc_spiram(65536);

          if (!tmp)
          {
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          u32 dsize = dehrust(tmp, (u8*)mem_obj[hsrc].addr, ssize);

          if (!dsize)
          {
            free(tmp);
            set_status(ESP_ERR_INV_HST);
            break;
          }

          void *dst = malloc_spiram(dsize);

          if (!dst)
          {
            free(tmp);
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          memcpy(dst, tmp, dsize);
          free(tmp);
          free(mem_obj[hsrc].addr);

          mem_obj[hsrc].addr = dst;
          mem_obj[hsrc].size = dsize;
          mem_obj[hsrc].type = dtype;

          wr_reg32(ESP_REG_DATA_OFFSET, 0);
          wr_reg32(ESP_REG_DATA_SIZE, min(dsize, DMA_BUF_SIZE));

          set_status(ESP_ST_READY);
        }
        break;

        case TASK_UNZIP:
        {
          int hsrc = rd_reg8(ESP_REG_OBJ_HANDLE);
          size_t ssize = rd_reg32(ESP_REG_DATA_SIZE);
          int dtype = rd_reg8(ESP_REG_OBJ_TYPE);

          if (!check_handle(hsrc))
          {
            set_status(ESP_ERR_INV_HST_HANDLE);
            break;
          }

          if (mem_obj[hsrc].type != OBJ_TYPE_ZIP)
          {
            set_status(ESP_ERR_INV_OBJ_TYPE);
            break;
          }

          auto src = (u8*)mem_obj[hsrc].addr;
          size_t dsize = *(u32*)src;
          void *dst = (u8*)malloc_spiram(dsize);

          if (!dst)
          {
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          tinfl_init(decomp);

          ssize -= 4;
          tinfl_status decomp_status = tinfl_decompress(decomp, &src[4], &ssize, (u8*)dst, (u8*)dst, &dsize, TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

#ifdef VERBOSE
          printf("Unzip rc = %d, inbytes = %d, outbytes = %d\r\n", decomp_status, ssize, dsize);
#endif

          if (decomp_status != TINFL_STATUS_DONE)
          {
            set_status(ESP_ERR_INV_ZIP);
            break;
          }

          mem_obj[hsrc].addr = dst;
          mem_obj[hsrc].size = dsize;
          mem_obj[hsrc].type = dtype;

          wr_reg32(ESP_REG_DATA_OFFSET, 0);
          wr_reg32(ESP_REG_DATA_SIZE, min(dsize, DMA_BUF_SIZE));

          set_status(ESP_ST_READY);
        }
        break;

        case TASK_RESET_STREAM_UNZIP:
          process_rx_data(DREQ_ZIP, 0);   // Reset stream depacker
        break;

        case TASK_WSCAN:
        {
#ifdef CONFIG_ESP32_WIFI_ENABLED
          if (net.is_busy)
          {
            set_status(ESP_ERR_NET_BUSY);
            break;
          }

          net.is_busy = true;
          wf_scan(300 /* timeout */);
          net.is_busy = false;
          put_txq(DREQ_WSCAN);
#endif
        }
        break;

        case TASK_CONN:
        {
#ifdef CONFIG_ESP32_WIFI_ENABLED
          if (net.is_busy)
          {
            set_status(ESP_ERR_NET_BUSY);
            break;
          }

          net.is_busy = true;
          net.state = NETWORK_OPENING;
          auto rc = wifi_connect((const char*)net.ssid, (const char*)net.pwd, 10000);
          net.is_busy = false;
          net.state = rc ? NETWORK_OPEN : NETWORK_CLOSED;

          if (rc)
          {
            get_ip(net.ip.own_ip, net.ip.mask, net.ip.gate);
            set_status(ESP_ST_READY);
          }
          else
#endif
            set_status(ESP_ERR_AP_NOT_CONNECTED);
        }
        break;

        case TASK_DISCONN:
        {
#ifdef CONFIG_ESP32_WIFI_ENABLED
          wifi_disconnect_now();
          net.state = NETWORK_CLOSED;
#endif
          set_status(ESP_ST_READY);
        }
        break;

        case TASK_HTTP_GET:
          http_do_get();
        break;

        case TASK_HTTPS_GET:
          https_do_get();
        break;

        case TASK_GOPHER_GET:
          gopher_do_get();
        break;

        case TASK_HTTP_STREAM_START:
          stream_http_start();
        break;

        case TASK_HTTPS_STREAM_START:
          stream_https_start();
        break;

        case TASK_GOPHER_STREAM_START:
          stream_gopher_start();
        break;

        case TASK_STREAM_READ:
          stream_read();
        break;

        case TASK_STREAM_CLOSE:
          stream_close();
        break;

        case TASK_HTTP_STREAM_READ:
          http_stream_read_task();
        break;

        case TASK_REBOOT:
          esp_restart();
        break;

        case TASK_RESET:
          esp_restart();
        break;
      }
    }
  }
}
