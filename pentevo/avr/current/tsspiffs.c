
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "tsspiffs.h"

TSF_RESULT tsf_format(TSF_CONFIG *cfg)
{
  if (!cfg || !cfg->block_size || !cfg->bulk_size) return TSF_RES_FS_ERROR;
  if ((cfg->bulk_size % cfg->block_size) != 0) return TSF_RES_FS_ERROR;
  if (cfg->block_size <= sizeof(TSF_CHUNK)) return TSF_RES_FS_ERROR;
  if (!cfg->hal_read_func || !cfg->hal_write_func) return TSF_RES_FS_ERROR;
  if (!cfg->hal_erase_func && !cfg->hal_erase_range_func) return TSF_RES_FS_ERROR;

  return tsf_init_chunk_range(cfg, cfg->bulk_start, cfg->bulk_size);
}

TSF_RESULT tsf_init_chunk(TSF_CONFIG *cfg, u32 addr)
{
  if (!cfg) return TSF_RES_FS_ERROR;
  return tsf_init_chunk_range(cfg, addr, cfg->block_size);
}

TSF_RESULT tsf_init_erased_chunk(TSF_CONFIG *cfg, u32 addr)
{
  TSF_RESULT rc;

  if (!cfg || !cfg->block_size) return TSF_RES_FS_ERROR;
  if (!cfg->hal_write_func) return TSF_RES_FS_ERROR;

  if (cfg->poll_func)
  {
    rc = cfg->poll_func();
    if (rc != TSF_RES_OK) return rc;
  }

#ifdef TSF_CHECK_BLANK
  rc = tsf_check_blank(cfg, addr, cfg->block_size);
  if (rc != TSF_RES_OK)
    return rc;
#endif

  u32 magic = TSF_MAGIC;
  cfg->hal_write_func(addr, &magic, sizeof(magic));

  return TSF_RES_OK;
}

TSF_RESULT tsf_init_chunk_range(TSF_CONFIG *cfg, u32 addr, u32 size)
{
  TSF_RESULT rc;

  if (!cfg || !cfg->block_size || !size) return TSF_RES_FS_ERROR;
  if ((size % cfg->block_size) != 0) return TSF_RES_FS_ERROR;
  if (!cfg->hal_erase_func && !cfg->hal_erase_range_func) return TSF_RES_FS_ERROR;

  if (cfg->poll_func)
  {
    rc = cfg->poll_func();
    if (rc != TSF_RES_OK) return rc;
  }

  if (cfg->hal_erase_range_func)
  {
    rc = cfg->hal_erase_range_func(addr, size);
    if (rc != TSF_RES_OK)
      return rc;
  }
  else
  {
    for (u32 pos = 0; pos < size; pos += cfg->block_size)
    {
      if (cfg->poll_func)
      {
        rc = cfg->poll_func();
        if (rc != TSF_RES_OK) return rc;
      }

      rc = cfg->hal_erase_func(addr + pos);
      if (rc != TSF_RES_OK)
        return rc;
    }
  }

  for (u32 pos = 0; pos < size; pos += cfg->block_size)
  {
    if (cfg->poll_func)
    {
      rc = cfg->poll_func();
      if (rc != TSF_RES_OK) return rc;
    }

    rc = tsf_init_erased_chunk(cfg, addr + pos);
    if (rc != TSF_RES_OK)
      return rc;
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_check_blank(TSF_CONFIG *cfg, u32 addr, u32 size)
{
  TSF_RESULT rc;
  u16 poll_left = 0;

  if (!cfg || !cfg->buf || !cfg->buf_size) return TSF_RES_FS_ERROR;

  while (size)
  {
    u16 sz;
    u8 *buf;

    if (cfg->poll_func && (poll_left == 0))
    {
      rc = cfg->poll_func();
      if (rc != TSF_RES_OK) return rc;
      poll_left = 512;
    }

    sz = (u16)min(cfg->buf_size, size);
    buf = cfg->buf;
    cfg->hal_read_func(addr, buf, sz);

    for (u16 j = 0; j < sz; j++)
    {
      if (*buf++ != 0xFF) return TSF_RES_NOT_BLANK;
    }

    if (poll_left > sz) poll_left -= sz;
    else poll_left = 0;

    size -= sz;
    addr += sz;
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_take_new_chunk(TSF_VOLUME *vol, u8 type, u32 *chunk_addr)
{
  if (!vol || !vol->cfg || !chunk_addr) return TSF_RES_FS_ERROR;
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
  if (!cfg || !vol) return TSF_RES_FS_ERROR;
  if (!cfg->block_size || !cfg->bulk_size) return TSF_RES_FS_ERROR;
  if ((cfg->bulk_size % cfg->block_size) != 0) return TSF_RES_FS_ERROR;
  if (cfg->block_size <= sizeof(TSF_CHUNK)) return TSF_RES_FS_ERROR;
  if (!cfg->hal_read_func || !cfg->hal_write_func) return TSF_RES_FS_ERROR;
  if (!cfg->hal_erase_func && !cfg->hal_erase_range_func) return TSF_RES_FS_ERROR;

  cfg->last_written_chunk = 0;
  vol->cfg = cfg;
  vol->list_addr = cfg->bulk_start;

  return tsf_vol_stat(vol);
}

TSF_RESULT tsf_vol_stat(TSF_VOLUME *vol)
{
  if (!vol || !vol->cfg) return TSF_RES_FS_ERROR;

  TSF_CONFIG *cfg = vol->cfg;
  vol->free = 0;
  vol->chunks_number = 0;
  vol->files_number = 0;

  for (u32 i = 0; i < cfg->bulk_size; i += cfg->block_size)
  {
    TSF_CHUNK chunk;

    if (cfg->poll_func)
    {
      TSF_RESULT rc = cfg->poll_func();
      if (rc != TSF_RES_OK) return rc;
    }

    cfg->hal_read_func(i + cfg->bulk_start, &chunk, sizeof(TSF_CHUNK));

    if (chunk.magic == TSF_MAGIC)
    {
      vol->chunks_number++;

      if (chunk.type == TSF_CHUNK_FREE)
        vol->free += cfg->block_size;

      else if (chunk.type == (u8)TSF_CHUNK_HEAD)
        vol->files_number++;
    }
  }

  return TSF_RES_OK;
}

TSF_RESULT tsf_search(TSF_VOLUME *vol, u32 *chunk_addr, const char *name)
{
  if (!vol || !vol->cfg || !chunk_addr || !name) return TSF_RES_FS_ERROR;

  TSF_CONFIG *cfg = vol->cfg;
  size_t name_len = strlen(name);
  if (name_len > 255) return TSF_RES_FS_ERROR;
  if (name_len && (!cfg->buf || (cfg->buf_size <= name_len))) return TSF_RES_FS_ERROR;

  u8 fnlen = (u8)name_len;

  for (u32 i = 0; i < cfg->bulk_size; i += cfg->block_size)
  {
    u32 addr = i + cfg->bulk_start;
    TSF_CHUNK chunk;
    cfg->hal_read_func(addr, &chunk, sizeof(TSF_CHUNK));

    if ((chunk.magic != TSF_MAGIC) || (chunk.type != (u8)TSF_CHUNK_HEAD))
      continue;

    u8 stored_fnlen;
    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, fnlen), &stored_fnlen, sizeof(stored_fnlen));

    if (fnlen != stored_fnlen)
      continue;

    if (fnlen)
    {
      cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), cfg->buf, fnlen);
      cfg->buf[fnlen] = 0;

      if (memcmp(name, cfg->buf, fnlen))
        continue;
    }

    *chunk_addr = addr;

    return TSF_RES_OK;
  }

  return TSF_RES_FILE_NOT_FOUND;
}

TSF_RESULT tsf_open(TSF_VOLUME *vol, TSF_FILE *file, const char *name, u8 mode)
{
  if (!vol || !file || !name) return TSF_RES_FS_ERROR;

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
  size_t name_len = strlen(name);
  if (name_len > 255) return TSF_RES_FS_ERROR;

  TSF_CONFIG *cfg = vol->cfg;
  if (name_len && (!cfg->buf || (cfg->buf_size <= name_len))) return TSF_RES_FS_ERROR;
  if ((sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + name_len) > cfg->block_size)
    return TSF_RES_FS_ERROR;

  u8 fnlen = (u8)name_len;

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

  file->addr = addr;
  file->size = 0;
  file->seek = 0;
  file->chunk_addr = addr;
  file->chunk_offset = sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + fnlen;
  file->prev_chunk_addr = addr;
  file->next_chunk = 0xFFFF;

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
  if (!file || !file->vol || !file->vol->cfg) return TSF_RES_FS_ERROR;

  u8 *data = (u8*)buf;
  TSF_CONFIG *cfg = file->vol->cfg;
  u32 addr = file->chunk_addr;
  if (file->seek >= file->size) return TSF_RES_OK;
  size = min(size, file->size - file->seek);

  while (size)
  {
    if (file->chunk_offset == cfg->block_size)  // new chunk
    {
      if (file->next_chunk == 0xFFFF) return TSF_RES_FS_ERROR;

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
  if (!file || !file->vol || !file->vol->cfg) return TSF_RES_FS_ERROR;
  if (size && !buf) return TSF_RES_FS_ERROR;

  const u8 *data = (const u8*)buf;
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

  return TSF_RES_OK;
}

TSF_RESULT tsf_delete_chunk_range(TSF_VOLUME *vol, u32 addr, u32 blocks, u32 *done, u32 total)
{
  TSF_RESULT rc;
  u32 left;

  if (!vol || !vol->cfg || !blocks) return TSF_RES_FS_ERROR;

  if (vol->cfg->hal_erase_func)
  {
    left = blocks;
    while (left > 0)
    {
      rc = vol->cfg->hal_erase_func(addr);
      if (rc != TSF_RES_OK) return rc;

      rc = tsf_init_erased_chunk(vol->cfg, addr);
      if (rc != TSF_RES_OK) return rc;

      vol->free += vol->cfg->block_size;
      if (done) (*done)++;
      if (vol->cfg->progress_func) vol->cfg->progress_func(done ? *done : blocks - left + 1, total);

      addr += vol->cfg->block_size;
      left--;
    }

    return TSF_RES_OK;
  }

  rc = tsf_init_chunk_range(vol->cfg, addr, blocks * vol->cfg->block_size);
  if (rc != TSF_RES_OK)
    return rc;

  vol->free += blocks * vol->cfg->block_size;
  if (done) *done += blocks;
  if (vol->cfg->progress_func) vol->cfg->progress_func(done ? *done : blocks, total);

  return TSF_RES_OK;
}

TSF_RESULT tsf_delete(TSF_VOLUME *vol, const char *name)
{
  if (!vol || !vol->cfg || !name) return TSF_RES_FS_ERROR;

  u32 addr;
  u32 count_addr;
  u16 next;
  u32 range_addr = 0;
  u32 range_blocks = 0;
  u32 total_blocks = 0;
  u32 done_blocks = 0;
  TSF_CONFIG *cfg = vol->cfg;

  TSF_RESULT rc = tsf_search(vol, &addr, name);
  if (rc != TSF_RES_OK)
    return rc;

  count_addr = addr;
  do
  {
    cfg->hal_read_func(count_addr + offsetof(TSF_CHUNK, next_chunk), &next, sizeof(next));
    total_blocks++;
    if (next != 0xFFFF)
      count_addr = chunk_addr(next);
  } while (next != 0xFFFF);

  if (cfg->progress_func) cfg->progress_func(0, total_blocks);

  do
  {
    u32 cur_addr = addr;

    cfg->hal_read_func(addr + offsetof(TSF_CHUNK, next_chunk), &next, sizeof(next));

    if (!range_blocks)
    {
      range_addr = cur_addr;
      range_blocks = 1;
    }
    else if (cur_addr == range_addr + range_blocks * cfg->block_size)
    {
      range_blocks++;
    }
    else
    {
      rc = tsf_delete_chunk_range(vol, range_addr, range_blocks, &done_blocks, total_blocks);
      if (rc != TSF_RES_OK)
        return rc;

      range_addr = cur_addr;
      range_blocks = 1;
    }

    if (next != 0xFFFF)
      addr = chunk_addr(next);
  } while (next != 0xFFFF);

  rc = tsf_delete_chunk_range(vol, range_addr, range_blocks, &done_blocks, total_blocks);
  if (rc != TSF_RES_OK)
    return rc;

  if (vol->files_number)
    vol->files_number--;

  return TSF_RES_OK;
}

TSF_RESULT tsf_stat(TSF_VOLUME *vol, TSF_FILE_STAT *stat, const char *name)
{
  if (!vol || !stat || !name) return TSF_RES_FS_ERROR;

  u32 addr;

  TSF_RESULT rc = tsf_search(vol, &addr, name);
  if (rc != TSF_RES_OK)
    return rc;

  vol->cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, size), &stat->size, sizeof(stat->size));
  return TSF_RES_OK;
}

TSF_RESULT tsf_list(TSF_VOLUME *vol, u8 flag)
{
  if (!vol || !vol->cfg) return TSF_RES_FS_ERROR;

  TSF_CONFIG *cfg = vol->cfg;
  if (!cfg->buf || !cfg->buf_size) return TSF_RES_FS_ERROR;

  if (flag == TSF_LIST_START)
  {
    vol->list_addr = cfg->bulk_start;
    return TSF_RES_OK;
  }

  for (u32 addr = vol->list_addr; addr < (cfg->bulk_start + cfg->bulk_size); addr += cfg->block_size)
  {
    TSF_CHUNK chunk;
    cfg->hal_read_func(addr, &chunk, sizeof(TSF_CHUNK));

    if ((chunk.magic != TSF_MAGIC) || (chunk.type != (u8)TSF_CHUNK_HEAD))
      continue;

    u8 fnlen;
    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + offsetof(TSF_HDR, fnlen), &fnlen, sizeof(fnlen));
    fnlen = min(fnlen, cfg->buf_size - 1);
    cfg->hal_read_func(addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), cfg->buf, fnlen);
    cfg->buf[fnlen] = 0;

    vol->list_addr = addr + cfg->block_size;

    return TSF_RES_OK;
  }

  return TSF_RES_NO_MORE_FILES;
}
