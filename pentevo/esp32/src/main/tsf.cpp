#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_console.h"
#include "esp_partition.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_timer.h"
#include "tsf.h"

#define TSF_FORMAT_ERASE_RANGE_SIZE 65536U

bool tsf_storage_initialized = false;
bool tsf_storage_mounted = false;
char tsf_storage_base_path[32] = { 0 };
char tsf_storage_part_label[16] = { 0 };

const esp_partition_t *tsf_storage_part = NULL;
TSF_CONFIG tsf_storage_cfg = {};
TSF_VOLUME tsf_storage_vol = {};
u8 tsf_storage_hal_buf[TSF_HAL_BUF_SIZE] = {};

typedef struct
{
  bool used;
  TSF_FILE file;
  char name[256];
} TsfVfsFile;

typedef struct
{
  struct dirent entry;
  bool started;
} TsfVfsDir;

bool tsf_storage_vfs_registered = false;
TsfVfsFile tsf_vfs_files[TSF_VFS_MAX_FILES] = {};

const char *tsf_storage_subtype_name()
{
  return TSF_PART_SUBTYPE_NAME;
}

int tsf_storage_errno_from_err(esp_err_t err)
{
  switch (err)
  {
    case ESP_OK: return 0;
    case ESP_ERR_NOT_FOUND: return ENOENT;
    case ESP_ERR_NO_MEM: return ENOMEM;
    case ESP_ERR_INVALID_ARG: return EINVAL;
    case ESP_ERR_INVALID_SIZE: return ENAMETOOLONG;
    case ESP_ERR_INVALID_STATE: return EINVAL;
    case ESP_ERR_NOT_SUPPORTED: return ENOTSUP;
    default: return EIO;
  }
}

int tsf_storage_set_errno(esp_err_t err)
{
  errno = tsf_storage_errno_from_err(err);
  return -1;
}

int tsf_vfs_alloc_file()
{
  for (int i = 0; i < TSF_VFS_MAX_FILES; i++)
  {
    if (!tsf_vfs_files[i].used)
    {
      memset(&tsf_vfs_files[i], 0, sizeof(tsf_vfs_files[i]));
      tsf_vfs_files[i].used = true;
      return i;
    }
  }

  errno = EMFILE;
  return -1;
}

TsfVfsFile *tsf_vfs_get_file(int fd)
{
  if (fd < 0 || fd >= TSF_VFS_MAX_FILES) return NULL;
  if (!tsf_vfs_files[fd].used) return NULL;
  return &tsf_vfs_files[fd];
}

void tsf_vfs_free_file(int fd)
{
  if (fd < 0 || fd >= TSF_VFS_MAX_FILES) return;
  memset(&tsf_vfs_files[fd], 0, sizeof(tsf_vfs_files[fd]));
}

esp_err_t tsf_storage_require_volume(TSF_VOLUME **out_vol)
{
  esp_err_t err;
  TSF_VOLUME *vol;

  if (!out_vol) return ESP_ERR_INVALID_ARG;

  err = tsf_storage_ensure_ready_quiet();
  if (err != ESP_OK) return err;

  vol = tsf_storage_volume();
  if (!vol) return ESP_ERR_INVALID_STATE;

  *out_vol = vol;
  return ESP_OK;
}

int tsf_vfs_open(void *ctx, const char *path, int flags, int mode)
{
  char name[256];
  TSF_VOLUME *vol;
  TSF_RESULT res;
  esp_err_t err;
  int fd;
  int acc;

  (void)ctx;
  (void)mode;

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  err = tsf_storage_require_volume(&vol);
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  fd = tsf_vfs_alloc_file();
  if (fd < 0) return -1;

  acc = flags & O_ACCMODE;
  if (acc == O_RDONLY)
  {
    res = tsf_open(vol, &tsf_vfs_files[fd].file, name, TSF_MODE_READ);
  }
  else if (acc == O_WRONLY)
  {
    if ((flags & O_APPEND) || !(flags & O_CREAT))
    {
      tsf_vfs_free_file(fd);
      errno = ENOTSUP;
      return -1;
    }

    res = tsf_delete(vol, name);
    if (res != TSF_RES_OK && res != TSF_RES_FILE_NOT_FOUND)
    {
      tsf_vfs_free_file(fd);
      return tsf_storage_set_errno(tsf_storage_result_to_err(res));
    }

    res = tsf_open(vol, &tsf_vfs_files[fd].file, name, TSF_MODE_CREATE_WRITE);
  }
  else
  {
    tsf_vfs_free_file(fd);
    errno = ENOTSUP;
    return -1;
  }

  if (res != TSF_RES_OK)
  {
    tsf_vfs_free_file(fd);
    return tsf_storage_set_errno(tsf_storage_result_to_err(res));
  }

  snprintf(tsf_vfs_files[fd].name, sizeof(tsf_vfs_files[fd].name), "%s", name);
  return fd;
}

int tsf_vfs_close(void *ctx, int fd)
{
  TsfVfsFile *vf = tsf_vfs_get_file(fd);
  TSF_RESULT res;

  (void)ctx;

  if (!vf)
  {
    errno = EBADF;
    return -1;
  }

  res = tsf_close(&vf->file);
  tsf_vfs_free_file(fd);
  if (res != TSF_RES_OK) return tsf_storage_set_errno(tsf_storage_result_to_err(res));

  return 0;
}

ssize_t tsf_vfs_read(void *ctx, int fd, void *dst, size_t size)
{
  TsfVfsFile *vf = tsf_vfs_get_file(fd);
  TSF_RESULT res;
  u32 before;
  u32 todo;

  (void)ctx;

  if (!vf)
  {
    errno = EBADF;
    return -1;
  }
  if (!dst && size)
  {
    errno = EINVAL;
    return -1;
  }
  if (size > UINT32_MAX)
  {
    errno = EINVAL;
    return -1;
  }

  before = vf->file.seek;
  todo = (u32)size;
  res = tsf_read(&vf->file, dst, todo);
  if (res != TSF_RES_OK) return tsf_storage_set_errno(tsf_storage_result_to_err(res));

  return (ssize_t)(vf->file.seek - before);
}

ssize_t tsf_vfs_write(void *ctx, int fd, const void *src, size_t size)
{
  TsfVfsFile *vf = tsf_vfs_get_file(fd);
  TSF_RESULT res;
  u32 before;
  u32 todo;

  (void)ctx;

  if (!vf)
  {
    errno = EBADF;
    return -1;
  }
  if (!src && size)
  {
    errno = EINVAL;
    return -1;
  }
  if (size > UINT32_MAX)
  {
    errno = EINVAL;
    return -1;
  }
  if ((vf->file.mode & TSF_MODE_WRITE) != TSF_MODE_WRITE)
  {
    errno = EBADF;
    return -1;
  }

  before = vf->file.seek;
  todo = (u32)size;
  res = tsf_write(&vf->file, src, todo);
  if (res != TSF_RES_OK) return tsf_storage_set_errno(tsf_storage_result_to_err(res));

  return (ssize_t)(vf->file.seek - before);
}

off_t tsf_vfs_lseek(void *ctx, int fd, off_t offset, int whence)
{
  TsfVfsFile *vf = tsf_vfs_get_file(fd);
  TSF_VOLUME *vol;
  int64_t base;
  int64_t target;
  TSF_RESULT res;
  esp_err_t err;

  (void)ctx;

  if (!vf)
  {
    errno = EBADF;
    return (off_t)-1;
  }
  if ((vf->file.mode & TSF_MODE_WRITE) == TSF_MODE_WRITE)
  {
    errno = ENOTSUP;
    return (off_t)-1;
  }

  if (whence == SEEK_SET)
    base = 0;
  else if (whence == SEEK_CUR)
    base = (int64_t)vf->file.seek;
  else if (whence == SEEK_END)
    base = (int64_t)vf->file.size;
  else
  {
    errno = EINVAL;
    return (off_t)-1;
  }

  target = base + (int64_t)offset;
  if (target < 0 || target > (int64_t)vf->file.size)
  {
    errno = EINVAL;
    return (off_t)-1;
  }

  if ((u32)target < vf->file.seek)
  {
    err = tsf_storage_require_volume(&vol);
    if (err != ESP_OK) return (off_t)tsf_storage_set_errno(err);

    memset(&vf->file, 0, sizeof(vf->file));
    res = tsf_open(vol, &vf->file, vf->name, TSF_MODE_READ);
    if (res != TSF_RES_OK) return (off_t)tsf_storage_set_errno(tsf_storage_result_to_err(res));
  }

  res = tsf_seek(&vf->file, (u32)target - vf->file.seek);
  if (res != TSF_RES_OK) return (off_t)tsf_storage_set_errno(tsf_storage_result_to_err(res));

  return (off_t)vf->file.seek;
}

int tsf_vfs_fstat(void *ctx, int fd, struct stat *st)
{
  TsfVfsFile *vf = tsf_vfs_get_file(fd);

  (void)ctx;

  if (!vf)
  {
    errno = EBADF;
    return -1;
  }
  if (!st)
  {
    errno = EINVAL;
    return -1;
  }

  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFREG | 0666;
  st->st_size = (off_t)vf->file.size;
  return 0;
}

int tsf_vfs_stat(void *ctx, const char *path, struct stat *st)
{
  char name[256];
  TSF_VOLUME *vol;
  TSF_FILE_STAT tsf_st = {};
  TSF_RESULT res;
  esp_err_t err;

  (void)ctx;

  if (!st)
  {
    errno = EINVAL;
    return -1;
  }

  if (tsf_storage_path_is_root(path))
  {
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFDIR | 0555;
    st->st_size = 0;
    return 0;
  }

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  err = tsf_storage_require_volume(&vol);
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  res = tsf_stat(vol, &tsf_st, name);
  if (res != TSF_RES_OK) return tsf_storage_set_errno(tsf_storage_result_to_err(res));

  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFREG | 0666;
  st->st_size = (off_t)tsf_st.size;
  return 0;
}

int tsf_vfs_unlink(void *ctx, const char *path)
{
  char name[256];
  TSF_VOLUME *vol;
  TSF_RESULT res;
  esp_err_t err;

  (void)ctx;

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  err = tsf_storage_require_volume(&vol);
  if (err != ESP_OK) return tsf_storage_set_errno(err);

  res = tsf_delete(vol, name);
  if (res != TSF_RES_OK) return tsf_storage_set_errno(tsf_storage_result_to_err(res));

  return 0;
}

int tsf_vfs_rename(void *ctx, const char *src, const char *dst)
{
  (void)ctx;
  (void)src;
  (void)dst;

  errno = ENOTSUP;
  return -1;
}

DIR *tsf_vfs_opendir(void *ctx, const char *path)
{
  TsfVfsDir *dir;
  TSF_VOLUME *vol;
  TSF_RESULT res;
  esp_err_t err;

  (void)ctx;

  if (!tsf_storage_path_is_root(path))
  {
    errno = ENOTDIR;
    return NULL;
  }

  err = tsf_storage_require_volume(&vol);
  if (err != ESP_OK)
  {
    tsf_storage_set_errno(err);
    return NULL;
  }

  res = tsf_list(vol, TSF_LIST_START);
  if (res != TSF_RES_OK)
  {
    tsf_storage_set_errno(tsf_storage_result_to_err(res));
    return NULL;
  }

  dir = (TsfVfsDir*)calloc(1, sizeof(TsfVfsDir));
  if (!dir)
  {
    errno = ENOMEM;
    return NULL;
  }

  dir->started = true;
  return (DIR*)dir;
}

struct dirent *tsf_vfs_readdir(void *ctx, DIR *pdir)
{
  TsfVfsDir *dir = (TsfVfsDir*)pdir;
  TSF_VOLUME *vol;
  TSF_FILE_STAT st = {};
  TSF_RESULT res;
  esp_err_t err;

  (void)ctx;
  (void)st;

  if (!dir)
  {
    errno = EBADF;
    return NULL;
  }

  err = tsf_storage_require_volume(&vol);
  if (err != ESP_OK)
  {
    tsf_storage_set_errno(err);
    return NULL;
  }

  res = tsf_list(vol, TSF_LIST_NEXT);
  if (res == TSF_RES_NO_MORE_FILES)
  {
    errno = 0;
    return NULL;
  }
  if (res != TSF_RES_OK)
  {
    tsf_storage_set_errno(tsf_storage_result_to_err(res));
    return NULL;
  }

  memset(&dir->entry, 0, sizeof(dir->entry));
  snprintf(dir->entry.d_name, sizeof(dir->entry.d_name), "%s", (const char*)vol->cfg->buf);
#ifdef DT_REG
  dir->entry.d_type = DT_REG;
#endif
  return &dir->entry;
}

int tsf_vfs_closedir(void *ctx, DIR *pdir)
{
  (void)ctx;

  if (!pdir)
  {
    errno = EBADF;
    return -1;
  }

  free(pdir);
  return 0;
}

int tsf_vfs_mkdir(void *ctx, const char *path, mode_t mode)
{
  (void)ctx;
  (void)path;
  (void)mode;

  errno = ENOTSUP;
  return -1;
}

esp_err_t tsf_storage_register_vfs()
{
  if (tsf_storage_vfs_registered) return ESP_OK;

  esp_vfs_t vfs = {};
  vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
  vfs.open_p = tsf_vfs_open;
  vfs.close_p = tsf_vfs_close;
  vfs.read_p = tsf_vfs_read;
  vfs.write_p = tsf_vfs_write;
  vfs.lseek_p = tsf_vfs_lseek;
  vfs.fstat_p = tsf_vfs_fstat;
  vfs.stat_p = tsf_vfs_stat;
  vfs.unlink_p = tsf_vfs_unlink;
  vfs.rename_p = tsf_vfs_rename;
  vfs.opendir_p = tsf_vfs_opendir;
  vfs.readdir_p = tsf_vfs_readdir;
  vfs.closedir_p = tsf_vfs_closedir;
  vfs.mkdir_p = tsf_vfs_mkdir;

  esp_err_t err = esp_vfs_register(TSF_BASE_PATH, &vfs, NULL);
  if (err != ESP_OK) return err;

  tsf_storage_vfs_registered = true;
  return ESP_OK;
}

TSF_VOLUME *tsf_storage_volume()
{
  return tsf_storage_mounted ? &tsf_storage_vol : NULL;
}

const char *tsf_storage_result_name(TSF_RESULT res)
{
  switch (res)
  {
    case TSF_RES_OK: return "OK";
    case TSF_RES_FS_ERROR: return "FS_ERROR";
    case TSF_RES_FILE_NOT_FOUND: return "FILE_NOT_FOUND";
    case TSF_RES_FILE_EXISTS: return "FILE_EXISTS";
    case TSF_RES_MODE_ERROR: return "MODE_ERROR";
    case TSF_RES_BULK_FULL: return "BULK_FULL";
    case TSF_RES_NOT_BLANK: return "NOT_BLANK";
    case TSF_RES_NO_MORE_FILES: return "NO_MORE_FILES";
    default: return "UNKNOWN";
  }
}

esp_err_t tsf_storage_result_to_err(TSF_RESULT res)
{
  switch (res)
  {
    case TSF_RES_OK: return ESP_OK;
    case TSF_RES_FILE_NOT_FOUND: return ESP_ERR_NOT_FOUND;
    case TSF_RES_FILE_EXISTS: return ESP_ERR_INVALID_STATE;
    case TSF_RES_MODE_ERROR: return ESP_ERR_INVALID_ARG;
    case TSF_RES_BULK_FULL: return ESP_ERR_NO_MEM;
    case TSF_RES_NOT_BLANK: return ESP_ERR_INVALID_STATE;
    case TSF_RES_NO_MORE_FILES: return ESP_ERR_NOT_FOUND;
    case TSF_RES_FS_ERROR:
    default: return ESP_FAIL;
  }
}

TSF_RESULT tsf_storage_hal_read(u32 addr, void *dst, u32 size)
{
  if (!tsf_storage_part) return TSF_RES_FS_ERROR;
  esp_partition_read(tsf_storage_part, addr, dst, size);
  return TSF_RES_OK;
}

TSF_RESULT tsf_storage_hal_write(u32 addr, const void *src, u32 size)
{
  if (!tsf_storage_part) return TSF_RES_FS_ERROR;
  esp_partition_write(tsf_storage_part, addr, src, size);
  return TSF_RES_OK;
}

TSF_RESULT tsf_storage_hal_erase(u32 addr)
{
  if (!tsf_storage_part) return TSF_RES_FS_ERROR;
  esp_partition_erase_range(tsf_storage_part, addr, TSF_BLOCK_SIZE);
  return TSF_RES_OK;
}

TSF_RESULT tsf_storage_hal_erase_range(u32 addr, u32 size)
{
  if (!tsf_storage_part) return TSF_RES_FS_ERROR;
  esp_partition_erase_range(tsf_storage_part, addr, size);
  return TSF_RES_OK;
}

esp_err_t tsf_storage_find_partition()
{
  if (tsf_storage_part) return ESP_OK;

  tsf_storage_part = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    (esp_partition_subtype_t)TSF_PART_SUBTYPE_VALUE,
    TSF_PART_LABEL);

  return tsf_storage_part ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t tsf_storage_prepare_config()
{
  esp_err_t err;

  err = tsf_storage_find_partition();
  if (err != ESP_OK) return err;
  if (!tsf_storage_part) return ESP_ERR_NOT_FOUND;
  if ((tsf_storage_part->size % TSF_BLOCK_SIZE) != 0) return ESP_ERR_INVALID_SIZE;

  memset(&tsf_storage_cfg, 0, sizeof(tsf_storage_cfg));
  tsf_storage_cfg.bulk_start = 0;
  tsf_storage_cfg.bulk_size = (u32)tsf_storage_part->size;
  tsf_storage_cfg.block_size = TSF_BLOCK_SIZE;
  tsf_storage_cfg.hal_read_func = tsf_storage_hal_read;
  tsf_storage_cfg.hal_write_func = tsf_storage_hal_write;
  tsf_storage_cfg.hal_erase_func = tsf_storage_hal_erase;
  tsf_storage_cfg.hal_erase_range_func = tsf_storage_hal_erase_range;
  tsf_storage_cfg.buf = tsf_storage_hal_buf;
  tsf_storage_cfg.buf_size = sizeof(tsf_storage_hal_buf);

  return ESP_OK;
}

esp_err_t tsf_storage_mount_common(bool quiet)
{
  esp_err_t err;
  TSF_RESULT res;

  err = tsf_storage_register_vfs();
  if (err != ESP_OK) return err;

  if (tsf_storage_mounted)
  {
    tsf_storage_initialized = true;
    return ESP_OK;
  }

  err = tsf_storage_prepare_config();
  if (err != ESP_OK)
  {
    if (!quiet)
      printf("E: TSF partition '%s' subtype=%s (0x%02X) not found: %s\r\n",
        TSF_PART_LABEL,
        tsf_storage_subtype_name(),
        TSF_PART_SUBTYPE_VALUE,
        esp_err_to_name(err));
    return err;
  }

  memset(&tsf_storage_vol, 0, sizeof(tsf_storage_vol));
  res = tsf_mount(&tsf_storage_cfg, &tsf_storage_vol);
  if (res != TSF_RES_OK)
  {
    if (!quiet)
      printf("E: tsf_mount failed: %s\r\n", tsf_storage_result_name(res));
    return tsf_storage_result_to_err(res);
  }

  if (tsf_storage_vol.chunks_number == 0)
  {
    if (!quiet)
      printf("E: TSF is not formatted\r\n");
    return ESP_ERR_INVALID_STATE;
  }

  tsf_storage_initialized = true;
  tsf_storage_mounted = true;
  snprintf(tsf_storage_base_path, sizeof(tsf_storage_base_path), "%s", TSF_BASE_PATH);
  snprintf(tsf_storage_part_label, sizeof(tsf_storage_part_label), "%s", TSF_PART_LABEL);

  return ESP_OK;
}

esp_err_t tsf_storage_mount()
{
  return tsf_storage_mount_common(false);
}

esp_err_t tsf_storage_mount_quiet()
{
  return tsf_storage_mount_common(true);
}

void tsf_storage_print_format_progress(const char *stage, u32 done, u32 total, uint64_t bytes_done, int64_t start_us)
{
  const u32 bar_width = 32;
  u32 percent = total ? (done * 100U) / total : 100U;
  u32 filled = total ? (done * bar_width) / total : bar_width;
  int64_t elapsed_us = esp_timer_get_time() - start_us;
  uint32_t kib_s = 0;

  if (elapsed_us > 0)
    kib_s = (uint32_t)((bytes_done * 1000000ULL) / (uint64_t)elapsed_us / 1024ULL);

  printf("\rTSF format %s: [", stage ? stage : "");
  for (u32 i = 0; i < bar_width; i++)
    putchar((i < filled) ? '#' : '.');
  printf("] %3" PRIu32 "%% %" PRIu32 "/%" PRIu32 " %" PRIu32 " KiB/s", percent, done, total, kib_s);
  fflush(stdout);
}

TSF_RESULT tsf_storage_format_with_progress()
{
  u32 total_blocks;
  u32 total_ranges;
  u32 last_percent;
  int64_t start_us;

  if (!tsf_storage_cfg.block_size || !tsf_storage_cfg.bulk_size) return TSF_RES_FS_ERROR;
  if ((tsf_storage_cfg.bulk_size % tsf_storage_cfg.block_size) != 0) return TSF_RES_FS_ERROR;
  if (!tsf_storage_cfg.hal_erase_range_func) return TSF_RES_FS_ERROR;

  total_blocks = tsf_storage_cfg.bulk_size / tsf_storage_cfg.block_size;
  total_ranges = (tsf_storage_cfg.bulk_size + TSF_FORMAT_ERASE_RANGE_SIZE - 1) / TSF_FORMAT_ERASE_RANGE_SIZE;
  start_us = esp_timer_get_time();

  printf("TSF format erase: %" PRIu32 " bytes in %" PRIu32 " ranges\r\n",
    tsf_storage_cfg.bulk_size,
    total_ranges);

  last_percent = UINT32_MAX;
  for (u32 done = 0, erased = 0; erased < tsf_storage_cfg.bulk_size; done++)
  {
    u32 size = tsf_storage_cfg.bulk_size - erased;
    u32 percent;
    TSF_RESULT res;

    if (size > TSF_FORMAT_ERASE_RANGE_SIZE)
      size = TSF_FORMAT_ERASE_RANGE_SIZE;

    res = tsf_storage_cfg.hal_erase_range_func(tsf_storage_cfg.bulk_start + erased, size);
    if (res != TSF_RES_OK)
    {
      printf("\r\n");
      return res;
    }

    erased += size;
    percent = total_ranges ? ((done + 1) * 100U) / total_ranges : 100U;
    if (percent != last_percent || erased == tsf_storage_cfg.bulk_size)
    {
      tsf_storage_print_format_progress("erase", done + 1, total_ranges, erased, start_us);
      last_percent = percent;
    }
  }

  printf("\r\nTSF format write magic: %" PRIu32 " blocks, %" PRIu32 " bytes each\r\n",
    total_blocks,
    tsf_storage_cfg.block_size);

  last_percent = UINT32_MAX;
  for (u32 block = 0; block < total_blocks; block++)
  {
    u32 done;
    u32 percent;
    TSF_RESULT res;

    res = tsf_init_erased_chunk(&tsf_storage_cfg, tsf_storage_cfg.bulk_start + block * tsf_storage_cfg.block_size);
    if (res != TSF_RES_OK)
    {
      printf("\r\n");
      return res;
    }

    done = block + 1;
    percent = total_blocks ? (done * 100U) / total_blocks : 100U;
    if (percent != last_percent || done == total_blocks)
    {
      tsf_storage_print_format_progress("magic", done, total_blocks, (uint64_t)done * tsf_storage_cfg.block_size, start_us);
      last_percent = percent;
    }
  }

  printf("\r\n");
  return TSF_RES_OK;
}

esp_err_t tsf_storage_format()
{
  esp_err_t err;
  TSF_RESULT res;

  tsf_storage_deinit();

  err = tsf_storage_prepare_config();
  if (err != ESP_OK)
  {
    printf("E: TSF partition '%s' subtype=%s (0x%02X) not found: %s\r\n",
      TSF_PART_LABEL,
      tsf_storage_subtype_name(),
      TSF_PART_SUBTYPE_VALUE,
      esp_err_to_name(err));
    return err;
  }

  res = tsf_storage_format_with_progress();
  if (res != TSF_RES_OK)
  {
    printf("E: tsf_format failed: %s\r\n", tsf_storage_result_name(res));
    return tsf_storage_result_to_err(res);
  }

  return tsf_storage_mount();
}

esp_err_t tsf_storage_init()
{
  return tsf_storage_mount();
}

void tsf_storage_deinit()
{
  tsf_storage_initialized = false;
  tsf_storage_mounted = false;
  tsf_storage_base_path[0] = 0;
  tsf_storage_part_label[0] = 0;
  memset(&tsf_storage_vol, 0, sizeof(tsf_storage_vol));
}

esp_err_t tsf_storage_ensure_ready()
{
  return tsf_storage_mount();
}

esp_err_t tsf_storage_ensure_ready_quiet()
{
  return tsf_storage_mount_quiet();
}

bool tsf_storage_path_is_root(const char *path)
{
  if (!path || !path[0]) return true;
  if (strcmp(path, "/") == 0) return true;
  if (strcmp(path, TSF_BASE_PATH) == 0) return true;
  return false;
}

bool tsf_storage_path_has_base(const char *path)
{
  size_t base_len;

  if (!path) return false;

  base_len = strlen(TSF_BASE_PATH);
  if (strncmp(path, TSF_BASE_PATH, base_len) != 0) return false;

  return path[base_len] == 0 || path[base_len] == '/';
}

esp_err_t tsf_storage_name_from_path(const char *path, char *out, size_t out_size)
{
  const char *name = path;
  size_t base_len;
  size_t len;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;
  out[0] = 0;
  if (!path || !path[0]) return ESP_ERR_INVALID_ARG;

  base_len = strlen(TSF_BASE_PATH);
  if (strncmp(name, TSF_BASE_PATH, base_len) == 0 && (name[base_len] == 0 || name[base_len] == '/'))
    name += base_len;

  while (*name == '/')
    name++;

  if (!name[0]) return ESP_ERR_INVALID_ARG;
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return ESP_ERR_INVALID_ARG;
  if (strchr(name, '/')) return ESP_ERR_NOT_SUPPORTED;

  len = strlen(name);
  if (len >= out_size) return ESP_ERR_INVALID_SIZE;
  if (len > 255) return ESP_ERR_INVALID_SIZE;

  memcpy(out, name, len);
  out[len] = 0;
  return ESP_OK;
}

esp_err_t tsf_storage_space_info(uint64_t *total_bytes, uint64_t *free_bytes)
{
  esp_err_t err;

  if (!total_bytes || !free_bytes) return ESP_ERR_INVALID_ARG;

  err = tsf_storage_ensure_ready_quiet();
  if (err != ESP_OK) return err;

  *total_bytes = tsf_storage_cfg.bulk_size;
  *free_bytes = tsf_storage_vol.free;
  return ESP_OK;
}

esp_err_t tsf_storage_open_read(const char *path, TSF_FILE *file, size_t *out_size)
{
  char name[256];
  TSF_VOLUME *vol;
  TSF_FILE_STAT st = {};
  TSF_RESULT res;
  esp_err_t err;

  if (out_size) *out_size = 0;
  if (!path || !path[0] || !file) return ESP_ERR_INVALID_ARG;

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return err;

  err = tsf_storage_ensure_ready_quiet();
  if (err != ESP_OK) return err;

  vol = tsf_storage_volume();
  if (!vol) return ESP_ERR_INVALID_STATE;

  res = tsf_stat(vol, &st, name);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  memset(file, 0, sizeof(*file));
  res = tsf_open(vol, file, name, TSF_MODE_READ);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  if (out_size) *out_size = st.size;
  return ESP_OK;
}

esp_err_t tsf_storage_read_file(const char *path, void *dst, size_t size, size_t *out_size)
{
  TSF_FILE file = {};
  size_t file_size = 0;
  u32 before;
  TSF_RESULT res;
  esp_err_t err;

  if (out_size) *out_size = 0;
  if (!path || !path[0]) return ESP_ERR_INVALID_ARG;
  if (!dst && size) return ESP_ERR_INVALID_ARG;
  if (size > UINT32_MAX) return ESP_ERR_INVALID_SIZE;

  err = tsf_storage_open_read(path, &file, &file_size);
  if (err != ESP_OK) return err;

  if (!dst)
  {
    if (out_size) *out_size = file_size;
    tsf_close(&file);
    return ESP_OK;
  }

  if (size > file_size)
  {
    tsf_close(&file);
    return ESP_ERR_INVALID_SIZE;
  }

  before = file.seek;
  res = tsf_read(&file, dst, (u32)size);
  if (out_size) *out_size = (size_t)(file.seek - before);
  tsf_close(&file);

  return tsf_storage_result_to_err(res);
}

esp_err_t tsf_storage_list_dir(const char *path)
{
  esp_err_t err;
  TSF_RESULT res;

  if (!tsf_storage_path_is_root(path)) return ESP_ERR_NOT_SUPPORTED;

  err = tsf_storage_ensure_ready();
  if (err != ESP_OK) return err;

  printf("Listing: %s\r\n", TSF_BASE_PATH);

  res = tsf_list(&tsf_storage_vol, TSF_LIST_START);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  for (;;)
  {
    TSF_FILE_STAT st = {};

    res = tsf_list(&tsf_storage_vol, TSF_LIST_NEXT);
    if (res == TSF_RES_NO_MORE_FILES) break;
    if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

    tsf_stat(&tsf_storage_vol, &st, (const char *)tsf_storage_cfg.buf);
    printf("  FILE  %10" PRIu32 "  %s\r\n", st.size, (const char *)tsf_storage_cfg.buf);
  }

  return ESP_OK;
}

int tsf_storage_info_cmd(int argc, char **argv)
{
  esp_err_t err;

  if (argc < 0 || !argv) return 1;

  err = tsf_storage_ensure_ready();
  if (err != ESP_OK)
  {
    printf("E: TSF mount failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("TSF info\r\n");
  printf("  mounted    : %s\r\n", tsf_storage_mounted ? "yes" : "no");
  printf("  label      : %s\r\n", TSF_PART_LABEL);
  printf("  subtype    : %s (0x%02X)\r\n", tsf_storage_subtype_name(), TSF_PART_SUBTYPE_VALUE);
  printf("  base path  : %s\r\n", TSF_BASE_PATH);
  printf("  block size : %u bytes\r\n", (unsigned)tsf_storage_cfg.block_size);
  printf("  blocks     : %u\r\n", (unsigned)(tsf_storage_cfg.bulk_size / tsf_storage_cfg.block_size));
  printf("  files      : %u\r\n", (unsigned)tsf_storage_vol.files_number);
  printf("  total      : %u bytes\r\n", (unsigned)tsf_storage_cfg.bulk_size);
  printf("  free       : %u bytes\r\n", (unsigned)tsf_storage_vol.free);

  if (tsf_storage_part)
  {
    printf("  address    : 0x%08" PRIX32 "\r\n", tsf_storage_part->address);
    printf("  size       : 0x%08" PRIX32 " (%" PRIu32 " bytes)\r\n",
      tsf_storage_part->size,
      tsf_storage_part->size);
  }

  return 0;
}

int tsf_storage_format_cmd(int argc, char **argv)
{
  esp_err_t err;

  if (argc < 0 || !argv) return 1;

  err = tsf_storage_format();
  if (err != ESP_OK) return 1;

  printf("TSF formatted and mounted: %s subtype=%s (0x%02X)\r\n", TSF_PART_LABEL, tsf_storage_subtype_name(), TSF_PART_SUBTYPE_VALUE);
  return 0;
}

int tsf_storage_ls_cmd(int argc, char **argv)
{
  const char *path = "/";
  esp_err_t err;

  if (argc >= 3)
    path = argv[2];

  err = tsf_storage_list_dir(path);
  return (err == ESP_OK) ? 0 : 1;
}

int tsf_storage_cmd(int argc, char **argv)
{
  const char *op;

  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  tsf format\r\n");
    printf("  tsf info\r\n");
    printf("  tsf ls [path]\r\n");
    return 0;
  }

  op = argv[1];

  if (!strcmp(op, "format"))
    return tsf_storage_format_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return tsf_storage_info_cmd(argc, argv);

  if (!strcmp(op, "ls"))
    return tsf_storage_ls_cmd(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void tsf_console_register_system_commands()
{
  ESP_ERROR_CHECK(tsf_storage_register_vfs());

  const esp_console_cmd_t cmd =
  {
    .command  = "tsf",
    .help = "TSF commands: format/info/ls",
    .hint     = NULL,
    .func     = &tsf_storage_cmd,
    .argtable = NULL
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
