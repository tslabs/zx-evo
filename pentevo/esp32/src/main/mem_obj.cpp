
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "helper.h"
#include "mem_obj.h"
#include "tracker.h"
#include "sfx.h"

const char TAG[] = "mem_obj";

EXT_RAM_BSS_ATTR MEM_OBJ mem_obj[OBJ_HANDLES_MAX] = {};

int find_avail_handle()
{
  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (!mem_obj[i].type)
      return i;

  return -1;
}

int check_handle(int h)
{
  return mem_obj[h].type != 0;
}

int mem_obj_can_flat_rw(int h)
{
  if (!check_handle(h)) return 0;

  switch (mem_obj[h].type)
  {
    case OBJ_TYPE_LIB:
    case OBJ_TYPE_XMC:
    case OBJ_TYPE_MDC:
    case OBJ_TYPE_S3C:
      return 0;
  }

  return 1;
}

int delete_obj(int h)
{
  switch (mem_obj[h].type)
  {
    case OBJ_TYPE_LIB:
    {
      if (mem_obj[h].text) free(mem_obj[h].text);
      if (mem_obj[h].data) free(mem_obj[h].data);
      if (mem_obj[h].rodata) free(mem_obj[h].rodata);
      if (mem_obj[h].bss) free(mem_obj[h].bss);
      memset(&mem_obj[h], 0, sizeof(MEM_OBJ));

      return 1;
    }

    case OBJ_TYPE_XMC:
    case OBJ_TYPE_MDC:
    case OBJ_TYPE_S3C:
      if (mem_obj[h].state == TRACK_OBJ_ST_PLAYING)
      {
        ESP_LOGE(TAG, "Cannot delete playing tracker object: handle %02X", h);
        return 0;
      }

      if (mem_obj[h].addr)
      {
        xm_free_context((xm_context_t*)mem_obj[h].addr);
        memset(&mem_obj[h], 0, sizeof(MEM_OBJ));
        return 1;
      }
    break;

    case OBJ_TYPE_WAV:
    {
      u8 old_state = mem_obj[h].state;
      mem_obj[h].state = OBJ_ST_DELETING;

      esp_err_t err = sfx_stop_handle_sync(h);
      if (err != ESP_OK)
      {
        mem_obj[h].state = old_state;
        ESP_LOGE(TAG, "Cannot stop WAV object users before delete: handle %02X, err %s", h, esp_err_to_name(err));
        return 0;
      }
    }
    break;
  }

  if (mem_obj[h].addr) free(mem_obj[h].addr);
  memset(&mem_obj[h], 0, sizeof(MEM_OBJ));

  return 1;
}

void write_obj(int handle, const void *from, int size)
{
  memcpy(mem_obj[handle].addr, from, max(mem_obj[handle].size, size));
}

int attach_obj(void *addr, int obj_size, int obj_type)
{
  if (!addr || obj_size <= 0)
  {
    set_status(ESP_ERR_INV_PARAM);
    return -1;
  }

  int handle = find_avail_handle();
  if (handle == -1)
  {
    set_status(ESP_ERR_OUT_OF_HANDLES);
    ESP_LOGE(TAG, "Cannot allocate handle for an object!");
    return -1;
  }

  switch (obj_type)
  {
    case OBJ_TYPE_XM:
    case OBJ_TYPE_XMC:
    case OBJ_TYPE_MOD:
    case OBJ_TYPE_MDC:
    case OBJ_TYPE_S3M:
    case OBJ_TYPE_S3C:
    case OBJ_TYPE_WAV:
    case OBJ_TYPE_DATA:
    case OBJ_TYPE_ELF:
    case OBJ_TYPE_HST:
    case OBJ_TYPE_ZIP:
    case OBJ_TYPE_DATAF:
      break;

    default:
      ESP_LOGE(TAG, "Cannot attach unknown type object!");
      set_status(ESP_ERR_INV_OBJ_TYPE);
      return -1;
  }

  if (obj_type == OBJ_TYPE_XMC || obj_type == OBJ_TYPE_MDC || obj_type == OBJ_TYPE_S3C)
  {
    xm_context_t *ctx = (xm_context_t*)addr;

    if (!tracker_context_has_registered_segments(ctx))
    {
      size_t context_size = ctx->ctx_size ? ctx->ctx_size : (size_t)obj_size;
      if (!ctx->ctx_size) ctx->ctx_size = context_size;

      if (!tracker_context_register_segment(ctx, addr, context_size, TRACKER_CONTEXT_SEG_CONTEXT))
      {
        set_status(ESP_ERR_INV_STATE);
        ESP_LOGE(TAG, "Cannot attach tracker object without segment ownership!");
        return -1;
      }
    }
  }

  mem_obj[handle].addr = addr;
  mem_obj[handle].size = obj_size;
  mem_obj[handle].type = obj_type;
  mem_obj[handle].state = OBJ_ST_NONE;

  return handle;
}

int make_obj(int obj_size, int obj_type)
{
  void *obj_addr = NULL;

  int handle = find_avail_handle();

  if (handle == -1)
  {
    set_status(ESP_ERR_OUT_OF_HANDLES);
    ESP_LOGE(TAG, "Cannot allocate handle for an object!");
    return -1;
  }

  switch (obj_type)
  {
    case OBJ_TYPE_XM:
    case OBJ_TYPE_MOD:
    case OBJ_TYPE_S3M:
    case OBJ_TYPE_WAV:
    case OBJ_TYPE_DATA:
    case OBJ_TYPE_ELF:
    case OBJ_TYPE_HST:
    case OBJ_TYPE_ZIP:
      obj_addr = malloc_spiram(obj_size);
      break;

    case OBJ_TYPE_DATAF:
      obj_addr = heap_caps_malloc(obj_size, MALLOC_CAP_INTERNAL);
      break;

    default:
    {
      ESP_LOGE(TAG, "Cannot create unknown type object!");
      set_status(ESP_ERR_INV_OBJ_TYPE);
      return -1;
    }
  }

  if (!obj_addr)
  {
    set_status(ESP_ERR_OUT_OF_MEMORY);
    ESP_LOGE(TAG, "Cannot allocate memory for an object! (%d bytes)", obj_size);
    return -1;
  }

#ifdef VERBOSE
  else
    printf("Object created: type %02X, handle %02X, addr %08X, size %u\r\n", obj_type, handle, (unsigned int)obj_addr, obj_size);
#endif

  mem_obj[handle].addr = obj_addr;
  mem_obj[handle].size = obj_size;
  mem_obj[handle].type = obj_type;
  mem_obj[handle].state = OBJ_ST_NONE;

  return handle;
}
