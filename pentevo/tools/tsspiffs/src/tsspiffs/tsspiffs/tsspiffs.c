
#include "tsspiffs.h"
#include <string.h>
#include <stddef.h>

#define chunk_addr(chunk) (cfg->bulk_start + ((chunk) * cfg->block_size))

TSF_RESULT tsf_format(TSF_CONFIG *cfg)
{
  for (u32 i = 0; i < cfg->bulk_size; i += cfg->block_size)
    tsf_init_chunk(cfg, i + cfg->bulk_start);

  return TSF_RES_OK;
}

TSF_RESULT tsf_init_chunk(TSF_CONFIG *cfg, u32 addr)
{
  cfg->hal_erase_func(addr);

#ifdef TSF_CHECK_BLANK
  TSF_RESULT rc = tsf_check_blank(cfg, addr, cfg->block_size);
  if (rc != TSF_RES_OK)
    return rc;
#endif

  u32 magic = TSF_MAGIC;
  cfg->hal_write_func(addr, &magic, sizeof(magic));

  return TSF_RES_OK;
}

TSF_RESULT tsf_check_blank(TSF_CONFIG *cfg, u32 addr, u32 size)
{
  while (size)
  {
    u16 sz = (u16)min(cfg->buf_size, size);
    u8 *buf = cfg->buf;
    cfg->hal_read_func(addr, buf, sz);

    for (u16 j = 0; j < sz; j++)
      if (*buf++ != 0xFF)
        return TSF_RES_NOT_BLANK;

    size -= sz;
    addr += sz;
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_take_new_chunk(TSF_VOLUME *vol, u8 type, u32 *chunk_addr)
{
  if (!vol->free) return TSF_RES_BULK_FULL;   // to speed up the search
  
  TSF_CONFIG *cfg = vol->cfg;

  u16 chunk_num = cfg->last_written_chunk;
  u32 addr = chunk_addr(chunk_num);
  TSF_CHUNK chunk;

  do
  {
    cfg->hal_read_func(addr, &chunk, sizeof(TSF_CHUNK));

    if ((chunk.magic == TSF_MAGIC) && (chunk.type == TSF_CHUNK_FREE))
    {
      *chunk_addr = addr;
      cfg->last_written_chunk = chunk_num;
      vol->free -= cfg->block_size;

      cfg->hal_write_func(addr + offsetof(TSF_CHUNK, type), &type, sizeof(type));

      return TSF_RES_OK;
    }

    addr += cfg->block_size;
    chunk_num++;
    
    if (addr >= cfg->bulk_start + cfg->bulk_size)
    {
      addr = cfg->bulk_start;
      chunk_num = 0;
    }
  } while (chunk_num != cfg->last_written_chunk);

  return TSF_RES_BULK_FULL;
}

TSF_RESULT tsf_mount(TSF_CONFIG *cfg, TSF_VOLUME *vol)
{
  cfg->last_written_chunk = 0;
  vol->cfg = cfg;

  return tsf_vol_stat(vol);
}

TSF_RESULT tsf_vol_stat(TSF_VOLUME *vol)
{
  TSF_CONFIG *cfg = vol->cfg;
  vol->free = 0;
  vol->chunks_number = 0;
  vol->files_number = 0;

  for (u32 i = 0; i < cfg->bulk_size; i += cfg->block_size)
  {
    TSF_CHUNK chunk;
    cfg->hal_read_func(i + cfg->bulk_start, &chunk, sizeof(TSF_CHUNK));

    if (chunk.magic == TSF_MAGIC)
    {
      vol->chunks_number++;

      if (chunk.type == TSF_CHUNK_FREE)
        vol->free += cfg->block_size - sizeof(TSF_CHUNK);

      else if (chunk.type & TSF_CHUNK_HEAD)
        vol->files_number++;
    }
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_search(TSF_VOLUME *vol, u32 *chunk_addr, const char *name)
{
  TSF_CONFIG *cfg = vol->cfg;
  u8 fnlen = (u8)strlen(name);

  for (u32 i = 0; i < cfg->bulk_size; i += cfg->block_size)
  {
    u32 addr = i + cfg->bulk_start;
    TSF_CHUNK chunk;
    cfg->hal_read_func(addr, &chunk, sizeof(TSF_CHUNK));

    if ((chunk.magic != TSF_MAGIC) || (chunk.type != TSF_CHUNK_HEAD))
      continue;

    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, fnlen), cfg->buf, fnlen + 1);

    if (fnlen != cfg->buf[0])
      continue;

    if (strncmp(name, (const char*)&cfg->buf[1], fnlen))
      continue;

    *chunk_addr = addr;

    return TSF_RES_OK;
  }

  return TSF_RES_FILE_NOT_FOUND;
}

TSF_RESULT tsf_open(TSF_VOLUME *vol, TSF_FILE *file, const char *name, u8 mode)
{
  file->mode = mode;
  file->vol = vol;

  if (mode == TSF_MODE_READ)
    return tsf_open_for_read(file, name);
  else if (mode == TSF_MODE_CREATE_WRITE)
    return tsf_create(file, name);
  else
    return TSF_RES_MODE_ERROR;
}

TSF_RESULT tsf_open_for_read(TSF_FILE *file, const char *name)
{
  TSF_VOLUME *vol = file->vol;
  u32 addr;

  TSF_RESULT rc = tsf_search(vol, &addr, name);
  if (rc != TSF_RES_OK)
    return rc;

  file->addr = file->chunk_addr = addr;
  vol->cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, size), &file->size, sizeof(file->size));
  file->seek = 0;
  file->chunk_offset = sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + (u8)strlen(name);
  vol->cfg->hal_read_func(addr + offsetof(TSF_CHUNK, next_chunk), &file->next_chunk, sizeof(file->next_chunk));

  return TSF_RES_OK;
}

TSF_RESULT tsf_create(TSF_FILE *file, const char *name)
{
  TSF_VOLUME *vol = file->vol;
  u32 addr;
  u8 fnlen = (u8)strlen(name);

#ifdef TSF_CHECK_EXIST_ON_CREATE
  {
    TSF_RESULT rc = tsf_search(vol, &addr, name);

    if (rc != TSF_RES_FILE_NOT_FOUND)
    {
      if (rc == TSF_RES_OK)
        return TSF_RES_FILE_EXISTS;
      else
        return rc;
    }
  }
#endif

  if (tsf_take_new_chunk(vol, TSF_CHUNK_HEAD, &addr) != TSF_RES_OK)
    return TSF_RES_BULK_FULL;

  TSF_CONFIG *cfg = vol->cfg;

  file->addr = addr;
  file->size = 0;
  file->seek = 0;
  file->chunk_addr = addr;
  file->chunk_offset = sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + fnlen;

  addr += sizeof(TSF_CHUNK);
  TSF_HDR hdr;
  hdr.size = 0xFFFFFFFF;
  hdr.fnlen = fnlen;
  cfg->hal_write_func(addr, &hdr, sizeof(TSF_HDR));
  addr += sizeof(TSF_HDR);
  cfg->hal_write_func(addr, name, fnlen);

  return TSF_RES_OK;
}

TSF_RESULT tsf_close(TSF_FILE *file)
{
  if ((file->mode & TSF_MODE_WRITE) == TSF_MODE_WRITE)
  {
    file->vol->cfg->hal_write_func(file->addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, size), &file->size, sizeof(file->size));   // save file size
    file->vol->files_number++;
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_read(TSF_FILE *file, void *buf, u32 size)
{
  return tsf_read_int(file, buf, size);
}

TSF_RESULT tsf_seek(TSF_FILE *file, u32 size)
{
  return tsf_read_int(file, 0, size);
}

TSF_RESULT tsf_read_int(TSF_FILE *file, void *buf, u32 size)
{
  u8 *data = (u8*)buf;
  TSF_CONFIG *cfg = file->vol->cfg;
  u32 addr = file->chunk_addr;
  size = min(size, file->size - file->seek);

  while (size)
  {
    if (file->chunk_offset == cfg->block_size)  // new chunk
    {
      addr = file->chunk_addr = chunk_addr(file->next_chunk);
      file->chunk_offset = sizeof(TSF_CHUNK);
      cfg->hal_read_func(addr + offsetof(TSF_CHUNK, next_chunk), &file->next_chunk, sizeof(file->next_chunk));
    }

    u16 sz = (u16)min(cfg->block_size - file->chunk_offset, size);

    if (buf)
    {
      cfg->hal_read_func(file->chunk_addr + file->chunk_offset, data, sz);
      data += sz;
    }

    file->seek += sz;
    file->chunk_offset += sz;
    size -= sz;
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_write(TSF_FILE *file, const void *buf, u32 size)
{
  u8 *data = (u8*)buf;
  TSF_CONFIG *cfg = file->vol->cfg;
  u32 addr = file->chunk_addr;

  while (size)
  {
    if (file->chunk_offset == cfg->block_size)  // new chunk
    {
      if (tsf_take_new_chunk(file->vol, TSF_CHUNK_BODY, &addr) != TSF_RES_OK)
        return TSF_RES_BULK_FULL;

      file->chunk_addr = addr;
      file->chunk_offset = sizeof(TSF_CHUNK);

      cfg->hal_write_func(file->prev_chunk_addr + offsetof(TSF_CHUNK, next_chunk), &cfg->last_written_chunk, sizeof(cfg->last_written_chunk));   // save next chunk address to the previous one, cfg->last_written_chunk is set after tsf_take_new_chunk()
    }

    u16 sz = (u16)min(cfg->block_size - file->chunk_offset, size);
    cfg->hal_write_func(file->chunk_addr + file->chunk_offset, data, sz);
    file->seek += sz;
    file->size += sz;
    file->chunk_offset += sz;
    data += sz;
    size -= sz;

    if (file->chunk_offset == cfg->block_size)  // close current chunk
      file->prev_chunk_addr = file->chunk_addr;
  }

  file->vol->files_number--;

  return TSF_RES_OK;
}

TSF_RESULT tsf_delete(TSF_VOLUME *vol, const char *name)
{
  u32 addr;
  u16 next;
  TSF_CONFIG *cfg = vol->cfg;

  TSF_RESULT rc = tsf_search(vol, &addr, name);
  if (rc != TSF_RES_OK)
    return rc;

  do
  {
    cfg->hal_read_func(addr + offsetof(TSF_CHUNK, next_chunk), &next, sizeof(next));
    if (tsf_init_chunk(cfg, addr) == TSF_RES_OK)
      vol->free += cfg->block_size;
  } while (next != 0xFFFF);

  return TSF_RES_OK;
}

TSF_RESULT tsf_stat(TSF_VOLUME *vol, TSF_FILE_STAT *stat, const char *name)
{
  u32 addr;

  TSF_RESULT rc = tsf_search(vol, &addr, name);
  if (rc != TSF_RES_OK)
    return rc;

  vol->cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, size), &stat->size, sizeof(stat->size));
  return TSF_RES_OK;
}

TSF_RESULT tsf_list(TSF_VOLUME *vol, u8 flag)
{
  static u32 last_addr;
  TSF_CONFIG *cfg = vol->cfg;

  if (flag == TSF_LIST_START)
  {
    last_addr = cfg->bulk_start;
    return TSF_RES_OK;
  }

  for (u32 addr = last_addr; addr < (cfg->bulk_start + cfg->bulk_size); addr += cfg->block_size)
  {
    TSF_CHUNK chunk;
    cfg->hal_read_func(addr, &chunk, sizeof(TSF_CHUNK));

    if ((chunk.magic != TSF_MAGIC) || (chunk.type != TSF_CHUNK_HEAD))
      continue;

    u8 fnlen;
    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, fnlen), &fnlen, sizeof(fnlen));
    fnlen = min(fnlen, cfg->buf_size - 1);
    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), cfg->buf, fnlen);
    cfg->buf[fnlen] = 0;

    last_addr = addr + cfg->block_size;

    return TSF_RES_OK;
  }

  return TSF_RES_NO_MORE_FILES;
}
