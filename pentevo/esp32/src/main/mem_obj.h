
#pragma once

#pragma pack(1)
typedef struct
{
  // Common
  void *addr;
  size_t size;
  u8 type;
  u8 state;

  // Lib
  void *text;
  void *rodata;
  void *data;
  void *bss;

  u32 sz_text;
  u32 sz_rodata;
  u32 sz_data;
  u32 sz_bss;
  
  void *entry;
} MEM_OBJ;
#pragma pack()

#define OBJ_HANDLES_MAX 256
extern MEM_OBJ mem_obj[OBJ_HANDLES_MAX];

int find_avail_handle();
int check_handle(int h);
int delete_handle(int h);
int make_obj(int obj_size, int obj_type);
