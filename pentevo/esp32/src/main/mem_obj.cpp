
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
#include "xm.h"
#include "xm_cpp.h"

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
      if (mem_obj[h].state == XM_OBJ_ST_PLAYING)
      {
        ESP_LOGE(TAG, "Cannot delete playing XM object: handle %02X", h);
        return 0;
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
    case OBJ_TYPE_XMC:
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
