#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CmdApp CmdApp;
typedef struct CmdFsManager CmdFsManager;
typedef struct CmdPanel CmdPanel;

void cmd_console_register_system_commands();
void cmd_set_status(CmdApp *app, const char *fmt, ...);
void cmd_show_error(CmdApp *app, const char *title, esp_err_t err);

#ifdef __cplusplus
}
#endif

// --------------- cmd_types.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_TEXT_COLS           80
#define CMD_TEXT_ROWS           30
#define CMD_FILE_NAME_MAX       256
#define CMD_PATH_MAX            256
#define CMD_PANEL_COUNT         2

#ifndef CMD_ERR_BASE
#define CMD_ERR_BASE            0x7000
#endif

typedef esp_err_t CmdErr;

typedef enum
{
  CMD_DEVICE_SD = 0,
  CMD_DEVICE_FAT,
  CMD_DEVICE_TSF,
  CMD_DEVICE_MAX,
  CMD_DEVICE_INVALID = -1
} cmd_device_id_t;

typedef enum
{
  CMD_ENTRY_UNKNOWN = 0,
  CMD_ENTRY_FILE,
  CMD_ENTRY_DIR,
  CMD_ENTRY_PARENT
} cmd_entry_type_t;

typedef enum
{
  CMD_ENTRY_FLAG_NONE       = 0,
  CMD_ENTRY_FLAG_SELECTED   = 1u << 0,
  CMD_ENTRY_FLAG_ERROR      = 1u << 1
} cmd_entry_flags_t;

typedef struct
{
  char name[CMD_FILE_NAME_MAX];
  cmd_entry_type_t type;
  uint64_t size;
  uint64_t mtime;
  uint32_t flags;
} cmd_file_entry_t;

typedef struct
{
  int x;
  int y;
} cmd_point_t;

typedef struct
{
  int w;
  int h;
} cmd_size_t;

typedef struct
{
  int x;
  int y;
  int w;
  int h;
} cmd_rect_t;

typedef enum
{
  CMD_KEY_NONE = 0,
  CMD_KEY_CHAR,
  CMD_KEY_ESC,
  CMD_KEY_ENTER,
  CMD_KEY_BACKSPACE,
  CMD_KEY_TAB,
  CMD_KEY_UP,
  CMD_KEY_DOWN,
  CMD_KEY_LEFT,
  CMD_KEY_RIGHT,
  CMD_KEY_HOME,
  CMD_KEY_END,
  CMD_KEY_PAGE_UP,
  CMD_KEY_PAGE_DOWN,
  CMD_KEY_INSERT,
  CMD_KEY_DELETE,
  CMD_KEY_F1,
  CMD_KEY_F2,
  CMD_KEY_F3,
  CMD_KEY_F4,
  CMD_KEY_F5,
  CMD_KEY_F6,
  CMD_KEY_F7,
  CMD_KEY_F8,
  CMD_KEY_F9,
  CMD_KEY_F10,
  CMD_KEY_F11,
  CMD_KEY_F12,
  CMD_KEY_UNKNOWN
} cmd_key_t;

typedef enum
{
  CMD_EVENT_NONE = 0,
  CMD_EVENT_KEY,
  CMD_EVENT_QUIT
} cmd_event_type_t;

typedef struct
{
  cmd_event_type_t type;
  cmd_key_t key;
  uint32_t ch;
  uint32_t mods;
} cmd_event_t;

typedef enum
{
  CMD_KEY_MOD_NONE  = 0,
  CMD_KEY_MOD_SHIFT = 1u << 0,
  CMD_KEY_MOD_CTRL  = 1u << 1,
  CMD_KEY_MOD_ALT   = 1u << 2
} cmd_key_mod_t;

typedef enum
{
  CMD_OK = ESP_OK,
  CMD_ERR_INVALID_DEVICE = CMD_ERR_BASE,
  CMD_ERR_NOT_MOUNTED,
  CMD_ERR_PATH_TOO_LONG,
  CMD_ERR_ENTRY_TOO_LONG,
  CMD_ERR_DIR_FULL,
  CMD_ERR_INPUT_UNAVAILABLE,
  CMD_ERR_CANCELLED
} cmd_err_t;

const char *cmd_device_name(cmd_device_id_t id);
const char *cmd_entry_type_name(cmd_entry_type_t type);

#ifdef __cplusplus
}
#endif

// --------------- cmd_display.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_DISPLAY_DEFAULT_COLS         CMD_TEXT_COLS
#define CMD_DISPLAY_DEFAULT_ROWS         CMD_TEXT_ROWS
#define CMD_DISPLAY_MAX_COLS             160
#define CMD_DISPLAY_MAX_ROWS             64
#define CMD_DISPLAY_COLS                 CMD_DISPLAY_MAX_COLS
#define CMD_DISPLAY_ROWS                 CMD_DISPLAY_MAX_ROWS
#define CMD_DISPLAY_CELL_COUNT           (CMD_DISPLAY_COLS * CMD_DISPLAY_ROWS)
#define CMD_DISPLAY_MUX_MAX_BACKENDS     4
#define CMD_DISPLAY_BACKEND_FLAG_FORCE_PARTIAL 1u

typedef uint16_t cmd_display_attr_t;

typedef enum
{
  CMD_DISPLAY_COLOR_BLACK = 0,
  CMD_DISPLAY_COLOR_BLUE,
  CMD_DISPLAY_COLOR_GREEN,
  CMD_DISPLAY_COLOR_CYAN,
  CMD_DISPLAY_COLOR_RED,
  CMD_DISPLAY_COLOR_MAGENTA,
  CMD_DISPLAY_COLOR_YELLOW,
  CMD_DISPLAY_COLOR_WHITE,
  CMD_DISPLAY_COLOR_BRIGHT_BLACK,
  CMD_DISPLAY_COLOR_BRIGHT_BLUE,
  CMD_DISPLAY_COLOR_BRIGHT_GREEN,
  CMD_DISPLAY_COLOR_BRIGHT_CYAN,
  CMD_DISPLAY_COLOR_BRIGHT_RED,
  CMD_DISPLAY_COLOR_BRIGHT_MAGENTA,
  CMD_DISPLAY_COLOR_BRIGHT_YELLOW,
  CMD_DISPLAY_COLOR_BRIGHT_WHITE
} cmd_display_color_t;

typedef enum
{
  CMD_DISPLAY_ATTR_FLAG_NONE      = 0,
  CMD_DISPLAY_ATTR_FLAG_BOLD      = 1u << 0,
  CMD_DISPLAY_ATTR_FLAG_INVERSE   = 1u << 1,
  CMD_DISPLAY_ATTR_FLAG_UNDERLINE = 1u << 2,
  CMD_DISPLAY_ATTR_FLAG_SELECTED  = 1u << 3
} cmd_display_attr_flag_t;

typedef enum
{
  CMD_DISPLAY_MODE_CONSOLE = 0,
  CMD_DISPLAY_MODE_FT812,
  CMD_DISPLAY_MODE_MIRROR
} CmdDisplayMode;

#define CMD_DISPLAY_ATTR_FG_MASK         0x000f
#define CMD_DISPLAY_ATTR_BG_MASK         0x00f0
#define CMD_DISPLAY_ATTR_FLAGS_MASK      0xff00
#define CMD_DISPLAY_ATTR_BG_SHIFT        4
#define CMD_DISPLAY_ATTR_FLAGS_SHIFT     8
#define CMD_DISPLAY_ATTR_DEFAULT         ((cmd_display_attr_t)(CMD_DISPLAY_COLOR_WHITE | (CMD_DISPLAY_COLOR_BLACK << CMD_DISPLAY_ATTR_BG_SHIFT)))

#define CMD_DISPLAY_BOX_TL               ((char)0xC9)
#define CMD_DISPLAY_BOX_TR               ((char)0xBB)
#define CMD_DISPLAY_BOX_BL               ((char)0xC8)
#define CMD_DISPLAY_BOX_BR               ((char)0xBC)
#define CMD_DISPLAY_BOX_H                ((char)0xCD)
#define CMD_DISPLAY_BOX_V                ((char)0xBA)

typedef struct
{
  char ch;
  cmd_display_attr_t attr;
} CmdDisplayCell;

typedef struct
{
  cmd_size_t size;
  CmdDisplayCell cells[CMD_DISPLAY_ROWS][CMD_DISPLAY_COLS];
} CmdDisplayBuffer;

typedef struct CmdDisplayBackend CmdDisplayBackend;
typedef struct CmdDisplayMux CmdDisplayMux;

typedef esp_err_t (*CmdDisplayBackendInitFn)(CmdDisplayBackend *backend);
typedef void (*CmdDisplayBackendDeinitFn)(CmdDisplayBackend *backend);
typedef esp_err_t (*CmdDisplayBackendClearFn)(CmdDisplayBackend *backend);
typedef esp_err_t (*CmdDisplayBackendPresentFn)(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer);
typedef esp_err_t (*CmdDisplayBackendPutCharFn)(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr);
typedef esp_err_t (*CmdDisplayBackendWriteTextFn)(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr);
typedef esp_err_t (*CmdDisplayBackendGetSizeFn)(CmdDisplayBackend *backend, cmd_size_t *size);

struct CmdDisplayBackend
{
  const char *name;
  void *ctx;
  uint32_t flags;

  CmdDisplayBackendInitFn init;
  CmdDisplayBackendDeinitFn deinit;
  CmdDisplayBackendClearFn clear;
  CmdDisplayBackendPresentFn present;
  CmdDisplayBackendPutCharFn put_char;
  CmdDisplayBackendWriteTextFn write_text;
  CmdDisplayBackendGetSizeFn get_size;
};

struct CmdDisplayMux
{
  CmdDisplayBackend *backends[CMD_DISPLAY_MUX_MAX_BACKENDS];
  size_t count;
};

extern CmdDisplayBackend cmd_display_console_backend;
extern CmdDisplayBackend cmd_display_ft812_backend;

cmd_display_attr_t cmd_display_make_attr(cmd_display_color_t fg, cmd_display_color_t bg, uint8_t flags);
cmd_display_color_t cmd_display_attr_fg(cmd_display_attr_t attr);
cmd_display_color_t cmd_display_attr_bg(cmd_display_attr_t attr);
uint8_t cmd_display_attr_flags(cmd_display_attr_t attr);

const char *cmd_display_mode_name(CmdDisplayMode mode);
bool cmd_display_mode_parse(const char *text, CmdDisplayMode *mode);
bool cmd_display_mode_has_console(CmdDisplayMode mode);
bool cmd_display_mode_has_ft812(CmdDisplayMode mode);

void cmd_display_buffer_init(CmdDisplayBuffer *buffer);
esp_err_t cmd_display_buffer_set_size(CmdDisplayBuffer *buffer, int w, int h);
void cmd_display_buffer_clear(CmdDisplayBuffer *buffer, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_put_char(CmdDisplayBuffer *buffer, int x, int y, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_write_text(CmdDisplayBuffer *buffer, int x, int y, const char *text, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_fill_rect(CmdDisplayBuffer *buffer, const cmd_rect_t *rect, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_draw_hline(CmdDisplayBuffer *buffer, int x, int y, int w, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_draw_vline(CmdDisplayBuffer *buffer, int x, int y, int h, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_buffer_draw_box(CmdDisplayBuffer *buffer, const cmd_rect_t *rect, cmd_display_attr_t attr);
CmdDisplayCell *cmd_display_buffer_cell(CmdDisplayBuffer *buffer, int x, int y);
const CmdDisplayCell *cmd_display_buffer_cell_const(const CmdDisplayBuffer *buffer, int x, int y);
bool cmd_display_point_inside(int x, int y);
bool cmd_display_buffer_point_inside(const CmdDisplayBuffer *buffer, int x, int y);

esp_err_t cmd_display_backend_init(CmdDisplayBackend *backend);
void cmd_display_backend_deinit(CmdDisplayBackend *backend);
esp_err_t cmd_display_backend_clear(CmdDisplayBackend *backend);
esp_err_t cmd_display_backend_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer);
esp_err_t cmd_display_backend_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_backend_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr);
esp_err_t cmd_display_backend_get_size(CmdDisplayBackend *backend, cmd_size_t *size);

void cmd_display_mux_init(CmdDisplayMux *mux);
esp_err_t cmd_display_mux_add_backend(CmdDisplayMux *mux, CmdDisplayBackend *backend);
esp_err_t cmd_display_mux_add_mode(CmdDisplayMux *mux, CmdDisplayMode mode);
esp_err_t cmd_display_mux_remove_backend(CmdDisplayMux *mux, CmdDisplayBackend *backend);
esp_err_t cmd_display_mux_init_backends(CmdDisplayMux *mux);
esp_err_t cmd_display_mux_get_size(CmdDisplayMux *mux, cmd_size_t *size);
void cmd_display_mux_deinit_backends(CmdDisplayMux *mux);
esp_err_t cmd_display_mux_clear(CmdDisplayMux *mux);
esp_err_t cmd_display_mux_present(CmdDisplayMux *mux, const CmdDisplayBuffer *buffer);
esp_err_t cmd_display_mux_put_char(CmdDisplayMux *mux, int x, int y, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_mux_write_text(CmdDisplayMux *mux, int x, int y, const char *text, cmd_display_attr_t attr);

esp_err_t cmd_display_console_init(CmdDisplayBackend *backend);
void cmd_display_console_deinit(CmdDisplayBackend *backend);
esp_err_t cmd_display_console_clear(CmdDisplayBackend *backend);
esp_err_t cmd_display_console_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer);
esp_err_t cmd_display_console_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_console_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr);
esp_err_t cmd_display_console_get_size(CmdDisplayBackend *backend, cmd_size_t *size);

esp_err_t cmd_display_ft812_begin(CmdDisplayBackend *backend);
void cmd_display_ft812_end(CmdDisplayBackend *backend);
esp_err_t cmd_display_ft812_clear(CmdDisplayBackend *backend);
esp_err_t cmd_display_ft812_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer);
esp_err_t cmd_display_ft812_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr);
esp_err_t cmd_display_ft812_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr);
esp_err_t cmd_display_ft812_get_size(CmdDisplayBackend *backend, cmd_size_t *size);

#ifdef __cplusplus
}
#endif

// --------------- cmd_fileops.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CMD_FILEOPS_BUFFER_SIZE
#define CMD_FILEOPS_BUFFER_SIZE 4096
#endif

#ifndef CMD_FILEOPS_MAX_DEPTH
#define CMD_FILEOPS_MAX_DEPTH 16
#endif

typedef enum
{
  CMD_FILEOPS_OP_COPY = 0,
  CMD_FILEOPS_OP_MOVE,
  CMD_FILEOPS_OP_DELETE
} cmd_fileops_op_t;

typedef enum
{
  CMD_FILEOPS_FLAG_NONE      = 0,
  CMD_FILEOPS_FLAG_RECURSIVE = 1u << 0,
  CMD_FILEOPS_FLAG_OVERWRITE = 1u << 1
} cmd_fileops_flags_t;

typedef struct
{
  cmd_fileops_op_t op;
  const char *filename;
  uint64_t bytes_done;
  uint64_t bytes_total;
  size_t files_done;
  size_t files_total;
  bool can_cancel;
} CmdFileopsProgress;

typedef bool (*CmdFileopsProgressFn)(const CmdFileopsProgress *progress, void *ctx);

typedef struct
{
  uint32_t flags;
  CmdFileopsProgressFn progress;
  void *progress_ctx;
} CmdFileopsOptions;

esp_err_t cmd_fileops_alloc_global_buffers();
void cmd_fileops_free_global_buffers();

esp_err_t cmd_fileops_copy_path(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_copy_file(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_move_path(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_delete_path(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_copy_selected(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_move_selected(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options);

esp_err_t cmd_fileops_delete_selected(CmdPanel *panel,
  const CmdFileopsOptions *options);

#ifdef __cplusplus
}
#endif

// --------------- cmd_fs.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_FS_DRIVER_MAX        CMD_DEVICE_MAX

#ifndef CMD_FS_SD_ROOT_PATH
#define CMD_FS_SD_ROOT_PATH      "/sd"
#endif

typedef struct CmdFsDriver CmdFsDriver;
typedef struct CmdFsManager CmdFsManager;
typedef struct CmdFsList CmdFsList;
typedef struct CmdFsFile CmdFsFile;

typedef esp_err_t (*CmdFsMountFn)(CmdFsDriver *driver);
typedef esp_err_t (*CmdFsUnmountFn)(CmdFsDriver *driver);
typedef esp_err_t (*CmdFsListBeginFn)(CmdFsDriver *driver, const char *path, CmdFsList *list);
typedef esp_err_t (*CmdFsListNextFn)(CmdFsDriver *driver, CmdFsList *list, cmd_file_entry_t *entry);
typedef void (*CmdFsListCloseFn)(CmdFsDriver *driver, CmdFsList *list);
typedef esp_err_t (*CmdFsStatFn)(CmdFsDriver *driver, const char *path, cmd_file_entry_t *entry);
typedef esp_err_t (*CmdFsMkdirFn)(CmdFsDriver *driver, const char *path);
typedef esp_err_t (*CmdFsRemoveFn)(CmdFsDriver *driver, const char *path);
typedef esp_err_t (*CmdFsRenameFn)(CmdFsDriver *driver, const char *old_path, const char *new_path);
typedef esp_err_t (*CmdFsOpenFn)(CmdFsDriver *driver, const char *path, CmdFsFile *file);
typedef esp_err_t (*CmdFsReadFn)(CmdFsDriver *driver, CmdFsFile *file, void *dst, size_t size, size_t *out_size);
typedef esp_err_t (*CmdFsWriteFn)(CmdFsDriver *driver, CmdFsFile *file, const void *src, size_t size, size_t *out_size);
typedef esp_err_t (*CmdFsSeekFn)(CmdFsDriver *driver, CmdFsFile *file, int64_t offset, int whence);
typedef esp_err_t (*CmdFsTellFn)(CmdFsDriver *driver, CmdFsFile *file, uint64_t *out_pos);
typedef esp_err_t (*CmdFsCloseFn)(CmdFsDriver *driver, CmdFsFile *file);

struct CmdFsList
{
  cmd_device_id_t device_id;
  char path[CMD_PATH_MAX];
  char abs_path[CMD_PATH_MAX];
  void *dir;
  size_t index;
  bool parent_pending;
  uint32_t flags;
};

struct CmdFsFile
{
  cmd_device_id_t device_id;
  FILE *fp;
  void *ctx;
  char name[CMD_FILE_NAME_MAX];
  uint32_t flags;
};

typedef struct
{
  uint64_t total_bytes;
  uint64_t free_bytes;
}
CmdFsSpaceInfo;

struct CmdFsDriver
{
  cmd_device_id_t device_id;
  const char *name;
  const char *root_path;
  void *ctx;

  CmdFsMountFn mount;
  CmdFsUnmountFn unmount;
  CmdFsListBeginFn list_begin;
  CmdFsListNextFn list_next;
  CmdFsListCloseFn list_close;
  CmdFsStatFn stat;
  CmdFsMkdirFn mkdir;
  CmdFsRemoveFn remove;
  CmdFsRenameFn rename;
  CmdFsOpenFn open_read;
  CmdFsOpenFn open_write;
  CmdFsReadFn read;
  CmdFsWriteFn write;
  CmdFsSeekFn seek;
  CmdFsTellFn tell;
  CmdFsCloseFn close;
};

struct CmdFsManager
{
  CmdFsDriver *drivers[CMD_FS_DRIVER_MAX];
  uint32_t registered_mask;
  uint32_t mounted_mask;
};

extern CmdFsDriver cmd_fs_sd_driver;
extern CmdFsDriver cmd_fs_fat_driver;
extern CmdFsDriver cmd_fs_tsf_driver;

bool cmd_fs_is_valid_device_id(cmd_device_id_t device_id);

void cmd_fs_manager_init(CmdFsManager *manager);
esp_err_t cmd_fs_manager_register_driver(CmdFsManager *manager, CmdFsDriver *driver);
esp_err_t cmd_fs_manager_unregister_driver(CmdFsManager *manager, cmd_device_id_t device_id);
CmdFsDriver *cmd_fs_manager_get_driver(CmdFsManager *manager, cmd_device_id_t device_id);
const CmdFsDriver *cmd_fs_manager_get_driver_const(const CmdFsManager *manager, cmd_device_id_t device_id);
esp_err_t cmd_fs_manager_register_default_drivers(CmdFsManager *manager);
esp_err_t cmd_fs_manager_build_abs_path(const CmdFsManager *manager, cmd_device_id_t device_id, const char *current_path, const char *filename, char *out, size_t out_size);

esp_err_t cmd_fs_manager_mount(CmdFsManager *manager, cmd_device_id_t device_id);
esp_err_t cmd_fs_manager_unmount(CmdFsManager *manager, cmd_device_id_t device_id);
esp_err_t cmd_fs_manager_unmount_all(CmdFsManager *manager);
esp_err_t cmd_fs_manager_list_begin(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsList *list);
esp_err_t cmd_fs_manager_list_next(CmdFsManager *manager, CmdFsList *list, cmd_file_entry_t *entry);
void cmd_fs_manager_list_close(CmdFsManager *manager, CmdFsList *list);
esp_err_t cmd_fs_manager_stat(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, cmd_file_entry_t *entry);
esp_err_t cmd_fs_manager_space_info(CmdFsManager *manager, cmd_device_id_t device_id, CmdFsSpaceInfo *info);
esp_err_t cmd_fs_manager_mkdir(CmdFsManager *manager, cmd_device_id_t device_id, const char *path);
esp_err_t cmd_fs_manager_remove(CmdFsManager *manager, cmd_device_id_t device_id, const char *path);
esp_err_t cmd_fs_manager_rename(CmdFsManager *manager, cmd_device_id_t device_id, const char *old_path, const char *new_path);
esp_err_t cmd_fs_manager_open_read(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsFile *file);
esp_err_t cmd_fs_manager_open_write(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsFile *file);
esp_err_t cmd_fs_manager_read(CmdFsManager *manager, CmdFsFile *file, void *dst, size_t size, size_t *out_size);
esp_err_t cmd_fs_manager_write(CmdFsManager *manager, CmdFsFile *file, const void *src, size_t size, size_t *out_size);
esp_err_t cmd_fs_manager_seek(CmdFsManager *manager, CmdFsFile *file, int64_t offset, int whence);
esp_err_t cmd_fs_manager_tell(CmdFsManager *manager, CmdFsFile *file, uint64_t *out_pos);
esp_err_t cmd_fs_manager_close(CmdFsManager *manager, CmdFsFile *file);

esp_err_t cmd_fs_copy_path(const char *src, char *out, size_t out_size);
esp_err_t cmd_fs_join_path(const char *left, const char *right, char *out, size_t out_size);
esp_err_t cmd_fs_join3_path(const char *left, const char *middle, const char *right, char *out, size_t out_size);
esp_err_t cmd_fs_parent_path(const char *path, char *out, size_t out_size);
esp_err_t cmd_fs_extract_filename(const char *path, char *out, size_t out_size);
const char *cmd_fs_filename_ptr(const char *path);
bool cmd_fs_path_is_root(const char *path);

#ifdef __cplusplus
}
#endif

// --------------- cmd_input.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_INPUT_WAIT_FOREVER  UINT32_MAX
#define CMD_INPUT_ESC_TIMEOUT_MS 120

typedef struct CmdInputBackend CmdInputBackend;

typedef esp_err_t (*cmd_input_begin_fn)(CmdInputBackend *backend);
typedef void (*cmd_input_end_fn)(CmdInputBackend *backend);
typedef esp_err_t (*cmd_input_poll_event_fn)(CmdInputBackend *backend, cmd_event_t *event, uint32_t timeout_ms);

struct CmdInputBackend
{
  const char *name;
  void *ctx;
  cmd_input_begin_fn begin;
  cmd_input_end_fn end;
  cmd_input_poll_event_fn poll_event;
};

void cmd_input_event_clear(cmd_event_t *event);
void cmd_input_event_key(cmd_event_t *event, cmd_key_t key, uint32_t mods);
void cmd_input_event_char(cmd_event_t *event, uint32_t ch, uint32_t mods);
void cmd_input_event_quit(cmd_event_t *event);

esp_err_t cmd_input_begin(CmdInputBackend *backend);
void cmd_input_end(CmdInputBackend *backend);
esp_err_t cmd_input_poll_event(CmdInputBackend *backend, cmd_event_t *event, uint32_t timeout_ms);

CmdInputBackend *cmd_input_console_backend();
CmdInputBackend *cmd_input_default_backend();

const char *cmd_input_key_name(cmd_key_t key);

#ifdef __cplusplus
}
#endif

// --------------- cmd_panel.h ---------------

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CMD_PANEL_MAX_ENTRIES
#define CMD_PANEL_MAX_ENTRIES       1000
#endif

#ifndef CMD_PANEL_DEFAULT_PAGE_STEP
#define CMD_PANEL_DEFAULT_PAGE_STEP 10
#endif

typedef enum
{
  CMD_PANEL_FLAG_NONE   = 0,
  CMD_PANEL_FLAG_ACTIVE = 1u << 0,
  CMD_PANEL_FLAG_ERROR  = 1u << 1,
  CMD_PANEL_FLAG_TRUNCATED = 1u << 2
} cmd_panel_flags_t;

struct CmdPanel
{
  CmdFsManager *fs;
  cmd_device_id_t device_id;
  char current_path[CMD_PATH_MAX];
  cmd_file_entry_t *entries;
  size_t capacity;
  size_t count;
  size_t cursor;
  size_t scroll;
  size_t view_rows;
  size_t selected_count;
  uint64_t selected_size;
  CmdFsSpaceInfo space_info;
  esp_err_t space_info_error;
  bool space_info_valid;
  bool active;
  uint32_t flags;
  esp_err_t last_error;
};

esp_err_t cmd_panel_alloc_global_buffers();
void cmd_panel_free_global_buffers();
esp_err_t cmd_panel_alloc_entries(CmdPanel *panel, size_t capacity);
void cmd_panel_free_entries(CmdPanel *panel);
void cmd_panel_init(CmdPanel *panel, CmdFsManager *fs, cmd_device_id_t device_id);
void cmd_panel_reset(CmdPanel *panel);
void cmd_panel_set_active(CmdPanel *panel, bool active);
bool cmd_panel_is_active(const CmdPanel *panel);
esp_err_t cmd_panel_set_path(CmdPanel *panel, const char *path);
esp_err_t cmd_panel_reload(CmdPanel *panel);
esp_err_t cmd_panel_draw(CmdPanel *panel, CmdDisplayBuffer *buffer, const cmd_rect_t *rect);
void cmd_panel_cursor_up(CmdPanel *panel);
void cmd_panel_cursor_down(CmdPanel *panel);
void cmd_panel_cursor_page_up(CmdPanel *panel);
void cmd_panel_cursor_page_down(CmdPanel *panel);
void cmd_panel_cursor_home(CmdPanel *panel);
void cmd_panel_cursor_end(CmdPanel *panel);
const cmd_file_entry_t *cmd_panel_get_selected_entry(const CmdPanel *panel);
void cmd_panel_toggle_selection(CmdPanel *panel);
void cmd_panel_invert_selection(CmdPanel *panel);
void cmd_panel_clear_selection(CmdPanel *panel);
void cmd_panel_update_selection_stats(CmdPanel *panel);
bool cmd_panel_has_selection(const CmdPanel *panel);
esp_err_t cmd_panel_change_device(CmdPanel *panel, cmd_device_id_t device_id);
esp_err_t cmd_panel_enter_selected_dir(CmdPanel *panel);
esp_err_t cmd_panel_go_parent(CmdPanel *panel);
void cmd_panel_clamp_cursor(CmdPanel *panel);
void cmd_panel_ensure_cursor_visible(CmdPanel *panel);

#ifdef __cplusplus
}
#endif
