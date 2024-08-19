
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
#include "wifi.h"
#include "helper.h"

const char TAG[] = "helper";

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
  
  return heap_caps_malloc(x, MALLOC_CAP_SPIRAM);
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
  
  auto m = heap_caps_malloc(x, MALLOC_CAP_SPIRAM);
  memcpy(m, p, x);
  free(p);
  
  return m;
}

int mz_uncomp(u8 *dst, u32 &dst_len, u8 *src, u32 src_len)
{
  int rc;
  mz_stream stream;
  memset(&stream, 0, sizeof(stream));

  stream.next_in = src;
  stream.avail_in = src_len;
  stream.next_out = dst;
  stream.avail_out = dst_len;

  rc = mz_inflateInit(&stream);
  if (rc != MZ_OK) return rc;

  rc = mz_inflate(&stream, MZ_FINISH);
  src_len = src_len - stream.avail_in;

  if (rc != MZ_STREAM_END)
  {
    mz_inflateEnd(&stream);
    return ((rc == MZ_BUF_ERROR) && (!stream.avail_in)) ? MZ_DATA_ERROR : rc;
  }

  dst_len = stream.total_out;
  return mz_inflateEnd(&stream);
}

void helper_task(void *arg)
{
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

          set_ok_ready();
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

            set_ok_ready();
          }
        }
        break;

        case TASK_MAKE_OBJ:
        {
          size_t obj_size = rd_reg32(ESP_REG_DATA_SIZE);
          int obj_type = rd_reg8(ESP_REG_OBJ_TYPE);

          int obj_hdl = make_obj(obj_size, obj_type);

          if (obj_hdl >= 0)
          {
            wr_reg8(ESP_REG_OBJ_HANDLE, obj_hdl);
            wr_reg32(ESP_REG_DATA_OFFSET, 0);
            wr_reg32(ESP_REG_DATA_SIZE, min(obj_size, DMA_BUF_SIZE));

            set_ok_ready();

#ifdef VERBOSE
            printf("Object created: type %02X, handle %02X, addr %08X, size %u\r\n", obj_type, handle, (unsigned int)obj_addr, obj_size);
#endif
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

          auto rc = delete_handle(handle);

          if (!rc)
          {
            set_status(ESP_ERR_OBJ_NOT_DELETED);
            ESP_LOGE(TAG, "Object cannot be deleted! (%02X)", handle);
          }
          else
          {
            set_ok_ready();

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
              auto rc = delete_handle(i);

              if (!rc)
                ESP_LOGE(TAG, "Object cannot be deleted! (%02X)", i);
#ifdef VERBOSE
              else
                printf("Object deleted: handle %02X\r\n", i);
#endif
            }
          }

          set_ok_ready();
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
          u8 *tmp = (u8*)heap_caps_malloc(65536, MALLOC_CAP_SPIRAM);
   
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
   
          void *dst = heap_caps_malloc(dsize, MALLOC_CAP_SPIRAM);
 
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

          set_ok_ready();
        }
        break;
        
        case TASK_UNZIP:
        {
          int hsrc = rd_reg8(ESP_REG_OBJ_HANDLE);
          u32 ssize = rd_reg32(ESP_REG_DATA_SIZE);
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
          u32 dsize = *(u32*)src;
          void *dst = (u8*)heap_caps_malloc(dsize, MALLOC_CAP_SPIRAM);
  
          if (!dst)
          {
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

          ssize -= 4;
          auto rc = mz_uncomp((u8*)dst, dsize, &src[4], ssize);
          
          if (rc != MZ_OK)
          {
            set_status((rc == MZ_MEM_ERROR) ? ESP_ERR_OUT_OF_MEMORY : ESP_ERR_INV_ZIP);
            break;
          }
          
#ifdef VERBOSE
          printf("Unzip rc = %d, i_sz = %ld, o_sz = %ld\r\n", rc, ssize, dsize);
#endif
          
          mem_obj[hsrc].addr = dst;
          mem_obj[hsrc].size = dsize;
          mem_obj[hsrc].type = dtype;

          wr_reg32(ESP_REG_DATA_OFFSET, 0);
          wr_reg32(ESP_REG_DATA_SIZE, min(dsize, DMA_BUF_SIZE));

          set_ok_ready();
        }
        break;

        case TASK_WSCAN:
        {
          if (net.is_busy)
          {
            set_status(ESP_ERR_NET_BUSY);
            break;
          }

          net.is_busy = true;
          wf_scan();
          net.is_busy = false;
          put_txq(DREQ_WSCAN);
        }
        break;

        case TASK_CONN:
        {
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
            set_ok_ready();
          }
          else
            set_status(ESP_ERR_AP_NOT_CONNECTED);
        }
        break;
      }
    }
  }
}
