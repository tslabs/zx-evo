
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_private/esp_clk.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"

#include "cmd.h"
#include "ft8xx.h"
#include "tracker.h"
#include "sfx.h"
#include "mem_obj.h"
#include "esp_spi_defs.h"
#include "sdmmc.h"
#include "fatfs.h"
#include "stats.h"
#include "tsf.h"


#define CMD_LAYOUT_TOP_Y              0
#define CMD_LAYOUT_PANEL_Y            0

#define CMD_PANEL_LEFT                0
#define CMD_PANEL_RIGHT               1

#define CMD_APP_STATUS_MAX            80
#define CMD_APP_DEVICE_ITEM_MAX       40
#define CMD_APP_DEVICE_DIALOG_W       44
#define CMD_APP_DEVICE_DIALOG_H       9
#define CMD_APP_HELP_DIALOG_W         70
#define CMD_APP_HELP_DIALOG_H         25
#define CMD_APP_HELP_DIALOG_REFRESH_MS 100
#define CMD_APP_FILEOP_DIALOG_W       58
#define CMD_APP_FILEOP_DIALOG_H       8
#define CMD_APP_ERROR_DIALOG_W        60
#define CMD_APP_ERROR_DIALOG_H        8
#define CMD_APP_MKDIR_DIALOG_W        54
#define CMD_APP_MKDIR_DIALOG_H        7
#define CMD_APP_ERROR_TITLE_MAX       40
#define CMD_APP_VIEWER_READ_BUFFER_SIZE 512
#define CMD_APP_VIEWER_MAX_ROWS       (CMD_DISPLAY_ROWS - 2)
#define CMD_APP_VIEWER_LINE_SIZE      (CMD_DISPLAY_COLS + 1)
#define CMD_APP_HEX_VIEWER_BYTES_PER_ROW 16
#define CMD_APP_BAUD_SWITCH_DELAY_MS 100
#define CMD_APP_BAUD_SEQ_MAX        96
#define CMD_APP_BAUD_DEFAULT        115200
#define CMD_APP_SFX_PREVIEW_GROUP   255
#define CMD_APP_MODULE_INFO_INITIAL_CAP 4096
#define CMD_APP_MODULE_INFO_MAX_TEXT_SIZE (512 * 1024)
#define CMD_APP_MODULE_INFO_READ_BUFFER_SIZE MOD_HEADER_SIZE
#define CMD_APP_XM_MAX_ROWS 256
#define CMD_APP_S3M_FILE_TYPE_OFFSET 29
#define CMD_APP_S3M_ORDERS_OFFSET 32
#define CMD_APP_S3M_SAMPLES_OFFSET 34
#define CMD_APP_S3M_PATTERNS_OFFSET 36
#define CMD_APP_S3M_FLAGS_OFFSET 38
#define CMD_APP_S3M_CWTV_OFFSET 40
#define CMD_APP_S3M_FORMAT_VERSION_OFFSET 42
#define CMD_APP_S3M_GLOBAL_VOLUME_OFFSET 48
#define CMD_APP_S3M_SPEED_OFFSET 49
#define CMD_APP_S3M_TEMPO_OFFSET 50
#define CMD_APP_S3M_MASTER_VOLUME_OFFSET 51
#define CMD_APP_S3M_PANNING_TABLE_FLAG_OFFSET 53
#define CMD_APP_S3M_CHANNEL_SETTINGS_OFFSET 64
#define CMD_APP_S3M_FILE_TYPE_MODULE 0x10
#define CMD_APP_S3M_PANNING_TABLE_PRESENT 0xFC
#define CMD_APP_S3M_SAMPLE_TYPE_NONE 0
#define CMD_APP_S3M_SAMPLE_TYPE_PCM 1
#define CMD_APP_S3M_SAMPLE_FLAG_LOOP 0x01
#define CMD_APP_S3M_SAMPLE_FLAG_STEREO 0x02
#define CMD_APP_S3M_SAMPLE_FLAG_16BIT 0x04
#define CMD_APP_S3M_SAMPLE_PACK_NONE 0

#ifndef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM UART_NUM_0
#endif

typedef enum
{
  CMD_APP_FILEOP_NONE = 0,
  CMD_APP_FILEOP_COPY,
  CMD_APP_FILEOP_MOVE,
  CMD_APP_FILEOP_DELETE
} cmd_app_fileop_t;

typedef enum
{
  CMD_APP_NAME_DIALOG_MKDIR = 0,
  CMD_APP_NAME_DIALOG_RENAME
} cmd_app_name_dialog_op_t;

typedef struct CmdApp
{
  CmdFsManager fs;
  CmdPanel left_panel;
  CmdPanel right_panel;
  CmdDisplayBuffer buffer;
  CmdDisplayMux mux;
  CmdInputBackend *input;
  int active_panel;
  bool device_dialog_open;
  int device_dialog_panel;
  int device_dialog_selected;
  bool device_dialog_available[CMD_DEVICE_MAX];
  char device_dialog_items[CMD_DEVICE_MAX][CMD_APP_DEVICE_ITEM_MAX + 1];
  bool help_dialog_open;
  bool mkdir_dialog_open;
  cmd_app_name_dialog_op_t mkdir_dialog_op;
  char mkdir_name[CMD_FILE_NAME_MAX];
  char rename_src_path[CMD_PATH_MAX];
  char rename_dst_path[CMD_PATH_MAX];
  char rename_remap_path[CMD_PATH_MAX];
  bool fileop_confirm_open;
  cmd_app_fileop_t fileop_confirm_op;
  char fileop_name[CMD_FILE_NAME_MAX];
  char fileop_dst_path[CMD_PATH_MAX];
  bool fileop_progress_open;
  cmd_app_fileop_t fileop_progress_op;
  char fileop_progress_name[CMD_FILE_NAME_MAX];
  uint64_t fileop_bytes_done;
  uint64_t fileop_bytes_total;
  size_t fileop_files_done;
  size_t fileop_files_total;
  bool fileop_progress_can_cancel;
  bool fileop_group;
  size_t fileop_item_count;
  uint64_t fileop_item_size;
  bool fileop_cancel_requested;
  bool error_dialog_open;
  esp_err_t error_dialog_err;
  char error_dialog_title[CMD_APP_ERROR_TITLE_MAX + 1];
  char error_dialog_message[CMD_APP_STATUS_MAX + 1];
  bool viewer_open;
  cmd_device_id_t viewer_device;
  char viewer_path[CMD_PATH_MAX];
  char viewer_title[CMD_FILE_NAME_MAX];
  CmdFsFile viewer_file;
  bool viewer_file_open;
  uint8_t viewer_read_buffer[CMD_APP_VIEWER_READ_BUFFER_SIZE];
  uint8_t viewer_seek_buffer[CMD_APP_VIEWER_READ_BUFFER_SIZE];
  uint64_t viewer_prev_offsets[CMD_APP_VIEWER_MAX_ROWS + 1];
  char viewer_draw_line[CMD_DISPLAY_COLS + 1];
  char viewer_num[24];
  size_t viewer_read_pos;
  size_t viewer_read_len;
  uint64_t viewer_read_base;
  uint64_t viewer_top_offset;
  uint64_t viewer_next_offset;
  uint64_t viewer_line_offsets[CMD_APP_VIEWER_MAX_ROWS + 1];
  size_t viewer_top_line;
  size_t viewer_next_line;
  size_t viewer_page_lines;
  bool viewer_eof;
  char viewer_lines[CMD_APP_VIEWER_MAX_ROWS][CMD_APP_VIEWER_LINE_SIZE];
  bool hex_viewer_open;
  cmd_device_id_t hex_viewer_device;
  char hex_viewer_path[CMD_PATH_MAX];
  char hex_viewer_title[CMD_FILE_NAME_MAX];
  CmdFsFile hex_viewer_file;
  bool hex_viewer_file_open;
  uint8_t hex_viewer_read_buffer[CMD_APP_VIEWER_READ_BUFFER_SIZE];
  uint8_t hex_viewer_row[CMD_APP_HEX_VIEWER_BYTES_PER_ROW];
  char hex_viewer_tmp[8];
  uint64_t hex_viewer_offset;
  uint64_t hex_viewer_size;
  bool hex_viewer_size_known;
  size_t hex_viewer_page_rows;
  bool hex_viewer_eof;
  char hex_viewer_lines[CMD_APP_VIEWER_MAX_ROWS][CMD_APP_VIEWER_LINE_SIZE];
  bool module_info_open;
  cmd_device_id_t module_info_device;
  char module_info_path[CMD_PATH_MAX];
  char module_info_title[CMD_FILE_NAME_MAX];
  char *module_info_text;
  size_t module_info_text_len;
  size_t module_info_text_cap;
  size_t module_info_top_line;
  size_t module_info_line_count;
  uint8_t module_info_read_buffer[CMD_APP_MODULE_INFO_READ_BUFFER_SIZE];
  char module_info_tmp[CMD_DISPLAY_COLS + 1];
  char module_info_name[64];
  char module_info_sample_name[64];
  bool jpg_viewer_open;
  bool jpg_viewer_added_ft812_backend;
  int jpg_viewer_panel;
  size_t jpg_viewer_index;
  bool jpg_viewer_stretch;
  char jpg_viewer_path[CMD_PATH_MAX];
  char jpg_viewer_status[96 + 1];
  int sfx_preview_handle;
  int sfx_preview_channel;
  void *sfx_preview_addr;
  char status[CMD_APP_STATUS_MAX + 1];
} CmdApp;

CmdApp *g_cmd_app;

esp_err_t cmd_app_render(CmdApp *app);
esp_err_t cmd_app_render_console(CmdApp *app);
esp_err_t cmd_app_reload_panel(CmdApp *app, CmdPanel *panel, const char *name);

int cmd_app_screen_w(const CmdApp *app)
{
  if (!app || app->buffer.size.w <= 0) return CMD_DISPLAY_DEFAULT_COLS;
  return app->buffer.size.w;
}

int cmd_app_screen_h(const CmdApp *app)
{
  if (!app || app->buffer.size.h <= 0) return CMD_DISPLAY_DEFAULT_ROWS;
  return app->buffer.size.h;
}

int cmd_app_bottom_y(const CmdApp *app)
{
  return cmd_app_screen_h(app) - 1;
}

int cmd_app_panel_info_y(const CmdApp *app)
{
  int y = cmd_app_bottom_y(app) - 2 + 1;
  if (y < CMD_LAYOUT_PANEL_Y + 1) y = CMD_LAYOUT_PANEL_Y + 1;
  return y;
}

void cmd_app_make_panel_rects(const CmdApp *app, cmd_rect_t *left, cmd_rect_t *right)
{
  int cols = cmd_app_screen_w(app);
  int left_w = cols / 2;
  int right_w = cols - left_w;
  int panel_h = cmd_app_panel_info_y(app) - CMD_LAYOUT_PANEL_Y;

  if (panel_h < 3) panel_h = 3;

  if (left)
  {
    left->x = 0;
    left->y = CMD_LAYOUT_PANEL_Y;
    left->w = left_w;
    left->h = panel_h;
  }

  if (right)
  {
    right->x = left_w;
    right->y = CMD_LAYOUT_PANEL_Y;
    right->w = right_w;
    right->h = panel_h;
  }
}

int cmd_app_center_x(const CmdApp *app, int w)
{
  int cols = cmd_app_screen_w(app);
  if (w >= cols) return 0;
  return (cols - w) / 2;
}

int cmd_app_center_y(const CmdApp *app, int h)
{
  int rows = cmd_app_screen_h(app);
  if (h >= rows) return 0;
  return (rows - h) / 2;
}

CmdApp *cmd_app_alloc()
{
  return (CmdApp *)heap_caps_calloc(
    1,
    sizeof(CmdApp),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void cmd_app_free(CmdApp *app)
{
  if (!app) return;

  heap_caps_free(app);
}

cmd_display_attr_t cmd_app_normal_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_WHITE, CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_ATTR_FLAG_NONE);
}

cmd_display_attr_t cmd_app_bar_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_COLOR_CYAN, CMD_DISPLAY_ATTR_FLAG_BOLD);
}

size_t cmd_app_text_len(const char *s)
{
  size_t len = 0;

  if (!s) return 0;

  while (s[len])
    len++;

  return len;
}

void cmd_app_text_clear(char *out, size_t out_size)
{
  if (!out || out_size == 0) return;
  out[0] = 0;
}

void cmd_app_append_limited(char *out, size_t out_size, const char *text, size_t max_chars)
{
  size_t len;
  size_t pos = 0;

  if (!out || out_size == 0 || !text) return;

  len = cmd_app_text_len(out);

  while (text[pos] && pos < max_chars && len + 1 < out_size)
  {
    out[len] = text[pos];
    len++;
    pos++;
  }

  out[len] = 0;
}

void cmd_app_append_text(char *out, size_t out_size, const char *text)
{
  if (!text) return;
  cmd_app_append_limited(out, out_size, text, cmd_app_text_len(text));
}

void cmd_app_format_baud(char *out, size_t out_size, uint32_t baud)
{
  unsigned long whole;
  unsigned long frac;

  if (!out || out_size == 0) return;

  if (baud >= 1000000)
  {
    whole = (unsigned long)(baud / 1000000);
    frac = (unsigned long)(((baud % 1000000) + 50000) / 100000);
    if (frac >= 10)
    {
      whole++;
      frac = 0;
    }
    if (frac == 0)
      snprintf(out, out_size, "%luM", whole);
    else
      snprintf(out, out_size, "%lu.%luM", whole, frac);
    return;
  }

  if (baud >= 1000)
  {
    whole = (unsigned long)(baud / 1000);
    frac = (unsigned long)(((baud % 1000) + 50) / 100);
    if (frac >= 10)
    {
      whole++;
      frac = 0;
    }
    if (frac == 0)
      snprintf(out, out_size, "%luk", whole);
    else
      snprintf(out, out_size, "%lu.%luk", whole, frac);
    return;
  }

  snprintf(out, out_size, "%lu", (unsigned long)baud);
}

bool cmd_app_get_uart_baud_text(char *out, size_t out_size)
{
  uint32_t baud = 0;
  esp_err_t err;

  if (!out || out_size == 0) return false;

  err = uart_get_baudrate((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &baud);
  if (err != ESP_OK || baud == 0)
  {
    out[0] = 0;
    return false;
  }

  cmd_app_format_baud(out, out_size, baud);
  return true;
}

void cmd_app_format_bytes(char *out, size_t out_size, uint64_t value)
{
  unsigned long long whole;
  unsigned long long frac;

  if (!out || out_size == 0) return;

  if (value >= (1ULL << 30))
  {
    whole = (unsigned long long)(value >> 30);
    frac = (unsigned long long)(((value & ((1ULL << 30) - 1)) * 100ULL) >> 30);
    snprintf(out, out_size, "%llu.%02llu GiB", whole, frac);
    return;
  }

  if (value >= (1ULL << 20))
  {
    whole = (unsigned long long)(value >> 20);
    frac = (unsigned long long)(((value & ((1ULL << 20) - 1)) * 100ULL) >> 20);
    snprintf(out, out_size, "%llu.%02llu MiB", whole, frac);
    return;
  }

  if (value >= (1ULL << 10))
  {
    whole = (unsigned long long)(value >> 10);
    frac = (unsigned long long)(((value & ((1ULL << 10) - 1)) * 100ULL) >> 10);
    snprintf(out, out_size, "%llu.%02llu KiB", whole, frac);
    return;
  }

  snprintf(out, out_size, "%llu bytes", (unsigned long long)value);
}

const char *cmd_err_name(esp_err_t err)
{
  switch (err)
  {
    case CMD_ERR_INVALID_DEVICE: return "CMD_ERR_INVALID_DEVICE";
    case CMD_ERR_NOT_MOUNTED: return "CMD_ERR_NOT_MOUNTED";
    case CMD_ERR_PATH_TOO_LONG: return "CMD_ERR_PATH_TOO_LONG";
    case CMD_ERR_ENTRY_TOO_LONG: return "CMD_ERR_ENTRY_TOO_LONG";
    case CMD_ERR_DIR_FULL: return "CMD_ERR_DIR_FULL";
    case CMD_ERR_INPUT_UNAVAILABLE: return "CMD_ERR_INPUT_UNAVAILABLE";
    case CMD_ERR_CANCELLED: return "CMD_ERR_CANCELLED";
    default: return esp_err_to_name(err);
  }
}

void cmd_set_status(CmdApp *app, const char *fmt, ...)
{
  va_list args;
  int written;

  if (!app) return;

  app->status[0] = 0;
  if (!fmt) return;

  va_start(args, fmt);
  written = vsnprintf(app->status, sizeof(app->status), fmt, args);
  va_end(args);

  if (written < 0)
    app->status[0] = 0;
  else
    app->status[sizeof(app->status) - 1] = 0;
}

void cmd_show_error(CmdApp *app, const char *title, esp_err_t err)
{
  if (!app) return;

  app->error_dialog_open = true;
  app->error_dialog_err = err;
  cmd_app_text_clear(app->error_dialog_title, sizeof(app->error_dialog_title));
  cmd_app_append_text(app->error_dialog_title, sizeof(app->error_dialog_title), title ? title : "Error");
  cmd_app_text_clear(app->error_dialog_message, sizeof(app->error_dialog_message));
  cmd_app_append_text(app->error_dialog_message, sizeof(app->error_dialog_message), title ? title : "Error");
  cmd_app_append_text(app->error_dialog_message, sizeof(app->error_dialog_message), ": ");
  cmd_app_append_text(app->error_dialog_message, sizeof(app->error_dialog_message), cmd_err_name(err));
  cmd_set_status(app, "%s", app->error_dialog_message);
}

void cmd_app_make_top_bar_line(char *out, size_t out_size, CmdPanel *panel, const char *status)
{
  cmd_app_text_clear(out, out_size);
  cmd_app_append_text(out, out_size, "Active: ");

  if (panel)
  {
    cmd_app_append_text(out, out_size, cmd_device_name(panel->device_id));
    cmd_app_append_text(out, out_size, "  Path: ");
    cmd_app_append_text(out, out_size, panel->current_path);
    if (panel->selected_count > 0)
    {
      char sel[48];
      snprintf(sel, sizeof(sel), "  Sel: %u/%llu",
        (unsigned)panel->selected_count,
        (unsigned long long)panel->selected_size);
      cmd_app_append_text(out, out_size, sel);
    }
  }
  else
  {
    cmd_app_append_text(out, out_size, "?  Path: /");
  }

  cmd_app_append_text(out, out_size, "  Free/Used: n/a  ");
  cmd_app_append_text(out, out_size, status ? status : "ready");
}

void cmd_app_set_status(CmdApp *app, const char *text)
{
  cmd_set_status(app, "%s", text ? text : "");
}

void cmd_app_set_status_err(CmdApp *app, const char *op, esp_err_t err)
{
  cmd_set_status(app, "%s failed: %s", op ? op : "operation", cmd_err_name(err));
}

CmdPanel *cmd_app_panel_by_index(CmdApp *app, int index)
{
  if (!app) return NULL;
  if (index == CMD_PANEL_LEFT) return &app->left_panel;
  if (index == CMD_PANEL_RIGHT) return &app->right_panel;
  return NULL;
}

CmdPanel *cmd_app_active_panel(CmdApp *app)
{
  if (!app) return NULL;
  return cmd_app_panel_by_index(app, app->active_panel);
}

CmdPanel *cmd_app_passive_panel(CmdApp *app)
{
  if (!app) return NULL;
  if (app->active_panel == CMD_PANEL_LEFT) return &app->right_panel;
  return &app->left_panel;
}

void cmd_app_set_active_panel(CmdApp *app, int index)
{
  if (!app) return;
  if (index != CMD_PANEL_LEFT && index != CMD_PANEL_RIGHT) return;

  app->active_panel = index;
  cmd_panel_set_active(&app->left_panel, index == CMD_PANEL_LEFT);
  cmd_panel_set_active(&app->right_panel, index == CMD_PANEL_RIGHT);
}

void cmd_app_switch_panel(CmdApp *app)
{
  if (!app) return;

  if (app->active_panel == CMD_PANEL_LEFT)
    cmd_app_set_active_panel(app, CMD_PANEL_RIGHT);
  else
    cmd_app_set_active_panel(app, CMD_PANEL_LEFT);
}

const char *cmd_app_panel_name(int index)
{
  if (index == CMD_PANEL_LEFT) return "left panel";
  if (index == CMD_PANEL_RIGHT) return "right panel";
  return "panel";
}

cmd_display_attr_t cmd_app_dialog_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_WHITE, CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_ATTR_FLAG_NONE);
}

cmd_display_attr_t cmd_app_dialog_frame_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_WHITE, CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_ATTR_FLAG_BOLD);
}

cmd_display_attr_t cmd_app_dialog_selected_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_COLOR_BRIGHT_YELLOW, CMD_DISPLAY_ATTR_FLAG_BOLD | CMD_DISPLAY_ATTR_FLAG_SELECTED);
}

cmd_display_attr_t cmd_app_dialog_unavailable_attr()
{
  return cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_BLACK, CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_ATTR_FLAG_NONE);
}

void cmd_app_open_device_dialog(CmdApp *app, int panel_index)
{
  CmdPanel *panel;

  if (!app) return;
  if (panel_index != CMD_PANEL_LEFT && panel_index != CMD_PANEL_RIGHT) return;

  panel = cmd_app_panel_by_index(app, panel_index);
  if (!panel) return;

  app->device_dialog_open = true;
  app->device_dialog_panel = panel_index;
  app->device_dialog_selected = cmd_fs_is_valid_device_id(panel->device_id) ? (int)panel->device_id : 0;

  for (int i = 0; i < CMD_DEVICE_MAX; i++)
  {
    cmd_device_id_t device_id = (cmd_device_id_t)i;
    bool available = cmd_fs_manager_mount(&app->fs, device_id) == ESP_OK;

    app->device_dialog_available[device_id] = available;
    snprintf(app->device_dialog_items[device_id],
      sizeof(app->device_dialog_items[device_id]),
      "%d  %-6s %s",
      i + 1,
      cmd_device_name(device_id),
      available ? "" : "(unavailable)");
  }

  cmd_app_set_status(app, "select device: Enter/1..3, Esc cancel");
}

void cmd_app_close_device_dialog(CmdApp *app)
{
  if (!app) return;

  app->device_dialog_open = false;
  app->device_dialog_panel = CMD_PANEL_LEFT;
  app->device_dialog_selected = 0;
}

esp_err_t cmd_app_select_device(CmdApp *app, int panel_index, cmd_device_id_t device_id)
{
  CmdPanel *panel;
  esp_err_t err;
  char line[CMD_APP_STATUS_MAX + 1];

  if (!app) return ESP_ERR_INVALID_ARG;
  if (!cmd_fs_is_valid_device_id(device_id)) return CMD_ERR_INVALID_DEVICE;

  panel = cmd_app_panel_by_index(app, panel_index);
  if (!panel) return ESP_ERR_INVALID_ARG;

  err = cmd_panel_change_device(panel, device_id);
  if (err != ESP_OK)
  {
    snprintf(line, sizeof(line), "%s %s unavailable: %s",
      cmd_app_panel_name(panel_index),
      cmd_device_name(device_id),
      cmd_err_name(err));
    cmd_app_set_status(app, line);
    return ESP_OK;
  }

  snprintf(line, sizeof(line), "%s device: %s",
    cmd_app_panel_name(panel_index),
    cmd_device_name(device_id));
  cmd_app_set_status(app, line);
  return ESP_OK;
}

esp_err_t cmd_app_select_dialog_device(CmdApp *app, cmd_device_id_t device_id)
{
  int panel_index;

  if (!app) return ESP_ERR_INVALID_ARG;
  panel_index = app->device_dialog_panel;
  cmd_app_close_device_dialog(app);
  return cmd_app_select_device(app, panel_index, device_id);
}

void cmd_app_draw_device_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_rect_t row_rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  cmd_display_attr_t selected_attr;
  cmd_display_attr_t unavailable_attr;
  char title[CMD_APP_DEVICE_ITEM_MAX + 1];

  if (!app) return;
  if (!app->device_dialog_open) return;

  rect.x = cmd_app_center_x(app, CMD_APP_DEVICE_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_DEVICE_DIALOG_H);
  rect.w = CMD_APP_DEVICE_DIALOG_W;
  rect.h = CMD_APP_DEVICE_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_app_dialog_frame_attr();
  selected_attr = cmd_app_dialog_selected_attr();
  unavailable_attr = cmd_app_dialog_unavailable_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);

  snprintf(title, sizeof(title), " Select device: %s ", cmd_app_panel_name(app->device_dialog_panel));
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, title, frame_attr);

  for (int i = 0; i < CMD_DEVICE_MAX; i++)
  {
    cmd_display_attr_t row_attr = attr;

    row_rect.x = rect.x + 2;
    row_rect.y = rect.y + 2 + i;
    row_rect.w = rect.w - 4;
    row_rect.h = 1;

    if (i == app->device_dialog_selected)
      row_attr = selected_attr;
    else if (!app->device_dialog_available[i])
      row_attr = unavailable_attr;

    cmd_display_buffer_fill_rect(&app->buffer, &row_rect, ' ', row_attr);
    cmd_display_buffer_write_text(&app->buffer, row_rect.x + 1, row_rect.y, app->device_dialog_items[i], row_attr);
  }

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2,
    "Enter select   1..3 quick   Esc cancel", attr);
}

esp_err_t cmd_app_handle_device_dialog_key(CmdApp *app, const cmd_event_t *event)
{
  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->device_dialog_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY) return ESP_OK;

  if (event->key == CMD_KEY_ESC || event->key == CMD_KEY_F10 ||
    (event->key == CMD_KEY_CHAR && (event->ch == 'q' || event->ch == 'Q')))
  {
    cmd_app_close_device_dialog(app);
    cmd_app_set_status(app, "device selection cancelled");
    return ESP_OK;
  }

  if (event->key == CMD_KEY_UP)
  {
    if (app->device_dialog_selected > 0)
      app->device_dialog_selected--;
    return ESP_OK;
  }

  if (event->key == CMD_KEY_DOWN)
  {
    if (app->device_dialog_selected + 1 < CMD_DEVICE_MAX)
      app->device_dialog_selected++;
    return ESP_OK;
  }

  if (event->key == CMD_KEY_HOME)
  {
    app->device_dialog_selected = 0;
    return ESP_OK;
  }

  if (event->key == CMD_KEY_END)
  {
    app->device_dialog_selected = CMD_DEVICE_MAX - 1;
    return ESP_OK;
  }

  if (event->key == CMD_KEY_ENTER)
    return cmd_app_select_dialog_device(app, (cmd_device_id_t)app->device_dialog_selected);

  if (event->key == CMD_KEY_CHAR && event->ch >= '1' && event->ch <= '0' + CMD_DEVICE_MAX)
    return cmd_app_select_dialog_device(app, (cmd_device_id_t)(event->ch - '1'));

  return ESP_OK;
}

void cmd_app_open_help_dialog(CmdApp *app)
{
  if (!app) return;

  app->help_dialog_open = true;
  cmd_app_set_status(app, "help: Enter/Esc close");
}

void cmd_app_close_help_dialog(CmdApp *app)
{
  if (!app) return;

  app->help_dialog_open = false;
  cmd_app_set_status(app, "ready");
}

void cmd_app_draw_help_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  char backend_line[64];
  char baud_text[16];
  char info_line[96];
  char sram_text[32];
  char spiram_text[32];
  size_t sram_free;
  size_t spiram_free;

  if (!app) return;
  if (!app->help_dialog_open) return;

  rect.x = cmd_app_center_x(app, CMD_APP_HELP_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_HELP_DIALOG_H);
  rect.w = CMD_APP_HELP_DIALOG_W;
  rect.h = CMD_APP_HELP_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_app_dialog_frame_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);

  cmd_app_text_clear(backend_line, sizeof(backend_line));
  for (size_t i = 0; i < app->mux.count; i++)
  {
    CmdDisplayBackend *backend = app->mux.backends[i];
    const char *backend_name = "?";

    if (backend == &cmd_display_console_backend)
      backend_name = "uart";
    else if (backend == &cmd_display_ft812_backend)
      backend_name = "ft";
    else if (backend && backend->name)
      backend_name = backend->name;

    if (i > 0) cmd_app_append_text(backend_line, sizeof(backend_line), "+");
    cmd_app_append_text(backend_line, sizeof(backend_line), backend_name);
    if (backend == &cmd_display_console_backend &&
      cmd_app_get_uart_baud_text(baud_text, sizeof(baud_text)))
    {
      cmd_app_append_text(backend_line, sizeof(backend_line), " (");
      cmd_app_append_text(backend_line, sizeof(backend_line), baud_text);
      cmd_app_append_text(backend_line, sizeof(backend_line), ")");
    }
  }

  snprintf(info_line, sizeof(info_line), "Window: %dx%d  Display: %s",
    cmd_app_screen_w(app),
    cmd_app_screen_h(app),
    backend_line[0] ? backend_line : "?");

  sram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  cmd_app_format_bytes(sram_text, sizeof(sram_text), (uint64_t)sram_free);
  cmd_app_format_bytes(spiram_text, sizeof(spiram_text), (uint64_t)spiram_free);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, " Commander help ", frame_attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 2, "Tab                 switch active panel", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 3, "Alt+F1 / Alt+F2     select left/right device", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 4, "F9 or 1             select active panel device", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 5, "Enter               open directory or file", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 6, "Backspace           go to parent directory", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 7, "Ins / *             select item / invert selection", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 8, "F2                  rename selected entry", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 9, "F3 / Alt+F3         text/module info / hex viewer", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 10, "F5 / F6             copy / move to opposite panel", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 11, "F7 / F8 / Del       mkdir / delete", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 12, "Left / Right        page up / page down", attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 13, "Alt+F9              stop XM playback", attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 15, info_line, attr);
  snprintf(info_line, sizeof(info_line), "Memory: SRAM free %s  SPIRAM free %s", sram_text, spiram_text);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 16, info_line, attr);

  snprintf(info_line, sizeof(info_line), "Firmware: %s  API %u  Feature %u",
    PROD_VER_STRING,
    API_VER,
    FEAT_VER);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 18, info_line, attr);
  snprintf(info_line, sizeof(info_line), "Chip: %s  CPU: %d MHz",
    CONFIG_IDF_TARGET,
    esp_clk_cpu_freq() / 1000000);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 19, info_line, attr);
  snprintf(info_line, sizeof(info_line), "Audio CPU: %d.%d/%d.%d%% (last/max)",
    stats::_st.audio_total_last_cpu_x10 / 10,
    stats::_st.audio_total_last_cpu_x10 % 10,
    stats::_st.audio_total_max_cpu_x10 / 10,
    stats::_st.audio_total_max_cpu_x10 % 10);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 20, info_line, attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2, "Enter/Esc/F1/F10 close", attr);
}

esp_err_t cmd_app_handle_help_dialog_key(CmdApp *app, const cmd_event_t *event)
{
  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->help_dialog_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC || event->key == CMD_KEY_ENTER ||
    event->key == CMD_KEY_F1 || event->key == CMD_KEY_F10)
  {
    cmd_app_close_help_dialog(app);
    return ESP_OK;
  }

  return ESP_OK;
}

const char *cmd_app_fileop_verb(cmd_app_fileop_t op)
{
  switch (op)
  {
    case CMD_APP_FILEOP_COPY: return "Copy";
    case CMD_APP_FILEOP_MOVE: return "Move";
    case CMD_APP_FILEOP_DELETE: return "Delete";
    default: return "Operation";
  }
}

const char *cmd_app_fileop_past(cmd_app_fileop_t op)
{
  switch (op)
  {
    case CMD_APP_FILEOP_COPY: return "copied";
    case CMD_APP_FILEOP_MOVE: return "moved";
    case CMD_APP_FILEOP_DELETE: return "deleted";
    default: return "done";
  }
}

const char *cmd_app_fileop_status_name(cmd_app_fileop_t op)
{
  switch (op)
  {
    case CMD_APP_FILEOP_COPY: return "copy";
    case CMD_APP_FILEOP_MOVE: return "move";
    case CMD_APP_FILEOP_DELETE: return "delete";
    default: return "operation";
  }
}

bool cmd_app_has_modal(CmdApp *app)
{
  if (!app) return false;
  if (app->device_dialog_open) return true;
  if (app->help_dialog_open) return true;
  if (app->mkdir_dialog_open) return true;
  if (app->fileop_confirm_open) return true;
  if (app->fileop_progress_open) return true;
  if (app->viewer_open) return true;
  if (app->hex_viewer_open) return true;
  if (app->module_info_open) return true;
  if (app->jpg_viewer_open) return true;
  if (app->error_dialog_open) return true;
  return false;
}

void cmd_app_copy_string(char *out, size_t out_size, const char *text)
{
  if (!out || out_size == 0) return;
  out[0] = 0;
  cmd_app_append_text(out, out_size, text ? text : "");
}

esp_err_t cmd_app_join_selected_path(CmdPanel *panel, const cmd_file_entry_t *entry, char *out, size_t out_size)
{
  if (!panel || !entry || !out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (entry->type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;
  return cmd_fs_join_path(panel->current_path, entry->name, out, out_size);
}

bool cmd_app_extension_is(const char *name, const char *ext)
{
  const char *dot;
  size_t ext_len;

  if (!name || !ext) return false;

  dot = strrchr(name, '.');
  if (!dot) return false;

  ext_len = strlen(ext);
  if (strlen(dot) != ext_len) return false;

  for (size_t i = 0; i < ext_len; i++)
  {
    char a = dot[i];
    char b = ext[i];

    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    if (a != b) return false;
  }

  return true;
}

bool cmd_app_entry_is_xm(const cmd_file_entry_t *entry)
{
  if (!entry || entry->type != CMD_ENTRY_FILE) return false;
  if (cmd_app_extension_is(entry->name, ".xm")) return true;
  if (cmd_app_extension_is(entry->name, ".xmz")) return true;
  if (cmd_app_extension_is(entry->name, ".mod")) return true;
  if (cmd_app_extension_is(entry->name, ".s3m")) return true;
  return false;
}

bool cmd_app_entry_is_module_info(const cmd_file_entry_t *entry)
{
  if (!entry || entry->type != CMD_ENTRY_FILE) return false;
  if (cmd_app_extension_is(entry->name, ".xm")) return true;
  if (cmd_app_extension_is(entry->name, ".mod")) return true;
  if (cmd_app_extension_is(entry->name, ".s3m")) return true;
  return false;
}

bool cmd_app_entry_is_jpg(const cmd_file_entry_t *entry)
{
  if (!entry || entry->type != CMD_ENTRY_FILE) return false;
  return ft_jpg_ext_match(entry->name) != 0;
}

bool cmd_app_entry_is_dxp(const cmd_file_entry_t *entry)
{
  if (!entry || entry->type != CMD_ENTRY_FILE) return false;
  return ft_dxp_ext_match(entry->name) != 0;
}

bool cmd_app_entry_is_ft_image(const cmd_file_entry_t *entry)
{
  return cmd_app_entry_is_jpg(entry) || cmd_app_entry_is_dxp(entry);
}

bool cmd_app_entry_is_wav(const cmd_file_entry_t *entry)
{
  if (!entry || entry->type != CMD_ENTRY_FILE) return false;
  return cmd_app_extension_is(entry->name, ".wav");
}

bool cmd_app_has_ft812_backend(CmdApp *app)
{
  if (!app) return false;

  for (size_t i = 0; i < app->mux.count; i++)
    if (app->mux.backends[i] == &cmd_display_ft812_backend)
      return true;

  return false;
}

bool cmd_app_has_console_backend(CmdApp *app)
{
  if (!app) return false;

  for (size_t i = 0; i < app->mux.count; i++)
    if (app->mux.backends[i] == &cmd_display_console_backend)
      return true;

  return false;
}

esp_err_t cmd_app_build_entry_abs_path(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry, char *out, size_t out_size)
{
  esp_err_t err;

  if (!app || !panel || !entry || !out || out_size == 0) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_mount(&app->fs, panel->device_id);
  if (err != ESP_OK) return err;

  return cmd_fs_manager_build_abs_path(&app->fs, panel->device_id,
    panel->current_path, entry->name, out, out_size);
}

esp_err_t cmd_app_play_xm_file(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  char path[CMD_PATH_MAX];
  esp_err_t err;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;
  if (!cmd_app_entry_is_xm(entry)) return ESP_ERR_INVALID_ARG;

  err = cmd_app_build_entry_abs_path(app, panel, entry, path, sizeof(path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "Module path", err);
    return ESP_OK;
  }

  if (xm_load_play_file(path, true))
  {
    cmd_show_error(app, "Module play", ESP_FAIL);
    return ESP_OK;
  }

  cmd_set_status(app, "Module playing: %s", entry->name);
  return ESP_OK;
}

esp_err_t cmd_app_stop_xm_playback(CmdApp *app)
{
  if (!app) return ESP_ERR_INVALID_ARG;

  if (xm_stop_cmd(true))
  {
    cmd_show_error(app, "Module stop", ESP_FAIL);
    return ESP_OK;
  }

  cmd_set_status(app, "Module stopped");
  return ESP_OK;
}

void cmd_app_delete_sfx_preview(CmdApp *app)
{
  if (!app) return;

  int handle = app->sfx_preview_handle;
  void *addr = app->sfx_preview_addr;

  app->sfx_preview_handle = -1;
  app->sfx_preview_channel = -1;
  app->sfx_preview_addr = NULL;

  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return;
  if (!check_handle(handle)) return;
  if (mem_obj[handle].type != OBJ_TYPE_WAV) return;
  if (addr && mem_obj[handle].addr != addr) return;

  delete_obj(handle);
}

esp_err_t cmd_app_play_wav_file(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  char path[CMD_PATH_MAX];
  SFX_WAV_INFO info = {};
  int handle = -1;
  int channel = -1;
  esp_err_t err;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;
  if (!cmd_app_entry_is_wav(entry)) return ESP_ERR_INVALID_ARG;

  err = cmd_app_build_entry_abs_path(app, panel, entry, path, sizeof(path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "WAV path", err);
    return ESP_OK;
  }

  cmd_app_delete_sfx_preview(app);

  err = sfx_load_file(path, &handle, &info);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "WAV load", err);
    return ESP_OK;
  }

  app->sfx_preview_handle = handle;
  app->sfx_preview_channel = -1;
  app->sfx_preview_addr = mem_obj[handle].addr;

  err = sfx_play_sync(handle, CMD_APP_SFX_PREVIEW_GROUP, SFX_VOLUME_MAX, SFX_PAN_CENTER, SFX_PITCH_ONE, &channel);
  if (err != ESP_OK)
  {
    cmd_app_delete_sfx_preview(app);
    cmd_show_error(app, "WAV play", err);
    return ESP_OK;
  }

  app->sfx_preview_channel = channel;
  cmd_set_status(app, "WAV playing: %s  handle=%02X ch=%u %uHz %u-bit %s",
    entry->name,
    (unsigned)handle,
    (unsigned)channel,
    (unsigned)info.sample_rate,
    (unsigned)info.bits_per_sample,
    info.channels == 1 ? "mono" : "stereo");
  return ESP_OK;
}

void cmd_app_close_jpg_viewer(CmdApp *app)
{
  bool remove_ft812_backend;

  if (!app) return;

  remove_ft812_backend = app->jpg_viewer_added_ft812_backend;

  app->jpg_viewer_open = false;
  app->jpg_viewer_added_ft812_backend = false;
  app->jpg_viewer_panel = CMD_PANEL_LEFT;
  app->jpg_viewer_index = 0;
  app->jpg_viewer_stretch = false;
  app->jpg_viewer_path[0] = 0;
  app->jpg_viewer_status[0] = 0;

  if (remove_ft812_backend)
  {
    cmd_display_backend_deinit(&cmd_display_ft812_backend);
    cmd_display_mux_remove_backend(&app->mux, &cmd_display_ft812_backend);
    return;
  }

  if (cmd_app_has_ft812_backend(app))
  {
    esp_err_t err = ft_text_attr_mode_show();
    if (err == ESP_OK) err = ft_wait_swap(1000);
    if (err != ESP_OK) cmd_show_error(app, "FT restore", err);
  }
}

esp_err_t cmd_app_show_jpg_current(CmdApp *app)
{
  CmdPanel *panel;
  const cmd_file_entry_t *entry;
  esp_err_t err;
  int status_len;
  size_t used;
  size_t name_len;
  size_t dst_len;

  if (!app || !app->jpg_viewer_open) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_panel_by_index(app, app->jpg_viewer_panel);
  if (!panel) return ESP_ERR_INVALID_STATE;
  if (app->jpg_viewer_index >= panel->count) return ESP_ERR_INVALID_STATE;

  entry = &panel->entries[app->jpg_viewer_index];
  if (!cmd_app_entry_is_ft_image(entry)) return ESP_ERR_INVALID_STATE;

  err = cmd_app_build_entry_abs_path(app, panel, entry, app->jpg_viewer_path, sizeof(app->jpg_viewer_path));
  if (err != ESP_OK) return err;

  status_len = snprintf(app->jpg_viewer_status, sizeof(app->jpg_viewer_status),
    "%s %u/%u %s  ",
    cmd_app_entry_is_dxp(entry) ? "DXP" : "JPEG",
    (unsigned)(app->jpg_viewer_index + 1),
    (unsigned)panel->count,
    cmd_app_entry_is_dxp(entry) ? "center" : (app->jpg_viewer_stretch ? "stretch" : "center"));
  if (status_len < 0) return ESP_FAIL;

  if ((size_t)status_len >= sizeof(app->jpg_viewer_status))
  {
    app->jpg_viewer_status[sizeof(app->jpg_viewer_status) - 1] = 0;
  }
  else
  {
    used = (size_t)status_len;
    name_len = strlen(entry->name);
    dst_len = sizeof(app->jpg_viewer_status) - used - 1;
    if (name_len > dst_len) name_len = dst_len;
    memcpy(&app->jpg_viewer_status[used], entry->name, name_len);
    app->jpg_viewer_status[used + name_len] = 0;
  }

  cmd_set_status(app, "%s", app->jpg_viewer_status);

  if (cmd_app_entry_is_dxp(entry))
    return ft_dxp_show_file_mode(app->jpg_viewer_path, true);

  return ft_jpg_show_file_mode(app->jpg_viewer_path, app->jpg_viewer_stretch, true);
}

esp_err_t cmd_app_open_jpg_viewer(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  esp_err_t err;
  bool added_ft812_backend = false;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;
  if (!cmd_app_entry_is_ft_image(entry)) return ESP_ERR_INVALID_ARG;

  if (!cmd_app_has_ft812_backend(app))
  {
    err = cmd_display_mux_add_backend(&app->mux, &cmd_display_ft812_backend);
    if (err != ESP_OK)
    {
      cmd_show_error(app, "image viewer", err);
      return ESP_OK;
    }

    err = cmd_display_backend_init(&cmd_display_ft812_backend);
    if (err != ESP_OK)
    {
      cmd_display_mux_remove_backend(&app->mux, &cmd_display_ft812_backend);
      cmd_show_error(app, "image viewer", err);
      return ESP_OK;
    }

    added_ft812_backend = true;
  }

  app->jpg_viewer_open = true;
  app->jpg_viewer_added_ft812_backend = added_ft812_backend;
  app->jpg_viewer_panel = app->active_panel;
  app->jpg_viewer_index = panel->cursor;
  app->jpg_viewer_stretch = true;

  err = cmd_app_show_jpg_current(app);
  if (err != ESP_OK)
  {
    cmd_app_close_jpg_viewer(app);
    cmd_show_error(app, "image viewer", err);
  }

  return ESP_OK;
}

bool cmd_app_find_next_jpg(CmdApp *app, int dir, size_t *out_index)
{
  CmdPanel *panel;
  size_t count;

  if (!app || !out_index) return false;

  panel = cmd_app_panel_by_index(app, app->jpg_viewer_panel);
  if (!panel || panel->count == 0) return false;

  count = panel->count;
  for (size_t step = 1; step <= count; step++)
  {
    size_t index;

    if (dir > 0)
      index = (app->jpg_viewer_index + step) % count;
    else
      index = (app->jpg_viewer_index + count - (step % count)) % count;

    if (cmd_app_entry_is_ft_image(&panel->entries[index]))
    {
      *out_index = index;
      return true;
    }
  }

  return false;
}

esp_err_t cmd_app_jpg_viewer_step(CmdApp *app, int dir)
{
  CmdPanel *panel;
  size_t index;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  if (!cmd_app_find_next_jpg(app, dir, &index))
  {
    cmd_set_status(app, "no other image file");
    return ESP_OK;
  }

  panel = cmd_app_panel_by_index(app, app->jpg_viewer_panel);
  if (!panel) return ESP_ERR_INVALID_STATE;

  app->jpg_viewer_index = index;
  panel->cursor = index;
  cmd_panel_ensure_cursor_visible(panel);

  err = cmd_app_show_jpg_current(app);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "image viewer", err);
    cmd_app_close_jpg_viewer(app);
    return ESP_OK;
  }

  err = cmd_app_render_console(app);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "console render", err);
    cmd_app_close_jpg_viewer(app);
  }

  return ESP_OK;
}

esp_err_t cmd_app_jpg_viewer_set_stretch(CmdApp *app, bool stretch)
{
  esp_err_t err;

  if (!app || !app->jpg_viewer_open) return ESP_ERR_INVALID_ARG;

  CmdPanel *panel = cmd_app_panel_by_index(app, app->jpg_viewer_panel);
  if (!panel) return ESP_ERR_INVALID_STATE;
  if (app->jpg_viewer_index >= panel->count) return ESP_ERR_INVALID_STATE;

  const cmd_file_entry_t *entry = &panel->entries[app->jpg_viewer_index];
  if (cmd_app_entry_is_dxp(entry))
  {
    cmd_set_status(app, "DXP viewer: stretch is not supported");
    return ESP_OK;
  }

  app->jpg_viewer_stretch = stretch;
  err = ft_rgb888_640x480_show_current(stretch);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "JPEG display", err);
    cmd_app_close_jpg_viewer(app);
    return ESP_OK;
  }

  cmd_set_status(app, "JPEG %s", stretch ? "stretch" : "center");
  return ESP_OK;
}

esp_err_t cmd_app_handle_jpg_viewer_key(CmdApp *app, const cmd_event_t *event)
{
  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->jpg_viewer_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC || event->key == CMD_KEY_F10)
  {
    cmd_app_close_jpg_viewer(app);
    cmd_set_status(app, "image viewer closed");
    return ESP_OK;
  }

  if (event->key == CMD_KEY_PAGE_UP || event->key == CMD_KEY_LEFT)
    return cmd_app_jpg_viewer_step(app, -1);

  if (event->key == CMD_KEY_PAGE_DOWN || event->key == CMD_KEY_RIGHT)
    return cmd_app_jpg_viewer_step(app, 1);

  if (event->key == CMD_KEY_CHAR && event->ch == '*')
    return cmd_app_jpg_viewer_set_stretch(app, true);

  if (event->key == CMD_KEY_CHAR && event->ch == '/')
    return cmd_app_jpg_viewer_set_stretch(app, false);

  return ESP_OK;
}

int cmd_app_viewer_visible_rows(CmdApp *app)
{
  int rows;

  rows = cmd_app_screen_h(app) - 2;
  if (rows < 1) rows = 1;
  if (rows > CMD_APP_VIEWER_MAX_ROWS) rows = CMD_APP_VIEWER_MAX_ROWS;
  return rows;
}

int cmd_app_viewer_visible_cols(CmdApp *app)
{
  int cols;

  cols = cmd_app_screen_w(app);
  if (cols < 1) cols = 1;
  if (cols > CMD_DISPLAY_COLS) cols = CMD_DISPLAY_COLS;
  return cols;
}

void cmd_app_viewer_clear_page(CmdApp *app)
{
  if (!app) return;

  app->viewer_page_lines = 0;
  app->viewer_eof = true;

  for (int i = 0; i < CMD_APP_VIEWER_MAX_ROWS; i++)
    app->viewer_lines[i][0] = 0;
}

char cmd_app_viewer_char(uint8_t ch)
{
  if (ch == '\t') return ' ';
  if (ch < 32) return '.';
  return (char)ch;
}

void cmd_app_viewer_reset_stream(CmdApp *app)
{
  if (!app) return;

  app->viewer_read_pos = 0;
  app->viewer_read_len = 0;
  app->viewer_read_base = 0;
}

uint64_t cmd_app_viewer_stream_offset(CmdApp *app)
{
  if (!app) return 0;
  return app->viewer_read_base + app->viewer_read_pos;
}

esp_err_t cmd_app_viewer_close_file(CmdApp *app)
{
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;
  if (!app->viewer_file_open) return ESP_OK;

  err = cmd_fs_manager_close(&app->fs, &app->viewer_file);
  app->viewer_file_open = false;
  memset(&app->viewer_file, 0, sizeof(app->viewer_file));
  cmd_app_viewer_reset_stream(app);
  return err;
}

esp_err_t cmd_app_viewer_open_file(CmdApp *app)
{
  CmdFsDriver *driver;
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;

  err = cmd_app_viewer_close_file(app);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_mount(&app->fs, app->viewer_device);
  if (err != ESP_OK) return err;

  driver = cmd_fs_manager_get_driver(&app->fs, app->viewer_device);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->open_read || !driver->read || !driver->seek || !driver->tell || !driver->close)
    return ESP_ERR_NOT_SUPPORTED;

  memset(&app->viewer_file, 0, sizeof(app->viewer_file));
  err = cmd_fs_manager_open_read(&app->fs, app->viewer_device, app->viewer_path, &app->viewer_file);
  if (err != ESP_OK) return err;

  app->viewer_file_open = true;
  cmd_app_viewer_reset_stream(app);
  return ESP_OK;
}

esp_err_t cmd_app_viewer_seek_stream(CmdApp *app, uint64_t offset)
{
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;
  if (!app->viewer_file_open) return ESP_ERR_INVALID_STATE;
  if (offset > (uint64_t)INT64_MAX) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_seek(&app->fs, &app->viewer_file, (int64_t)offset, SEEK_SET);
  if (err != ESP_OK) return err;

  app->viewer_read_pos = 0;
  app->viewer_read_len = 0;
  app->viewer_read_base = offset;
  return ESP_OK;
}

esp_err_t cmd_app_viewer_read_line(CmdApp *app, char *out, size_t out_size, bool *out_have_line)
{
  size_t got = 0;
  size_t pos = 0;
  size_t cols;
  uint64_t file_pos = 0;
  bool have_line = false;
  esp_err_t err;

  if (!app || !out || out_size == 0 || !out_have_line) return ESP_ERR_INVALID_ARG;

  out[0] = 0;
  *out_have_line = false;

  if (!app->viewer_file_open) return ESP_ERR_INVALID_STATE;

  cols = (size_t)cmd_app_viewer_visible_cols(app);
  if (cols >= out_size) cols = out_size - 1;

  while (true)
  {
    if (app->viewer_read_pos >= app->viewer_read_len)
    {
      err = cmd_fs_manager_tell(&app->fs, &app->viewer_file, &file_pos);
      if (err != ESP_OK) return err;

      err = cmd_fs_manager_read(&app->fs, &app->viewer_file, app->viewer_read_buffer,
        sizeof(app->viewer_read_buffer), &got);
      if (err != ESP_OK) return err;

      app->viewer_read_base = file_pos;
      app->viewer_read_pos = 0;
      app->viewer_read_len = got;

      if (got == 0)
      {
        if (!have_line) return ESP_OK;
        out[pos] = 0;
        *out_have_line = true;
        return ESP_OK;
      }
    }

    uint8_t ch = app->viewer_read_buffer[app->viewer_read_pos++];
    if (ch == '\r') continue;

    if (ch == '\n')
    {
      out[pos] = 0;
      *out_have_line = true;
      return ESP_OK;
    }

    have_line = true;
    if (pos < cols)
    {
      out[pos] = cmd_app_viewer_char(ch);
      pos++;
    }
  }
}

esp_err_t cmd_app_viewer_read_page_from_offset(CmdApp *app, uint64_t offset)
{
  size_t rows;
  bool have_line;
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;

  err = cmd_app_viewer_seek_stream(app, offset);
  if (err != ESP_OK) return err;

  cmd_app_viewer_clear_page(app);

  rows = (size_t)cmd_app_viewer_visible_rows(app);
  app->viewer_top_offset = offset;
  app->viewer_next_offset = offset;
  app->viewer_next_line = app->viewer_top_line;
  app->viewer_eof = false;

  while (app->viewer_page_lines < rows)
  {
    app->viewer_line_offsets[app->viewer_page_lines] = cmd_app_viewer_stream_offset(app);

    err = cmd_app_viewer_read_line(app,
      app->viewer_lines[app->viewer_page_lines],
      sizeof(app->viewer_lines[app->viewer_page_lines]),
      &have_line);
    if (err != ESP_OK) return err;

    if (!have_line)
    {
      app->viewer_eof = true;
      app->viewer_next_offset = cmd_app_viewer_stream_offset(app);
      app->viewer_line_offsets[app->viewer_page_lines] = app->viewer_next_offset;
      return ESP_OK;
    }

    app->viewer_page_lines++;
    app->viewer_next_line++;
  }

  app->viewer_next_offset = cmd_app_viewer_stream_offset(app);
  app->viewer_line_offsets[app->viewer_page_lines] = app->viewer_next_offset;
  return ESP_OK;
}

void cmd_app_viewer_set_status(CmdApp *app)
{
  if (!app) return;

  cmd_set_status(app, "view: %s  line %llu",
    app->viewer_title[0] ? app->viewer_title : cmd_fs_filename_ptr(app->viewer_path),
    (unsigned long long)(app->viewer_top_line + 1));
}

esp_err_t cmd_app_viewer_find_prev_offset(CmdApp *app, uint64_t before_offset, size_t lines_back,
  uint64_t *out_offset, size_t *out_lines)
{
  uint64_t *candidates;
  uint8_t *read_buffer;
  size_t candidate_count;
  size_t total_count;
  size_t capacity = CMD_APP_VIEWER_MAX_ROWS + 1;
  size_t got = 0;
  uint64_t window;
  uint64_t start;
  uint64_t pos;
  uint64_t left;
  esp_err_t err;

  if (!app || !out_offset || !out_lines) return ESP_ERR_INVALID_ARG;
  if (!app->viewer_file_open) return ESP_ERR_INVALID_STATE;

  candidates = app->viewer_prev_offsets;
  read_buffer = app->viewer_seek_buffer;

  *out_offset = 0;
  *out_lines = 0;
  if (before_offset == 0 || lines_back == 0) return ESP_OK;

  if (lines_back > CMD_APP_VIEWER_MAX_ROWS) lines_back = CMD_APP_VIEWER_MAX_ROWS;
  window = (uint64_t)CMD_APP_VIEWER_READ_BUFFER_SIZE * (lines_back + 2);

  while (true)
  {
    start = before_offset > window ? before_offset - window : 0;
    candidate_count = 0;
    total_count = 0;
    pos = start;

    if (start == 0)
    {
      candidates[candidate_count++] = 0;
      total_count++;
    }

    err = cmd_fs_manager_seek(&app->fs, &app->viewer_file, (int64_t)start, SEEK_SET);
    if (err != ESP_OK) return err;

    while (pos < before_offset)
    {
      left = before_offset - pos;
      size_t want = CMD_APP_VIEWER_READ_BUFFER_SIZE;
      if (left < want) want = (size_t)left;

      err = cmd_fs_manager_read(&app->fs, &app->viewer_file, read_buffer, want, &got);
      if (err != ESP_OK) return err;
      if (got == 0) break;

      for (size_t i = 0; i < got && pos < before_offset; i++)
      {
        uint8_t ch = read_buffer[i];
        pos++;
        if (ch != '\n' || pos >= before_offset) continue;

        if (candidate_count < capacity)
        {
          candidates[candidate_count++] = pos;
        }
        else
        {
          memmove(candidates, candidates + 1, (capacity - 1) * sizeof(candidates[0]));
          candidates[capacity - 1] = pos;
        }
        total_count++;
      }
    }

    if (candidate_count >= lines_back)
    {
      *out_offset = candidates[candidate_count - lines_back];
      *out_lines = lines_back;
      return ESP_OK;
    }

    if (start == 0)
    {
      if (candidate_count > 0)
      {
        *out_offset = candidates[0];
        *out_lines = total_count;
      }
      return ESP_OK;
    }

    if (window >= before_offset) window = before_offset;
    else window *= 2;
  }
}

esp_err_t cmd_app_viewer_scroll_down(CmdApp *app)
{
  bool have_line;
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;
  if (app->viewer_page_lines == 0) return ESP_OK;
  if (app->viewer_eof && app->viewer_page_lines <= 1) return ESP_OK;

  uint64_t new_top_offset = app->viewer_page_lines > 1 ?
    app->viewer_line_offsets[1] : app->viewer_next_offset;

  for (size_t i = 1; i < app->viewer_page_lines; i++)
  {
    memcpy(app->viewer_lines[i - 1], app->viewer_lines[i], sizeof(app->viewer_lines[i - 1]));
    app->viewer_line_offsets[i - 1] = app->viewer_line_offsets[i];
  }

  app->viewer_page_lines--;
  app->viewer_top_line++;
  app->viewer_top_offset = new_top_offset;
  app->viewer_line_offsets[0] = new_top_offset;

  if (!app->viewer_eof)
  {
    app->viewer_line_offsets[app->viewer_page_lines] = app->viewer_next_offset;

    err = cmd_app_viewer_read_line(app,
      app->viewer_lines[app->viewer_page_lines],
      sizeof(app->viewer_lines[app->viewer_page_lines]),
      &have_line);
    if (err != ESP_OK) return err;

    if (have_line)
    {
      app->viewer_page_lines++;
      app->viewer_next_line++;
      app->viewer_next_offset = cmd_app_viewer_stream_offset(app);
      app->viewer_line_offsets[app->viewer_page_lines] = app->viewer_next_offset;
    }
    else
    {
      app->viewer_eof = true;
      app->viewer_next_offset = cmd_app_viewer_stream_offset(app);
      app->viewer_line_offsets[app->viewer_page_lines] = app->viewer_next_offset;
    }
  }

  cmd_app_viewer_set_status(app);
  return ESP_OK;
}

esp_err_t cmd_app_viewer_page_down(CmdApp *app)
{
  size_t rows;
  uint64_t old_offset;
  size_t old_top_line;
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;
  if (app->viewer_page_lines == 0) return ESP_OK;

  rows = (size_t)cmd_app_viewer_visible_rows(app);

  if (app->viewer_eof)
  {
    size_t step = app->viewer_page_lines > 1 ? app->viewer_page_lines - 1 : 0;
    while (step > 0)
    {
      err = cmd_app_viewer_scroll_down(app);
      if (err != ESP_OK) return err;
      step--;
    }
    return ESP_OK;
  }

  old_offset = app->viewer_top_offset;
  old_top_line = app->viewer_top_line;
  app->viewer_top_line += rows;

  err = cmd_app_viewer_read_page_from_offset(app, app->viewer_next_offset);
  if (err != ESP_OK) return err;

  if (app->viewer_page_lines == 0)
  {
    app->viewer_top_line = old_top_line;
    err = cmd_app_viewer_read_page_from_offset(app, old_offset);
    if (err != ESP_OK) return err;
  }

  cmd_app_viewer_set_status(app);
  return ESP_OK;
}

esp_err_t cmd_app_viewer_move_up(CmdApp *app, size_t lines)
{
  uint64_t old_offset;
  uint64_t new_offset = 0;
  size_t old_top_line;
  size_t moved = 0;
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;
  if (lines == 0 || app->viewer_top_offset == 0 || app->viewer_top_line == 0) return ESP_OK;

  old_offset = app->viewer_top_offset;
  old_top_line = app->viewer_top_line;

  err = cmd_app_viewer_find_prev_offset(app, app->viewer_top_offset, lines, &new_offset, &moved);
  if (err != ESP_OK) return err;

  if (moved == 0 && new_offset == old_offset) return ESP_OK;

  if (old_top_line > moved) app->viewer_top_line = old_top_line - moved;
  else app->viewer_top_line = 0;

  err = cmd_app_viewer_read_page_from_offset(app, new_offset);
  if (err != ESP_OK)
  {
    app->viewer_top_line = old_top_line;
    cmd_app_viewer_read_page_from_offset(app, old_offset);
    return err;
  }

  cmd_app_viewer_set_status(app);
  return ESP_OK;
}

esp_err_t cmd_app_viewer_home(CmdApp *app)
{
  esp_err_t err;

  if (!app || !app->viewer_open) return ESP_ERR_INVALID_ARG;

  app->viewer_top_line = 0;
  err = cmd_app_viewer_read_page_from_offset(app, 0);
  if (err != ESP_OK) return err;

  cmd_app_viewer_set_status(app);
  return ESP_OK;
}

void cmd_app_close_text_viewer(CmdApp *app)
{
  if (!app) return;

  cmd_app_viewer_close_file(app);
  app->viewer_open = false;
  app->viewer_device = CMD_DEVICE_INVALID;
  app->viewer_path[0] = 0;
  app->viewer_title[0] = 0;
  app->viewer_top_offset = 0;
  app->viewer_next_offset = 0;
  app->viewer_top_line = 0;
  app->viewer_next_line = 0;
  cmd_app_viewer_reset_stream(app);
  cmd_app_viewer_clear_page(app);
  cmd_app_set_status(app, "ready");
}

esp_err_t cmd_app_open_text_viewer(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  esp_err_t err;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;

  if (entry->type != CMD_ENTRY_FILE)
  {
    cmd_app_set_status(app, "view supports files only");
    return ESP_OK;
  }

  err = cmd_app_join_selected_path(panel, entry, app->viewer_path, sizeof(app->viewer_path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "view", err);
    return ESP_OK;
  }

  app->viewer_open = true;
  app->viewer_device = panel->device_id;
  app->viewer_top_offset = 0;
  app->viewer_next_offset = 0;
  app->viewer_top_line = 0;
  app->viewer_next_line = 0;
  app->viewer_eof = false;
  cmd_app_viewer_reset_stream(app);
  cmd_app_copy_string(app->viewer_title, sizeof(app->viewer_title), entry->name);

  err = cmd_app_viewer_open_file(app);
  if (err == ESP_OK) err = cmd_app_viewer_read_page_from_offset(app, 0);
  if (err != ESP_OK)
  {
    cmd_app_close_text_viewer(app);
    cmd_show_error(app, "view", err);
    return ESP_OK;
  }

  cmd_app_viewer_set_status(app);
  return ESP_OK;
}

esp_err_t cmd_app_handle_text_viewer_key(CmdApp *app, const cmd_event_t *event)
{
  size_t rows;
  esp_err_t err;

  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->viewer_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC)
  {
    cmd_app_close_text_viewer(app);
    return ESP_OK;
  }

  rows = (size_t)cmd_app_viewer_visible_rows(app);

  switch (event->key)
  {
    case CMD_KEY_UP:
      if (app->viewer_top_line > 0)
      {
        err = cmd_app_viewer_move_up(app, 1);
        if (err != ESP_OK)
        {
          cmd_show_error(app, "view", err);
          return ESP_OK;
        }
      }
      return ESP_OK;

    case CMD_KEY_DOWN:
      err = cmd_app_viewer_scroll_down(app);
      if (err != ESP_OK)
      {
        cmd_show_error(app, "view", err);
        return ESP_OK;
      }
      return ESP_OK;

    case CMD_KEY_PAGE_UP:
    case CMD_KEY_LEFT:
      if (app->viewer_top_line > 0)
      {
        err = cmd_app_viewer_move_up(app, rows);
        if (err != ESP_OK)
        {
          cmd_show_error(app, "view", err);
          return ESP_OK;
        }
      }
      return ESP_OK;

    case CMD_KEY_PAGE_DOWN:
    case CMD_KEY_RIGHT:
      err = cmd_app_viewer_page_down(app);
      if (err != ESP_OK)
      {
        cmd_show_error(app, "view", err);
        return ESP_OK;
      }
      return ESP_OK;

    case CMD_KEY_HOME:
      err = cmd_app_viewer_home(app);
      if (err != ESP_OK)
      {
        cmd_show_error(app, "view", err);
        return ESP_OK;
      }
      return ESP_OK;

    default:
      return ESP_OK;
  }
}

void cmd_app_make_viewer_title(char *out, size_t out_size, CmdApp *app)
{
  if (!out || out_size == 0) return;

  cmd_app_text_clear(out, out_size);
  if (!app) return;

  cmd_app_append_text(out, out_size, "View: ");
  cmd_app_append_text(out, out_size, cmd_device_name(app->viewer_device));
  cmd_app_append_text(out, out_size, ":");
  cmd_app_append_text(out, out_size, app->viewer_path);
  cmd_app_append_text(out, out_size, "  line ");
  snprintf(app->viewer_num, sizeof(app->viewer_num), "%llu", (unsigned long long)(app->viewer_top_line + 1));
  cmd_app_append_text(out, out_size, app->viewer_num);
}

void cmd_app_draw_text_viewer(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_rect_t line_rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t bar_attr;
  int rows;
  int cols;
  int bottom_y;

  if (!app || !app->viewer_open) return;

  rows = cmd_app_viewer_visible_rows(app);
  cols = cmd_app_viewer_visible_cols(app);
  bottom_y = cmd_app_bottom_y(app);
  attr = cmd_app_normal_attr();
  bar_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_YELLOW, CMD_DISPLAY_COLOR_CYAN,
    CMD_DISPLAY_ATTR_FLAG_BOLD);

  rect.x = 0;
  rect.y = 0;
  rect.w = cmd_app_screen_w(app);
  rect.h = cmd_app_screen_h(app);

  line_rect.x = 0;
  line_rect.y = 0;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_app_make_viewer_title(app->viewer_draw_line, sizeof(app->viewer_draw_line), app);
  cmd_display_buffer_write_text(&app->buffer, 0, 0, app->viewer_draw_line, bar_attr);

  for (int row = 0; row < rows; row++)
  {
    line_rect.x = 0;
    line_rect.y = 1 + row;
    line_rect.w = cols;
    line_rect.h = 1;
    cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', attr);

    if ((size_t)row < app->viewer_page_lines)
      cmd_display_buffer_write_text(&app->buffer, 0, line_rect.y, app->viewer_lines[row], attr);
  }

  line_rect.x = 0;
  line_rect.y = bottom_y;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_display_buffer_write_text(&app->buffer, 0, bottom_y,
    "Viewer  Up/Down PgUp/PgDn Left/Right Home  Esc Close", bar_attr);
}


uint16_t cmd_app_info_rd_le16(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t cmd_app_info_rd_le24(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

uint32_t cmd_app_info_rd_le32(const uint8_t *p)
{
  return (uint32_t)cmd_app_info_rd_le16(p) | ((uint32_t)cmd_app_info_rd_le16(p + 2) << 16);
}

bool cmd_app_module_info_u64_add(uint64_t *value, uint64_t add)
{
  if (!value) return false;
  if (UINT64_MAX - *value < add) return false;
  *value += add;
  return true;
}

void cmd_app_module_info_copy_text(char *dst, size_t dst_size, const uint8_t *src, size_t src_size)
{
  size_t n;

  if (!dst || dst_size == 0) return;

  dst[0] = 0;
  if (!src) return;

  n = src_size;
  while (n > 0 && (src[n - 1] == 0 || src[n - 1] == ' ')) n--;
  if (n >= dst_size) n = dst_size - 1;

  for (size_t i = 0; i < n; i++)
  {
    uint8_t ch = src[i];
    dst[i] = (ch >= 32 && ch <= 126) ? (char)ch : '.';
  }
  dst[n] = 0;
}

void cmd_app_module_info_free_text(CmdApp *app)
{
  if (!app) return;

  if (app->module_info_text)
    heap_caps_free(app->module_info_text);

  app->module_info_text = NULL;
  app->module_info_text_len = 0;
  app->module_info_text_cap = 0;
  app->module_info_top_line = 0;
  app->module_info_line_count = 0;
}

esp_err_t cmd_app_module_info_reserve(CmdApp *app, size_t needed)
{
  char *new_text;
  size_t new_cap;

  if (!app) return ESP_ERR_INVALID_ARG;
  if (needed > CMD_APP_MODULE_INFO_MAX_TEXT_SIZE) return ESP_ERR_NO_MEM;
  if (needed <= app->module_info_text_cap) return ESP_OK;

  new_cap = app->module_info_text_cap ? app->module_info_text_cap : CMD_APP_MODULE_INFO_INITIAL_CAP;
  while (new_cap < needed)
  {
    if (new_cap > CMD_APP_MODULE_INFO_MAX_TEXT_SIZE / 2)
    {
      new_cap = CMD_APP_MODULE_INFO_MAX_TEXT_SIZE;
      break;
    }
    new_cap *= 2;
  }

  if (new_cap < needed) return ESP_ERR_NO_MEM;

  new_text = (char*)heap_caps_realloc(app->module_info_text, new_cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!new_text) return ESP_ERR_NO_MEM;

  app->module_info_text = new_text;
  app->module_info_text_cap = new_cap;
  return ESP_OK;
}

esp_err_t cmd_app_module_info_append_line(CmdApp *app, const char *fmt, ...)
{
  va_list ap;
  int written;
  size_t line_len;
  size_t needed;
  esp_err_t err;

  if (!app || !fmt) return ESP_ERR_INVALID_ARG;

  va_start(ap, fmt);
  written = vsnprintf(app->module_info_tmp, sizeof(app->module_info_tmp), fmt, ap);
  va_end(ap);

  if (written < 0) return ESP_FAIL;

  app->module_info_tmp[sizeof(app->module_info_tmp) - 1] = 0;
  line_len = strlen(app->module_info_tmp);
  needed = app->module_info_text_len + line_len + 2;

  err = cmd_app_module_info_reserve(app, needed);
  if (err != ESP_OK) return err;

  memcpy(app->module_info_text + app->module_info_text_len, app->module_info_tmp, line_len);
  app->module_info_text_len += line_len;
  app->module_info_text[app->module_info_text_len++] = '\n';
  app->module_info_text[app->module_info_text_len] = 0;
  app->module_info_line_count++;
  return ESP_OK;
}

esp_err_t cmd_app_module_info_read_at(CmdApp *app, CmdFsFile *file, uint64_t offset, void *dst, size_t size, size_t *out_got)
{
  size_t got = 0;
  esp_err_t err;

  if (!app || !file || !dst) return ESP_ERR_INVALID_ARG;
  if (offset > (uint64_t)INT64_MAX) return ESP_ERR_INVALID_ARG;

  memset(dst, 0, size);

  err = cmd_fs_manager_seek(&app->fs, file, (int64_t)offset, SEEK_SET);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_read(&app->fs, file, dst, size, &got);
  if (err != ESP_OK) return err;

  if (out_got) *out_got = got;
  return ESP_OK;
}

const char *cmd_app_module_info_mod_magic_text(CmdApp *app, const uint8_t *header, const mod_layout_t *layout)
{
  if (!app || !header || !layout) return "?";

  if (layout->sample_count != MOD_SAMPLE_COUNT) return "15-sample";

  cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), header + MOD_MAGIC_OFFSET, 4);
  return app->module_info_name[0] ? app->module_info_name : "?";
}

const char *cmd_app_module_info_mod_loop_text(const mod_layout_t *layout, uint16_t index)
{
  if (!layout || index >= layout->sample_count) return "no";
  if (layout->sample_length[index] == 0) return "no";
  if (layout->sample_loop_start[index] >= layout->sample_length[index]) return "no";
  if (layout->sample_loop_length[index] <= 2) return "no";
  return "yes";
}

esp_err_t cmd_app_module_info_build_mod(CmdApp *app, CmdFsFile *file, const cmd_file_entry_t *entry)
{
  mod_layout_t layout;
  uint8_t *header;
  size_t want;
  size_t got = 0;
  esp_err_t err;

  if (!app || !file || !entry) return ESP_ERR_INVALID_ARG;

  header = app->module_info_read_buffer;
  want = entry->size < CMD_APP_MODULE_INFO_READ_BUFFER_SIZE ? (size_t)entry->size : CMD_APP_MODULE_INFO_READ_BUFFER_SIZE;
  err = cmd_app_module_info_read_at(app, file, 0, header, want, &got);
  if (err != ESP_OK) return err;
  if (got < MOD_15_HEADER_SIZE) return ESP_FAIL;
  if (!mod_read_layout_from_header(header, got, (size_t)entry->size, &layout)) return ESP_FAIL;

  cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), header, MOD_TITLE_SIZE);

  err = cmd_app_module_info_append_line(app, "MOD module: %s", app->module_info_name[0] ? app->module_info_name : "<unnamed>");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "File: %s", entry->name);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Format: %s  channels=%u  orders=%u  patterns=%u  restart=%u",
    cmd_app_module_info_mod_magic_text(app, header, &layout),
    (unsigned)layout.channels,
    (unsigned)layout.song_length,
    (unsigned)layout.patterns,
    (unsigned)layout.restart_position);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Expected size=%u  file size=%llu  sample bytes=%u",
    (unsigned)layout.expected_size,
    (unsigned long long)entry->size,
    (unsigned)layout.sample_bytes);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Samples / instruments:");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, " #  Name                    LenB   LoopSt LoopLen Loop Vol Fine");
  if (err != ESP_OK) return err;

  for (uint16_t i = 0; i < layout.sample_count; i++)
  {
    size_t off = MOD_TITLE_SIZE + (size_t)i * MOD_SAMPLE_HEADER_SIZE;
    uint8_t volume = header[off + 25];
    int finetune = header[off + 24] & 0x0f;

    if (finetune >= 8) finetune -= 16;
    if (volume > 64) volume = 64;

    cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), header + off, 22);
    err = cmd_app_module_info_append_line(app, "%02u  %-22s %6u %6u %7u %-4s %3u %4d",
      (unsigned)i + 1,
      app->module_info_name,
      (unsigned)layout.sample_length[i],
      (unsigned)layout.sample_loop_start[i],
      (unsigned)layout.sample_loop_length[i],
      cmd_app_module_info_mod_loop_text(&layout, i),
      (unsigned)volume,
      finetune);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}

const char *cmd_app_module_info_xm_loop_text(uint8_t flags)
{
  switch (flags & 0x03)
  {
    case 1: return "fwd";
    case 2: return "ping";
    default: return "no";
  }
}

esp_err_t cmd_app_module_info_skip_xm_patterns(CmdApp *app, CmdFsFile *file, uint64_t *offset, uint16_t num_patterns, uint64_t file_size)
{
  uint8_t *buf;

  if (!app || !file || !offset) return ESP_ERR_INVALID_ARG;

  buf = app->module_info_read_buffer;

  for (uint16_t i = 0; i < num_patterns; i++)
  {
    size_t got = 0;
    uint32_t header_size;
    uint16_t rows;
    uint16_t packed_size;
    uint64_t next;
    esp_err_t err;

    err = cmd_app_module_info_read_at(app, file, *offset, buf, 9, &got);
    if (err != ESP_OK) return err;
    if (got < 9) return ESP_FAIL;

    header_size = cmd_app_info_rd_le32(buf);
    rows = cmd_app_info_rd_le16(buf + 5);
    packed_size = cmd_app_info_rd_le16(buf + 7);
    if (header_size < 9 || rows == 0 || rows > CMD_APP_XM_MAX_ROWS) return ESP_FAIL;

    next = *offset;
    if (!cmd_app_module_info_u64_add(&next, header_size)) return ESP_ERR_INVALID_SIZE;
    if (!cmd_app_module_info_u64_add(&next, packed_size)) return ESP_ERR_INVALID_SIZE;
    if (next > file_size) return ESP_FAIL;
    *offset = next;
  }

  return ESP_OK;
}

esp_err_t cmd_app_module_info_build_xm(CmdApp *app, CmdFsFile *file, const cmd_file_entry_t *entry)
{
  uint8_t *buf;
  uint32_t header_size;
  uint16_t version;
  uint16_t length;
  uint16_t restart;
  uint16_t channels;
  uint16_t patterns;
  uint16_t instruments;
  uint16_t flags;
  uint16_t tempo;
  uint16_t bpm;
  uint64_t offset;
  uint64_t instruments_offset;
  size_t got = 0;
  esp_err_t err;

  if (!app || !file || !entry) return ESP_ERR_INVALID_ARG;

  buf = app->module_info_read_buffer;
  err = cmd_app_module_info_read_at(app, file, 0, buf, 336, &got);
  if (err != ESP_OK) return err;
  if (got < 80) return ESP_FAIL;
  if (memcmp(buf, "Extended Module: ", 17) != 0) return ESP_FAIL;
  if (buf[37] != 0x1a) return ESP_FAIL;

  header_size = cmd_app_info_rd_le32(buf + 60);
  if (header_size < 20) return ESP_FAIL;
  if ((uint64_t)60 + header_size > entry->size) return ESP_FAIL;

  version = cmd_app_info_rd_le16(buf + 58);
  length = cmd_app_info_rd_le16(buf + 64);
  restart = cmd_app_info_rd_le16(buf + 66);
  channels = cmd_app_info_rd_le16(buf + 68);
  patterns = cmd_app_info_rd_le16(buf + 70);
  instruments = cmd_app_info_rd_le16(buf + 72);
  flags = cmd_app_info_rd_le16(buf + 74);
  tempo = cmd_app_info_rd_le16(buf + 76);
  bpm = cmd_app_info_rd_le16(buf + 78);

  cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), buf + 17, 20);
  cmd_app_module_info_copy_text(app->module_info_sample_name, sizeof(app->module_info_sample_name), buf + 38, 20);

  err = cmd_app_module_info_append_line(app, "XM module: %s", app->module_info_name[0] ? app->module_info_name : "<unnamed>");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "File: %s", entry->name);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Tracker: %s  version=%u.%02u",
    app->module_info_sample_name[0] ? app->module_info_sample_name : "?",
    (unsigned)(version >> 8),
    (unsigned)(version & 0xff));
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Orders=%u  restart=%u  channels=%u  patterns=%u  instruments=%u",
    (unsigned)length,
    (unsigned)restart,
    (unsigned)channels,
    (unsigned)patterns,
    (unsigned)instruments);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Tempo=%u  BPM=%u  frequency=%s  file size=%llu",
    (unsigned)tempo,
    (unsigned)bpm,
    (flags & 1) ? "linear" : "amiga",
    (unsigned long long)entry->size);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "");
  if (err != ESP_OK) return err;

  offset = (uint64_t)60 + header_size;
  err = cmd_app_module_info_skip_xm_patterns(app, file, &offset, patterns, entry->size);
  if (err != ESP_OK) return err;

  instruments_offset = offset;

  err = cmd_app_module_info_append_line(app, "Instruments:");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Idx Samples Name");
  if (err != ESP_OK) return err;

  for (uint8_t pass = 0; pass < 2; pass++)
  {
    offset = instruments_offset;

    if (pass == 1)
    {
      err = cmd_app_module_info_append_line(app, "");
      if (err != ESP_OK) return err;
      err = cmd_app_module_info_append_line(app, "Samples:");
      if (err != ESP_OK) return err;
      err = cmd_app_module_info_append_line(app, "Ins Smp Name                    LenB   LoopSt LoopLen Loop Bit Vol Pan Rel Fine");
      if (err != ESP_OK) return err;
    }

    for (uint16_t i = 0; i < instruments; i++)
    {
      uint32_t instr_header_size;
      uint16_t sample_count;
      uint32_t sample_header_size = 0;
      uint64_t sample_data_bytes = 0;

      err = cmd_app_module_info_read_at(app, file, offset, buf, 263, &got);
      if (err != ESP_OK) return err;
      if (got < 29) return ESP_FAIL;

      instr_header_size = cmd_app_info_rd_le32(buf);
      if (instr_header_size < 29) return ESP_FAIL;

      sample_count = cmd_app_info_rd_le16(buf + 27);
      if (sample_count > 0)
      {
        if (got < 33) return ESP_FAIL;
        sample_header_size = cmd_app_info_rd_le32(buf + 29);
        if (sample_header_size < 18) return ESP_FAIL;
      }

      if (pass == 0)
      {
        cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), buf + 4, 22);
        err = cmd_app_module_info_append_line(app, "%03u %7u %s",
          (unsigned)i + 1,
          (unsigned)sample_count,
          app->module_info_name);
        if (err != ESP_OK) return err;
      }

      if (!cmd_app_module_info_u64_add(&offset, instr_header_size)) return ESP_ERR_INVALID_SIZE;
      if (offset > entry->size) return ESP_FAIL;

      for (uint16_t j = 0; j < sample_count; j++)
      {
        uint32_t sample_len;
        uint32_t loop_start;
        uint32_t loop_len;
        uint8_t volume;
        uint8_t sample_flags;
        uint8_t panning;
        int rel_note;
        int finetune;

        err = cmd_app_module_info_read_at(app, file, offset, buf, sample_header_size < 40 ? sample_header_size : 40, &got);
        if (err != ESP_OK) return err;
        if (got < sample_header_size && got < 18) return ESP_FAIL;

        sample_len = cmd_app_info_rd_le32(buf);

        if (pass == 1)
        {
          loop_start = cmd_app_info_rd_le32(buf + 4);
          loop_len = cmd_app_info_rd_le32(buf + 8);
          volume = buf[12];
          finetune = (int8_t)buf[13];
          sample_flags = buf[14];
          panning = buf[15];
          rel_note = (int8_t)buf[16];
          cmd_app_module_info_copy_text(app->module_info_sample_name, sizeof(app->module_info_sample_name), buf + 18, got >= 40 ? 22 : 0);

          err = cmd_app_module_info_append_line(app, "%03u %03u %-22s %6u %6u %7u %-4s %3u %3u %3u %3d %4d",
            (unsigned)i + 1,
            (unsigned)j + 1,
            app->module_info_sample_name,
            (unsigned)sample_len,
            (unsigned)loop_start,
            (unsigned)loop_len,
            cmd_app_module_info_xm_loop_text(sample_flags),
            (sample_flags & 0x10) ? 16u : 8u,
            (unsigned)volume,
            (unsigned)panning,
            rel_note,
            finetune);
          if (err != ESP_OK) return err;
        }

        sample_data_bytes += sample_len;
        if (!cmd_app_module_info_u64_add(&offset, sample_header_size)) return ESP_ERR_INVALID_SIZE;
        if (offset > entry->size) return ESP_FAIL;
      }

      if (!cmd_app_module_info_u64_add(&offset, sample_data_bytes)) return ESP_ERR_INVALID_SIZE;
      if (offset > entry->size) return ESP_FAIL;
    }
  }

  return ESP_OK;
}


esp_err_t cmd_app_module_info_build_s3m(CmdApp *app, CmdFsFile *file, const cmd_file_entry_t *entry)
{
  uint8_t *buf;
  uint16_t order_count;
  uint16_t sample_count;
  uint16_t pattern_count;
  uint16_t flags;
  uint16_t cwtv;
  uint16_t format_version;
  uint16_t playable_orders = 0;
  uint16_t max_order_pattern = 0;
  uint16_t channels = 0;
  uint64_t tables_size;
  uint64_t sample_ptr_offset;
  size_t got = 0;
  esp_err_t err;

  if (!app || !file || !entry) return ESP_ERR_INVALID_ARG;

  buf = app->module_info_read_buffer;
  err = cmd_app_module_info_read_at(app, file, 0, buf, S3M_HEADER_SIZE, &got);
  if (err != ESP_OK) return err;
  if (got < S3M_HEADER_SIZE) return ESP_FAIL;
  if (memcmp(buf + S3M_MAGIC_OFFSET, "SCRM", 4) != 0) return ESP_FAIL;
  if (buf[CMD_APP_S3M_FILE_TYPE_OFFSET] != CMD_APP_S3M_FILE_TYPE_MODULE) return ESP_FAIL;

  order_count = cmd_app_info_rd_le16(buf + CMD_APP_S3M_ORDERS_OFFSET);
  sample_count = cmd_app_info_rd_le16(buf + CMD_APP_S3M_SAMPLES_OFFSET);
  pattern_count = cmd_app_info_rd_le16(buf + CMD_APP_S3M_PATTERNS_OFFSET);
  flags = cmd_app_info_rd_le16(buf + CMD_APP_S3M_FLAGS_OFFSET);
  cwtv = cmd_app_info_rd_le16(buf + CMD_APP_S3M_CWTV_OFFSET);
  format_version = cmd_app_info_rd_le16(buf + CMD_APP_S3M_FORMAT_VERSION_OFFSET);

  if (!order_count || order_count > S3M_MAX_ORDERS) return ESP_FAIL;
  if (sample_count > S3M_MAX_SAMPLES) return ESP_FAIL;
  if (!pattern_count || pattern_count > S3M_MAX_PATTERNS) return ESP_FAIL;

  tables_size = (uint64_t)order_count + (uint64_t)sample_count * 2 + (uint64_t)pattern_count * 2;
  if ((uint64_t)S3M_HEADER_SIZE + tables_size > entry->size) return ESP_FAIL;
  if (buf[CMD_APP_S3M_PANNING_TABLE_FLAG_OFFSET] == CMD_APP_S3M_PANNING_TABLE_PRESENT &&
    (uint64_t)S3M_HEADER_SIZE + tables_size + S3M_MAX_CHANNELS > entry->size) return ESP_FAIL;

  for (uint16_t i = 0; i < S3M_MAX_CHANNELS; i++)
    if (buf[CMD_APP_S3M_CHANNEL_SETTINGS_OFFSET + i] != 0xFF)
      channels++;

  if (channels < S3M_MIN_CHANNELS || channels > S3M_MAX_CHANNELS) return ESP_FAIL;

  err = cmd_app_module_info_read_at(app, file, S3M_HEADER_SIZE, buf, order_count, &got);
  if (err != ESP_OK) return err;
  if (got < order_count) return ESP_FAIL;

  for (uint16_t i = 0; i < order_count; i++)
  {
    uint8_t order = buf[i];

    if (order == 0xFF) break;
    if (order == 0xFE) continue;
    if (order >= pattern_count) return ESP_FAIL;
    playable_orders++;
    if (order > max_order_pattern) max_order_pattern = order;
  }

  if (!playable_orders) return ESP_FAIL;

  err = cmd_app_module_info_read_at(app, file, 0, buf, S3M_HEADER_SIZE, &got);
  if (err != ESP_OK) return err;
  if (got < S3M_HEADER_SIZE) return ESP_FAIL;
  cmd_app_module_info_copy_text(app->module_info_name, sizeof(app->module_info_name), buf, 28);

  err = cmd_app_module_info_append_line(app, "S3M module: %s", app->module_info_name[0] ? app->module_info_name : "<unnamed>");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "File: %s", entry->name);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Orders=%u playable=%u channels=%u patterns=%u samples=%u",
    (unsigned)order_count,
    (unsigned)playable_orders,
    (unsigned)channels,
    (unsigned)pattern_count,
    (unsigned)sample_count);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Speed=%u Tempo=%u GlobalVol=%u MasterVol=%u %s panning=%s",
    (unsigned)buf[CMD_APP_S3M_SPEED_OFFSET],
    (unsigned)buf[CMD_APP_S3M_TEMPO_OFFSET],
    (unsigned)buf[CMD_APP_S3M_GLOBAL_VOLUME_OFFSET],
    (unsigned)(buf[CMD_APP_S3M_MASTER_VOLUME_OFFSET] & 0x7F),
    (buf[CMD_APP_S3M_MASTER_VOLUME_OFFSET] & 0x80) ? "stereo" : "mono",
    buf[CMD_APP_S3M_PANNING_TABLE_FLAG_OFFSET] == CMD_APP_S3M_PANNING_TABLE_PRESENT ? "table" : "default");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Flags=0x%04X  CWT/V=0x%04X  sample format=%s  max order pattern=%u  file size=%llu",
    (unsigned)flags,
    (unsigned)cwtv,
    format_version == 1 ? "signed" : (format_version == 2 ? "unsigned" : "unknown"),
    (unsigned)max_order_pattern,
    (unsigned long long)entry->size);
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, "Samples / instruments:");
  if (err != ESP_OK) return err;
  err = cmd_app_module_info_append_line(app, " #  Name                         LenB     DataOfs LoopSt LoopEnd Loop Bit Vol C4Spd Flags");
  if (err != ESP_OK) return err;

  sample_ptr_offset = (uint64_t)S3M_HEADER_SIZE + order_count;
  for (uint16_t i = 0; i < sample_count; i++)
  {
    uint16_t para;
    uint64_t header_offset;
    uint64_t data_offset = 0;
    uint64_t length_bytes = 0;
    uint32_t length = 0;
    uint32_t loop_start = 0;
    uint32_t loop_end = 0;
    uint32_t c4speed = S3M_DEFAULT_C4SPEED;
    uint8_t type = CMD_APP_S3M_SAMPLE_TYPE_NONE;
    uint8_t volume = 0;
    uint8_t pack = CMD_APP_S3M_SAMPLE_PACK_NONE;
    uint8_t sample_flags = 0;
    unsigned bits = 8;
    const char *loop_text = "no";

    err = cmd_app_module_info_read_at(app, file, sample_ptr_offset + (uint64_t)i * 2, buf, 2, &got);
    if (err != ESP_OK) return err;
    if (got < 2) return ESP_FAIL;

    para = cmd_app_info_rd_le16(buf);
    if (!para)
    {
      err = cmd_app_module_info_append_line(app, "%02u  %-28s %8u %10u %6u %7u %-4s %3u %3u %5u 0x%02X",
        (unsigned)i + 1,
        "<empty>",
        0u,
        0u,
        0u,
        0u,
        "no",
        8u,
        0u,
        S3M_DEFAULT_C4SPEED,
        0u);
      if (err != ESP_OK) return err;
      continue;
    }

    header_offset = (uint64_t)para << 4;
    if (header_offset > entry->size || entry->size - header_offset < S3M_SAMPLE_HEADER_SIZE) return ESP_FAIL;

    err = cmd_app_module_info_read_at(app, file, header_offset, buf, S3M_SAMPLE_HEADER_SIZE, &got);
    if (err != ESP_OK) return err;
    if (got < S3M_SAMPLE_HEADER_SIZE) return ESP_FAIL;

    type = buf[0];
    data_offset = (((uint64_t)buf[13] << 16) | cmd_app_info_rd_le16(buf + 14)) << 4;
    length = cmd_app_info_rd_le32(buf + 16);
    loop_start = cmd_app_info_rd_le32(buf + 20);
    loop_end = cmd_app_info_rd_le32(buf + 24);
    volume = buf[28];
    pack = buf[30];
    sample_flags = buf[31];
    c4speed = cmd_app_info_rd_le32(buf + 32);
    if (!c4speed) c4speed = S3M_DEFAULT_C4SPEED;
    bits = (sample_flags & CMD_APP_S3M_SAMPLE_FLAG_16BIT) ? 16u : 8u;
    length_bytes = (uint64_t)length * (bits == 16 ? 2u : 1u);
    if ((sample_flags & CMD_APP_S3M_SAMPLE_FLAG_LOOP) && loop_end > loop_start)
    {
      loop_text = "yes";
    }

    cmd_app_module_info_copy_text(app->module_info_sample_name, sizeof(app->module_info_sample_name), buf + 48, 28);
    if (!app->module_info_sample_name[0] && type == CMD_APP_S3M_SAMPLE_TYPE_NONE)
      cmd_app_copy_string(app->module_info_sample_name, sizeof(app->module_info_sample_name), "<empty>");

    err = cmd_app_module_info_append_line(app, "%02u  %-28s %8llu %10llu %6u %7u %-4s %3u %3u %5u 0x%02X%s%s%s",
      (unsigned)i + 1,
      app->module_info_sample_name,
      (unsigned long long)length_bytes,
      (unsigned long long)data_offset,
      (unsigned)loop_start,
      (unsigned)loop_end,
      loop_text,
      bits,
      (unsigned)volume,
      (unsigned)c4speed,
      (unsigned)sample_flags,
      (type == CMD_APP_S3M_SAMPLE_TYPE_NONE || type == CMD_APP_S3M_SAMPLE_TYPE_PCM) ? "" : " type!",
      pack == CMD_APP_S3M_SAMPLE_PACK_NONE ? "" : " packed!",
      (sample_flags & CMD_APP_S3M_SAMPLE_FLAG_STEREO) ? " stereo!" : "");
    if (err != ESP_OK) return err;

  }

  return ESP_OK;
}

void cmd_app_close_module_info_viewer(CmdApp *app)
{
  if (!app) return;

  cmd_app_module_info_free_text(app);
  app->module_info_open = false;
  app->module_info_device = CMD_DEVICE_INVALID;
  app->module_info_path[0] = 0;
  app->module_info_title[0] = 0;
  cmd_app_set_status(app, "ready");
}

esp_err_t cmd_app_open_module_info_viewer(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  CmdFsFile file;
  CmdFsDriver *driver;
  bool file_open = false;
  esp_err_t err;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;

  if (!cmd_app_entry_is_module_info(entry))
  {
    cmd_app_set_status(app, "module info supports MOD/XM/S3M files only");
    return ESP_OK;
  }

  err = cmd_app_join_selected_path(panel, entry, app->module_info_path, sizeof(app->module_info_path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "module info", err);
    return ESP_OK;
  }

  err = cmd_fs_manager_mount(&app->fs, panel->device_id);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "module info", err);
    return ESP_OK;
  }

  driver = cmd_fs_manager_get_driver(&app->fs, panel->device_id);
  if (!driver || !driver->open_read || !driver->read || !driver->seek || !driver->close)
  {
    cmd_show_error(app, "module info", ESP_ERR_NOT_SUPPORTED);
    return ESP_OK;
  }

  memset(&file, 0, sizeof(file));
  err = cmd_fs_manager_open_read(&app->fs, panel->device_id, app->module_info_path, &file);
  if (err != ESP_OK)
  {
    cmd_show_error(app, "module info", err);
    return ESP_OK;
  }
  file_open = true;

  cmd_app_module_info_free_text(app);
  cmd_app_copy_string(app->module_info_title, sizeof(app->module_info_title), entry->name);

  if (cmd_app_extension_is(entry->name, ".mod"))
    err = cmd_app_module_info_build_mod(app, &file, entry);
  else if (cmd_app_extension_is(entry->name, ".s3m"))
    err = cmd_app_module_info_build_s3m(app, &file, entry);
  else
    err = cmd_app_module_info_build_xm(app, &file, entry);

  if (file_open)
  {
    esp_err_t close_err = cmd_fs_manager_close(&app->fs, &file);
    file_open = false;
    if (err == ESP_OK && close_err != ESP_OK) err = close_err;
  }

  if (err != ESP_OK)
  {
    cmd_app_module_info_free_text(app);
    cmd_show_error(app, "module info", err);
    return ESP_OK;
  }

  app->module_info_open = true;
  app->module_info_device = panel->device_id;
  app->module_info_top_line = 0;
  cmd_set_status(app, "module info: %s  lines=%u", entry->name, (unsigned)app->module_info_line_count);
  return ESP_OK;
}

size_t cmd_app_module_info_max_top_line(CmdApp *app)
{
  size_t rows;

  if (!app) return 0;

  rows = (size_t)cmd_app_viewer_visible_rows(app);
  if (app->module_info_line_count <= rows) return 0;
  return app->module_info_line_count - rows;
}

void cmd_app_module_info_set_status(CmdApp *app)
{
  if (!app) return;

  cmd_set_status(app, "module info: %s  line %u/%u",
    app->module_info_title[0] ? app->module_info_title : cmd_fs_filename_ptr(app->module_info_path),
    (unsigned)(app->module_info_top_line + 1),
    (unsigned)app->module_info_line_count);
}

void cmd_app_module_info_move_to(CmdApp *app, size_t top_line)
{
  size_t max_top;

  if (!app) return;

  max_top = cmd_app_module_info_max_top_line(app);
  if (top_line > max_top) top_line = max_top;
  app->module_info_top_line = top_line;
  cmd_app_module_info_set_status(app);
}

esp_err_t cmd_app_handle_module_info_viewer_key(CmdApp *app, const cmd_event_t *event)
{
  size_t rows;

  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->module_info_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC)
  {
    cmd_app_close_module_info_viewer(app);
    return ESP_OK;
  }

  rows = (size_t)cmd_app_viewer_visible_rows(app);

  switch (event->key)
  {
    case CMD_KEY_UP:
      if (app->module_info_top_line > 0)
        cmd_app_module_info_move_to(app, app->module_info_top_line - 1);
      return ESP_OK;

    case CMD_KEY_DOWN:
      cmd_app_module_info_move_to(app, app->module_info_top_line + 1);
      return ESP_OK;

    case CMD_KEY_PAGE_UP:
    case CMD_KEY_LEFT:
      if (app->module_info_top_line > rows)
        cmd_app_module_info_move_to(app, app->module_info_top_line - rows);
      else
        cmd_app_module_info_move_to(app, 0);
      return ESP_OK;

    case CMD_KEY_PAGE_DOWN:
    case CMD_KEY_RIGHT:
      cmd_app_module_info_move_to(app, app->module_info_top_line + rows);
      return ESP_OK;

    case CMD_KEY_HOME:
      cmd_app_module_info_move_to(app, 0);
      return ESP_OK;

    case CMD_KEY_END:
      cmd_app_module_info_move_to(app, cmd_app_module_info_max_top_line(app));
      return ESP_OK;

    default:
      return ESP_OK;
  }
}

void cmd_app_module_info_copy_line(CmdApp *app, size_t line_index, char *out, size_t out_size)
{
  const char *p;
  size_t current = 0;
  size_t len = 0;

  if (!out || out_size == 0) return;
  out[0] = 0;
  if (!app || !app->module_info_text) return;

  p = app->module_info_text;
  while (current < line_index && *p)
  {
    if (*p == '\n') current++;
    p++;
  }

  if (current != line_index) return;

  while (p[len] && p[len] != '\n') len++;
  if (len >= out_size) len = out_size - 1;
  memcpy(out, p, len);
  out[len] = 0;
}

void cmd_app_make_module_info_title(char *out, size_t out_size, CmdApp *app)
{
  if (!out || out_size == 0) return;

  cmd_app_text_clear(out, out_size);
  if (!app) return;

  cmd_app_append_text(out, out_size, "Module: ");
  cmd_app_append_text(out, out_size, cmd_device_name(app->module_info_device));
  cmd_app_append_text(out, out_size, ":");
  cmd_app_append_text(out, out_size, app->module_info_path);
  cmd_app_append_text(out, out_size, "  line ");
  snprintf(app->viewer_num, sizeof(app->viewer_num), "%u", (unsigned)(app->module_info_top_line + 1));
  cmd_app_append_text(out, out_size, app->viewer_num);
}

void cmd_app_draw_module_info_viewer(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_rect_t line_rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t bar_attr;
  int rows;
  int cols;
  int bottom_y;

  if (!app || !app->module_info_open) return;

  rows = cmd_app_viewer_visible_rows(app);
  cols = cmd_app_viewer_visible_cols(app);
  bottom_y = cmd_app_bottom_y(app);
  attr = cmd_app_normal_attr();
  bar_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_YELLOW, CMD_DISPLAY_COLOR_CYAN,
    CMD_DISPLAY_ATTR_FLAG_BOLD);

  rect.x = 0;
  rect.y = 0;
  rect.w = cmd_app_screen_w(app);
  rect.h = cmd_app_screen_h(app);

  line_rect.x = 0;
  line_rect.y = 0;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_app_make_module_info_title(app->viewer_draw_line, sizeof(app->viewer_draw_line), app);
  cmd_display_buffer_write_text(&app->buffer, 0, 0, app->viewer_draw_line, bar_attr);

  for (int row = 0; row < rows; row++)
  {
    line_rect.x = 0;
    line_rect.y = 1 + row;
    line_rect.w = cols;
    line_rect.h = 1;
    cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', attr);

    cmd_app_module_info_copy_line(app,
      app->module_info_top_line + (size_t)row,
      app->viewer_draw_line,
      sizeof(app->viewer_draw_line));
    cmd_display_buffer_write_text(&app->buffer, 0, line_rect.y, app->viewer_draw_line, attr);
  }

  line_rect.x = 0;
  line_rect.y = bottom_y;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_display_buffer_write_text(&app->buffer, 0, bottom_y,
    "Module info  Up/Down PgUp/PgDn Left/Right Home/End  Esc Close", bar_attr);
}

char cmd_app_hex_viewer_ascii(uint8_t ch)
{
  if (ch >= 32 && ch <= 126) return (char)ch;
  return '.';
}

void cmd_app_hex_viewer_clear_page(CmdApp *app)
{
  if (!app) return;

  app->hex_viewer_page_rows = 0;
  app->hex_viewer_eof = true;

  for (int i = 0; i < CMD_APP_VIEWER_MAX_ROWS; i++)
    app->hex_viewer_lines[i][0] = 0;
}

void cmd_app_hex_viewer_make_line(CmdApp *app, char *out, size_t out_size, uint64_t offset, const uint8_t *data, size_t size)
{
  char *tmp;

  if (!app || !out || out_size == 0) return;

  tmp = app->hex_viewer_tmp;

  cmd_app_text_clear(out, out_size);
  snprintf(out, out_size, "%08llX  ", (unsigned long long)offset);

  for (size_t i = 0; i < CMD_APP_HEX_VIEWER_BYTES_PER_ROW; i++)
  {
    if (i == 8) cmd_app_append_text(out, out_size, " ");

    if (i < size)
      snprintf(tmp, sizeof(app->hex_viewer_tmp), "%02X ", data[i]);
    else
      snprintf(tmp, sizeof(app->hex_viewer_tmp), "   ");

    cmd_app_append_text(out, out_size, tmp);
  }

  cmd_app_append_text(out, out_size, " |");

  for (size_t i = 0; i < CMD_APP_HEX_VIEWER_BYTES_PER_ROW; i++)
  {
    tmp[0] = i < size ? cmd_app_hex_viewer_ascii(data[i]) : ' ';
    tmp[1] = 0;
    cmd_app_append_text(out, out_size, tmp);
  }

  cmd_app_append_text(out, out_size, "|");
}

uint64_t cmd_app_hex_viewer_page_bytes(CmdApp *app)
{
  return (uint64_t)cmd_app_viewer_visible_rows(app) * CMD_APP_HEX_VIEWER_BYTES_PER_ROW;
}

uint64_t cmd_app_hex_viewer_align_offset(uint64_t offset)
{
  return (offset / CMD_APP_HEX_VIEWER_BYTES_PER_ROW) * CMD_APP_HEX_VIEWER_BYTES_PER_ROW;
}

esp_err_t cmd_app_hex_viewer_close_file(CmdApp *app)
{
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;
  if (!app->hex_viewer_file_open) return ESP_OK;

  err = cmd_fs_manager_close(&app->fs, &app->hex_viewer_file);
  app->hex_viewer_file_open = false;
  memset(&app->hex_viewer_file, 0, sizeof(app->hex_viewer_file));
  return err;
}

esp_err_t cmd_app_hex_viewer_open_file(CmdApp *app)
{
  CmdFsDriver *driver;
  esp_err_t err;

  if (!app || !app->hex_viewer_open) return ESP_ERR_INVALID_ARG;

  err = cmd_app_hex_viewer_close_file(app);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_mount(&app->fs, app->hex_viewer_device);
  if (err != ESP_OK) return err;

  driver = cmd_fs_manager_get_driver(&app->fs, app->hex_viewer_device);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->open_read || !driver->read || !driver->seek || !driver->tell || !driver->close)
    return ESP_ERR_NOT_SUPPORTED;

  memset(&app->hex_viewer_file, 0, sizeof(app->hex_viewer_file));
  err = cmd_fs_manager_open_read(&app->fs, app->hex_viewer_device,
    app->hex_viewer_path, &app->hex_viewer_file);
  if (err != ESP_OK) return err;

  app->hex_viewer_file_open = true;
  return ESP_OK;
}

esp_err_t cmd_app_hex_viewer_update_size(CmdApp *app)
{
  uint64_t size = 0;
  esp_err_t err;

  if (!app || !app->hex_viewer_file_open) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_seek(&app->fs, &app->hex_viewer_file, 0, SEEK_END);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_tell(&app->fs, &app->hex_viewer_file, &size);
  if (err != ESP_OK) return err;

  app->hex_viewer_size = size;
  app->hex_viewer_size_known = true;
  return cmd_fs_manager_seek(&app->fs, &app->hex_viewer_file, 0, SEEK_SET);
}

esp_err_t cmd_app_hex_viewer_read_page(CmdApp *app)
{
  uint8_t *row;
  size_t got = 0;
  size_t data_pos = 0;
  size_t data_len = 0;
  size_t row_len;
  size_t rows;
  uint64_t offset;
  bool hit_eof = false;
  esp_err_t err;

  if (!app || !app->hex_viewer_open) return ESP_ERR_INVALID_ARG;
  if (!app->hex_viewer_file_open) return ESP_ERR_INVALID_STATE;

  row = app->hex_viewer_row;

  cmd_app_hex_viewer_clear_page(app);

  rows = (size_t)cmd_app_viewer_visible_rows(app);
  offset = cmd_app_hex_viewer_align_offset(app->hex_viewer_offset);
  app->hex_viewer_offset = offset;

  if (offset > (uint64_t)INT64_MAX) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_seek(&app->fs, &app->hex_viewer_file, (int64_t)offset, SEEK_SET);
  if (err != ESP_OK) return err;

  while (app->hex_viewer_page_rows < rows)
  {
    row_len = 0;

    while (row_len < CMD_APP_HEX_VIEWER_BYTES_PER_ROW)
    {
      if (data_pos >= data_len)
      {
        err = cmd_fs_manager_read(&app->fs, &app->hex_viewer_file,
          app->hex_viewer_read_buffer, sizeof(app->hex_viewer_read_buffer), &got);
        if (err != ESP_OK) return err;
        if (got == 0)
        {
          hit_eof = true;
          break;
        }

        data_pos = 0;
        data_len = got;
      }

      row[row_len++] = app->hex_viewer_read_buffer[data_pos++];
      offset++;
    }

    if (row_len == 0) break;

    cmd_app_hex_viewer_make_line(
      app,
      app->hex_viewer_lines[app->hex_viewer_page_rows],
      sizeof(app->hex_viewer_lines[app->hex_viewer_page_rows]),
      offset - row_len,
      row,
      row_len);
    app->hex_viewer_page_rows++;

    if (row_len < CMD_APP_HEX_VIEWER_BYTES_PER_ROW) break;
  }

  if (app->hex_viewer_size_known && offset >= app->hex_viewer_size)
    hit_eof = true;

  app->hex_viewer_eof = hit_eof;
  return ESP_OK;
}

void cmd_app_hex_viewer_set_status(CmdApp *app)
{
  if (!app) return;

  cmd_set_status(app, "hex: %s  offset 0x%08llX",
    app->hex_viewer_title[0] ? app->hex_viewer_title : cmd_fs_filename_ptr(app->hex_viewer_path),
    (unsigned long long)app->hex_viewer_offset);
}

esp_err_t cmd_app_hex_viewer_move_to(CmdApp *app, uint64_t offset)
{
  uint64_t old_offset;
  esp_err_t err;

  if (!app || !app->hex_viewer_open) return ESP_ERR_INVALID_ARG;

  old_offset = app->hex_viewer_offset;
  app->hex_viewer_offset = cmd_app_hex_viewer_align_offset(offset);

  err = cmd_app_hex_viewer_read_page(app);
  if (err != ESP_OK)
  {
    app->hex_viewer_offset = old_offset;
    cmd_app_hex_viewer_read_page(app);
    cmd_show_error(app, "hex view", err);
    return ESP_OK;
  }

  if (app->hex_viewer_offset > 0 && app->hex_viewer_page_rows == 0)
  {
    app->hex_viewer_offset = old_offset;
    cmd_app_hex_viewer_read_page(app);
    return ESP_OK;
  }

  cmd_app_hex_viewer_set_status(app);
  return ESP_OK;
}

void cmd_app_close_hex_viewer(CmdApp *app)
{
  if (!app) return;

  cmd_app_hex_viewer_close_file(app);
  app->hex_viewer_open = false;
  app->hex_viewer_device = CMD_DEVICE_INVALID;
  app->hex_viewer_path[0] = 0;
  app->hex_viewer_title[0] = 0;
  app->hex_viewer_offset = 0;
  app->hex_viewer_size = 0;
  app->hex_viewer_size_known = false;
  cmd_app_hex_viewer_clear_page(app);
  cmd_app_set_status(app, "ready");
}

esp_err_t cmd_app_open_hex_viewer(CmdApp *app, CmdPanel *panel, const cmd_file_entry_t *entry)
{
  esp_err_t err;

  if (!app || !panel || !entry) return ESP_ERR_INVALID_ARG;

  if (entry->type != CMD_ENTRY_FILE)
  {
    cmd_app_set_status(app, "hex view supports files only");
    return ESP_OK;
  }

  err = cmd_app_join_selected_path(panel, entry, app->hex_viewer_path, sizeof(app->hex_viewer_path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "hex view", err);
    return ESP_OK;
  }

  app->hex_viewer_open = true;
  app->hex_viewer_device = panel->device_id;
  app->hex_viewer_offset = 0;
  app->hex_viewer_size = entry->size;
  app->hex_viewer_size_known = true;
  cmd_app_copy_string(app->hex_viewer_title, sizeof(app->hex_viewer_title), entry->name);

  err = cmd_app_hex_viewer_open_file(app);
  if (err == ESP_OK)
  {
    esp_err_t size_err = cmd_app_hex_viewer_update_size(app);
    if (size_err != ESP_OK)
    {
      app->hex_viewer_size = entry->size;
      app->hex_viewer_size_known = true;
    }
  }
  if (err == ESP_OK) err = cmd_app_hex_viewer_read_page(app);
  if (err != ESP_OK)
  {
    cmd_app_close_hex_viewer(app);
    cmd_show_error(app, "hex view", err);
    return ESP_OK;
  }

  cmd_app_hex_viewer_set_status(app);
  return ESP_OK;
}

esp_err_t cmd_app_handle_hex_viewer_key(CmdApp *app, const cmd_event_t *event)
{
  uint64_t page_bytes;
  uint64_t offset;

  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->hex_viewer_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC)
  {
    cmd_app_close_hex_viewer(app);
    return ESP_OK;
  }

  page_bytes = cmd_app_hex_viewer_page_bytes(app);

  switch (event->key)
  {
    case CMD_KEY_UP:
      if (app->hex_viewer_offset >= CMD_APP_HEX_VIEWER_BYTES_PER_ROW)
        return cmd_app_hex_viewer_move_to(app, app->hex_viewer_offset - CMD_APP_HEX_VIEWER_BYTES_PER_ROW);
      return cmd_app_hex_viewer_move_to(app, 0);

    case CMD_KEY_DOWN:
      if (!app->hex_viewer_eof || app->hex_viewer_page_rows > 1)
        return cmd_app_hex_viewer_move_to(app, app->hex_viewer_offset + CMD_APP_HEX_VIEWER_BYTES_PER_ROW);
      return ESP_OK;

    case CMD_KEY_PAGE_UP:
    case CMD_KEY_LEFT:
      if (app->hex_viewer_offset > page_bytes)
        return cmd_app_hex_viewer_move_to(app, app->hex_viewer_offset - page_bytes);
      return cmd_app_hex_viewer_move_to(app, 0);

    case CMD_KEY_PAGE_DOWN:
    case CMD_KEY_RIGHT:
      if (app->hex_viewer_eof)
      {
        if (app->hex_viewer_page_rows > 1)
          return cmd_app_hex_viewer_move_to(
            app,
            app->hex_viewer_offset + (app->hex_viewer_page_rows - 1) * CMD_APP_HEX_VIEWER_BYTES_PER_ROW);
        return ESP_OK;
      }
      return cmd_app_hex_viewer_move_to(app, app->hex_viewer_offset + page_bytes);

    case CMD_KEY_HOME:
      return cmd_app_hex_viewer_move_to(app, 0);

    case CMD_KEY_END:
      if (!app->hex_viewer_size_known)
      {
        cmd_app_set_status(app, "hex view: file size unknown");
        return ESP_OK;
      }

      if (app->hex_viewer_size <= page_bytes)
        return cmd_app_hex_viewer_move_to(app, 0);

      offset = app->hex_viewer_size - page_bytes;
      offset = cmd_app_hex_viewer_align_offset(offset);
      return cmd_app_hex_viewer_move_to(app, offset);

    default:
      return ESP_OK;
  }
}

void cmd_app_make_hex_viewer_title(char *out, size_t out_size, CmdApp *app)
{
  if (!out || out_size == 0) return;

  cmd_app_text_clear(out, out_size);
  if (!app) return;

  cmd_app_append_text(out, out_size, "Hex: ");
  cmd_app_append_text(out, out_size, cmd_device_name(app->hex_viewer_device));
  cmd_app_append_text(out, out_size, ":");
  cmd_app_append_text(out, out_size, app->hex_viewer_path);
  cmd_app_append_text(out, out_size, "  offset 0x");
  snprintf(app->viewer_num, sizeof(app->viewer_num), "%08llX", (unsigned long long)app->hex_viewer_offset);
  cmd_app_append_text(out, out_size, app->viewer_num);
}

void cmd_app_draw_hex_viewer(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_rect_t line_rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t bar_attr;
  int rows;
  int cols;
  int bottom_y;

  if (!app || !app->hex_viewer_open) return;

  rows = cmd_app_viewer_visible_rows(app);
  cols = cmd_app_viewer_visible_cols(app);
  bottom_y = cmd_app_bottom_y(app);
  attr = cmd_app_normal_attr();
  bar_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_YELLOW, CMD_DISPLAY_COLOR_CYAN,
    CMD_DISPLAY_ATTR_FLAG_BOLD);

  rect.x = 0;
  rect.y = 0;
  rect.w = cmd_app_screen_w(app);
  rect.h = cmd_app_screen_h(app);

  line_rect.x = 0;
  line_rect.y = 0;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_app_make_hex_viewer_title(app->viewer_draw_line, sizeof(app->viewer_draw_line), app);
  cmd_display_buffer_write_text(&app->buffer, 0, 0, app->viewer_draw_line, bar_attr);

  for (int row_index = 0; row_index < rows; row_index++)
  {
    line_rect.x = 0;
    line_rect.y = 1 + row_index;
    line_rect.w = cols;
    line_rect.h = 1;
    cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', attr);

    if ((size_t)row_index < app->hex_viewer_page_rows)
      cmd_display_buffer_write_text(&app->buffer, 0, line_rect.y, app->hex_viewer_lines[row_index], attr);
  }

  line_rect.x = 0;
  line_rect.y = bottom_y;
  line_rect.w = rect.w;
  line_rect.h = 1;
  cmd_display_buffer_fill_rect(&app->buffer, &line_rect, ' ', bar_attr);
  cmd_display_buffer_write_text(&app->buffer, 0, bottom_y,
    "Hex viewer  Up/Down PgUp/PgDn Left/Right Home End  Esc Close", bar_attr);
}

esp_err_t cmd_app_reload_both_panels(CmdApp *app)
{
  esp_err_t left_err;
  esp_err_t right_err;

  if (!app) return ESP_ERR_INVALID_ARG;

  left_err = cmd_panel_reload(&app->left_panel);
  right_err = cmd_panel_reload(&app->right_panel);

  if (left_err != ESP_OK) return left_err;
  return right_err;
}

void cmd_app_set_fileop_result_status(CmdApp *app, cmd_app_fileop_t op, const char *name, esp_err_t err)
{
  if (!app) return;

  if (err == ESP_OK)
  {
    cmd_set_status(app, "%s: %s", cmd_app_fileop_past(op), name ? name : "");
    return;
  }

  if (err == CMD_ERR_CANCELLED)
  {
    cmd_set_status(app, "%s cancelled", cmd_app_fileop_status_name(op));
    return;
  }

  cmd_show_error(app, cmd_app_fileop_status_name(op), err);
}

esp_err_t cmd_app_open_fileop_confirm(CmdApp *app, cmd_app_fileop_t op)
{
  CmdPanel *active;
  CmdPanel *passive;
  const cmd_file_entry_t *entry;
  bool group;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  active = cmd_app_active_panel(app);
  passive = cmd_app_passive_panel(app);
  if (!active || !passive) return ESP_ERR_INVALID_STATE;

  cmd_panel_update_selection_stats(active);
  group = cmd_panel_has_selection(active);

  app->fileop_group = group;
  app->fileop_item_count = group ? active->selected_count : 1;
  app->fileop_item_size = group ? active->selected_size : 0;
  app->fileop_dst_path[0] = 0;

  if (group)
  {
    snprintf(app->fileop_name, sizeof(app->fileop_name), "%u selected items", (unsigned)active->selected_count);

    if (op == CMD_APP_FILEOP_COPY || op == CMD_APP_FILEOP_MOVE)
      cmd_app_copy_string(app->fileop_dst_path, sizeof(app->fileop_dst_path), passive->current_path);

    app->fileop_confirm_open = true;
    app->fileop_confirm_op = op;
    cmd_app_set_status(app, "Enter OK   Esc cancel");
    return ESP_OK;
  }

  entry = cmd_panel_get_selected_entry(active);
  if (!entry)
  {
    cmd_app_set_status(app, "no selected entry");
    return ESP_OK;
  }

  if (entry->type == CMD_ENTRY_PARENT)
  {
    cmd_app_set_status(app, "cannot operate on ..");
    return ESP_OK;
  }

  cmd_app_copy_string(app->fileop_name, sizeof(app->fileop_name), entry->name);
  app->fileop_item_size = (entry->type == CMD_ENTRY_FILE) ? entry->size : 0;

  if (op == CMD_APP_FILEOP_COPY || op == CMD_APP_FILEOP_MOVE)
  {
    err = cmd_fs_join_path(passive->current_path, entry->name, app->fileop_dst_path, sizeof(app->fileop_dst_path));
    if (err != ESP_OK)
    {
      cmd_app_set_status_err(app, cmd_app_fileop_status_name(op), err);
      return ESP_OK;
    }
  }
  else
  {
    err = cmd_app_join_selected_path(active, entry, app->fileop_dst_path, sizeof(app->fileop_dst_path));
    if (err != ESP_OK)
    {
      cmd_app_set_status_err(app, cmd_app_fileop_status_name(op), err);
      return ESP_OK;
    }
  }

  app->fileop_confirm_open = true;
  app->fileop_confirm_op = op;
  cmd_app_set_status(app, "Enter OK   Esc cancel");
  return ESP_OK;
}

void cmd_app_close_fileop_confirm(CmdApp *app)
{
  if (!app) return;

  app->fileop_confirm_open = false;
  app->fileop_confirm_op = CMD_APP_FILEOP_NONE;
  app->fileop_name[0] = 0;
  app->fileop_dst_path[0] = 0;
  app->fileop_group = false;
  app->fileop_item_count = 0;
  app->fileop_item_size = 0;
}

void cmd_app_draw_confirm_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  char title[CMD_DISPLAY_COLS + 1];
  char line[CMD_DISPLAY_COLS + 1];

  if (!app || !app->fileop_confirm_open) return;

  rect.x = cmd_app_center_x(app, CMD_APP_FILEOP_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_FILEOP_DIALOG_H);
  rect.w = CMD_APP_FILEOP_DIALOG_W;
  rect.h = CMD_APP_FILEOP_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_app_dialog_frame_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);

  snprintf(title, sizeof(title), " %s ", cmd_app_fileop_verb(app->fileop_confirm_op));
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, title, frame_attr);

  cmd_app_text_clear(line, sizeof(line));
  if (app->fileop_group)
  {
    cmd_app_append_text(line, sizeof(line), cmd_app_fileop_verb(app->fileop_confirm_op));
    cmd_app_append_text(line, sizeof(line), " ");
    snprintf(line + cmd_app_text_len(line), sizeof(line) - cmd_app_text_len(line),
      "%u selected items", (unsigned)app->fileop_item_count);
    if (app->fileop_confirm_op == CMD_APP_FILEOP_DELETE)
      cmd_app_append_text(line, sizeof(line), "?");
  }
  else if (app->fileop_confirm_op == CMD_APP_FILEOP_DELETE)
  {
    cmd_app_append_text(line, sizeof(line), "Delete ");
    cmd_app_append_text(line, sizeof(line), app->fileop_name);
    cmd_app_append_text(line, sizeof(line), "?");
  }
  else
  {
    cmd_app_append_text(line, sizeof(line), cmd_app_fileop_verb(app->fileop_confirm_op));
    cmd_app_append_text(line, sizeof(line), " ");
    cmd_app_append_text(line, sizeof(line), app->fileop_name);
  }

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 2, line, attr);

  if (app->fileop_confirm_op == CMD_APP_FILEOP_COPY || app->fileop_confirm_op == CMD_APP_FILEOP_MOVE)
  {
    cmd_app_text_clear(line, sizeof(line));
    cmd_app_append_text(line, sizeof(line), "to ");
    cmd_app_append_text(line, sizeof(line), app->fileop_dst_path);
    cmd_app_append_text(line, sizeof(line), "?");
    cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 3, line, attr);
  }

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2,
    "Enter/OK start   Esc/Cancel", attr);
}

void cmd_app_draw_progress_bar(CmdApp *app, int x, int y, int w)
{
  uint64_t percent = 0;
  int fill;

  if (!app || w <= 2) return;

  if (app->fileop_bytes_total > 0)
    percent = (app->fileop_bytes_done * 100ull) / app->fileop_bytes_total;
  else if (app->fileop_files_total > 0)
    percent = ((uint64_t)app->fileop_files_done * 100ull) / (uint64_t)app->fileop_files_total;
  else
    percent = 100;

  if (percent > 100) percent = 100;

  cmd_display_buffer_put_char(&app->buffer, x, y, '[', cmd_app_dialog_frame_attr());
  cmd_display_buffer_put_char(&app->buffer, x + w - 1, y, ']', cmd_app_dialog_frame_attr());

  fill = (int)((percent * (uint64_t)(w - 2)) / 100ull);
  for (int i = 0; i < w - 2; i++)
  {
    char ch = i < fill ? '#' : ' ';
    cmd_display_buffer_put_char(&app->buffer, x + 1 + i, y, ch, cmd_app_dialog_selected_attr());
  }
}

void cmd_app_draw_progress_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  char title[CMD_DISPLAY_COLS + 1];
  char line[CMD_DISPLAY_COLS + 1];
  uint64_t percent = 0;

  if (!app || !app->fileop_progress_open) return;

  rect.x = cmd_app_center_x(app, CMD_APP_FILEOP_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_FILEOP_DIALOG_H);
  rect.w = CMD_APP_FILEOP_DIALOG_W;
  rect.h = CMD_APP_FILEOP_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_app_dialog_frame_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);

  snprintf(title, sizeof(title), " %s progress ", cmd_app_fileop_verb(app->fileop_progress_op));
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, title, frame_attr);

  cmd_app_text_clear(line, sizeof(line));
  cmd_app_append_text(line, sizeof(line), cmd_app_fileop_verb(app->fileop_progress_op));
  cmd_app_append_text(line, sizeof(line), ": ");
  cmd_app_append_text(line, sizeof(line), app->fileop_progress_name);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 2, line, attr);

  cmd_app_draw_progress_bar(app, rect.x + 2, rect.y + 4, rect.w - 4);

  if (app->fileop_bytes_total > 0)
    percent = (app->fileop_bytes_done * 100ull) / app->fileop_bytes_total;
  else if (app->fileop_files_total > 0)
    percent = ((uint64_t)app->fileop_files_done * 100ull) / (uint64_t)app->fileop_files_total;
  else
    percent = 100;

  if (percent > 100) percent = 100;

  if (app->fileop_files_total > 0)
  {
    snprintf(line, sizeof(line), "%u / %u items  %llu / %llu bytes  %llu%%",
      (unsigned)app->fileop_files_done,
      (unsigned)app->fileop_files_total,
      (unsigned long long)app->fileop_bytes_done,
      (unsigned long long)app->fileop_bytes_total,
      (unsigned long long)percent);
  }
  else
  {
    snprintf(line, sizeof(line), "%llu / %llu bytes  %llu%%",
      (unsigned long long)app->fileop_bytes_done,
      (unsigned long long)app->fileop_bytes_total,
      (unsigned long long)percent);
  }
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 5, line, attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2,
    app->fileop_progress_can_cancel ? "Esc cancel" : "Finishing...", attr);
}

void cmd_app_close_error_dialog(CmdApp *app)
{
  if (!app) return;

  app->error_dialog_open = false;
  app->error_dialog_err = ESP_OK;
  app->error_dialog_title[0] = 0;
  app->error_dialog_message[0] = 0;
}

void cmd_app_draw_error_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  char title[CMD_APP_ERROR_TITLE_MAX + 4];
  char line[CMD_DISPLAY_COLS + 1];

  if (!app || !app->error_dialog_open) return;

  rect.x = cmd_app_center_x(app, CMD_APP_ERROR_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_ERROR_DIALOG_H);
  rect.w = CMD_APP_ERROR_DIALOG_W;
  rect.h = CMD_APP_ERROR_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_RED, CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_ATTR_FLAG_BOLD);

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);

  cmd_app_text_clear(title, sizeof(title));
  cmd_app_append_text(title, sizeof(title), " ");
  cmd_app_append_text(title, sizeof(title), app->error_dialog_title[0] ? app->error_dialog_title : "Error");
  cmd_app_append_text(title, sizeof(title), " ");
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, title, frame_attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 2, app->error_dialog_message, attr);

  cmd_app_text_clear(line, sizeof(line));
  cmd_app_append_text(line, sizeof(line), "Code: 0x");
  snprintf(line + cmd_app_text_len(line), sizeof(line) - cmd_app_text_len(line), "%x", (unsigned)app->error_dialog_err);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 3, line, attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2,
    "Enter/Esc close", attr);
}

esp_err_t cmd_app_handle_error_dialog_key(CmdApp *app, const cmd_event_t *event)
{
  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->error_dialog_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC || event->key == CMD_KEY_ENTER || event->key == CMD_KEY_F10)
  {
    cmd_app_close_error_dialog(app);
    return ESP_OK;
  }

  return ESP_OK;
}


bool cmd_app_mkdir_name_is_valid(const char *name)
{
  if (!name || !name[0]) return false;

  for (size_t i = 0; name[i]; i++)
  {
    if (name[i] == '/' || name[i] == '\\') return false;
  }

  return true;
}

bool cmd_app_panel_is_at_path(const CmdPanel *panel, cmd_device_id_t device_id, const char *path)
{
  if (!panel || !path) return false;
  if (panel->device_id != device_id) return false;
  return strcmp(panel->current_path, path) == 0;
}

esp_err_t cmd_app_reload_matching_panel(CmdApp *app, CmdPanel *panel, cmd_device_id_t device_id, const char *path, const char *name)
{
  if (!cmd_app_panel_is_at_path(panel, device_id, path)) return ESP_OK;
  return cmd_app_reload_panel(app, panel, name);
}

bool cmd_app_focus_panel_entry(CmdPanel *panel, const char *name)
{
  if (!panel || !panel->entries || !name) return false;

  for (size_t i = 0; i < panel->count; i++)
  {
    if (strcmp(panel->entries[i].name, name) != 0) continue;

    panel->cursor = i;
    cmd_panel_ensure_cursor_visible(panel);
    return true;
  }

  return false;
}

bool cmd_app_path_is_same_or_child(const char *parent, const char *path)
{
  const char *base = parent;
  const char *child = path;
  size_t base_len;

  if (!base || !base[0]) base = "/";
  if (!child || !child[0]) child = "/";

  if (strcmp(base, child) == 0) return true;
  if (cmd_fs_path_is_root(base)) return true;

  base_len = strlen(base);
  while (base_len > 1 && base[base_len - 1] == '/')
    base_len--;

  if (strncmp(base, child, base_len) != 0) return false;
  return child[base_len] == '/';
}

esp_err_t cmd_app_recover_panel_from_removed_path(CmdPanel *panel, cmd_device_id_t device_id, const char *removed_path)
{
  char parent[CMD_PATH_MAX];
  esp_err_t err;

  if (!panel || !removed_path) return ESP_ERR_INVALID_ARG;
  if (panel->device_id != device_id) return ESP_OK;
  if (!cmd_app_path_is_same_or_child(removed_path, panel->current_path)) return ESP_OK;

  err = cmd_fs_parent_path(removed_path, parent, sizeof(parent));
  if (err != ESP_OK) return err;

  return cmd_panel_set_path(panel, parent);
}

esp_err_t cmd_app_recover_panels_from_removed_path(CmdApp *app, cmd_device_id_t device_id, const char *removed_path)
{
  esp_err_t err;

  if (!app || !removed_path) return ESP_ERR_INVALID_ARG;

  err = cmd_app_recover_panel_from_removed_path(&app->left_panel, device_id, removed_path);
  if (err != ESP_OK) return err;

  return cmd_app_recover_panel_from_removed_path(&app->right_panel, device_id, removed_path);
}

esp_err_t cmd_app_recover_panels_after_source_removal(CmdApp *app, CmdPanel *active)
{
  const cmd_file_entry_t *entry;
  char path[CMD_PATH_MAX];
  esp_err_t err;

  if (!app || !active) return ESP_ERR_INVALID_ARG;

  if (app->fileop_group)
  {
    for (size_t i = 0; i < active->count; i++)
    {
      entry = &active->entries[i];
      if (!(entry->flags & CMD_ENTRY_FLAG_SELECTED)) continue;
      if (entry->type != CMD_ENTRY_FILE && entry->type != CMD_ENTRY_DIR) continue;

      err = cmd_fs_join_path(active->current_path, entry->name, path, sizeof(path));
      if (err != ESP_OK) return err;

      err = cmd_app_recover_panels_from_removed_path(app, active->device_id, path);
      if (err != ESP_OK) return err;
    }

    return ESP_OK;
  }

  entry = cmd_panel_get_selected_entry(active);
  if (!entry) return ESP_OK;
  if (entry->type != CMD_ENTRY_FILE && entry->type != CMD_ENTRY_DIR) return ESP_OK;

  err = cmd_fs_join_path(active->current_path, entry->name, path, sizeof(path));
  if (err != ESP_OK) return err;

  return cmd_app_recover_panels_from_removed_path(app, active->device_id, path);
}

esp_err_t cmd_app_open_mkdir_dialog(CmdApp *app)
{
  if (!app) return ESP_ERR_INVALID_ARG;

  app->mkdir_dialog_open = true;
  app->mkdir_dialog_op = CMD_APP_NAME_DIALOG_MKDIR;
  app->mkdir_name[0] = 0;
  app->rename_src_path[0] = 0;
  app->rename_dst_path[0] = 0;
  app->rename_remap_path[0] = 0;
  cmd_app_set_status(app, "mkdir: type name, Enter OK, Esc cancel");
  return ESP_OK;
}

esp_err_t cmd_app_open_rename_dialog(CmdApp *app)
{
  CmdPanel *panel;
  const cmd_file_entry_t *entry;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  entry = cmd_panel_get_selected_entry(panel);
  if (!entry)
  {
    cmd_app_set_status(app, "no selected entry");
    return ESP_OK;
  }

  if (entry->type == CMD_ENTRY_PARENT)
  {
    cmd_app_set_status(app, "cannot rename ..");
    return ESP_OK;
  }

  if (entry->type != CMD_ENTRY_FILE && entry->type != CMD_ENTRY_DIR)
  {
    cmd_app_set_status(app, "cannot rename entry");
    return ESP_OK;
  }

  err = cmd_fs_join_path(panel->current_path, entry->name, app->rename_src_path, sizeof(app->rename_src_path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "rename", err);
    return ESP_OK;
  }

  cmd_app_copy_string(app->mkdir_name, sizeof(app->mkdir_name), entry->name);
  app->mkdir_dialog_open = true;
  app->mkdir_dialog_op = CMD_APP_NAME_DIALOG_RENAME;
  cmd_app_set_status(app, "rename: edit name, Enter OK, Esc cancel");
  return ESP_OK;
}

void cmd_app_close_mkdir_dialog(CmdApp *app)
{
  if (!app) return;

  app->mkdir_dialog_open = false;
  app->mkdir_dialog_op = CMD_APP_NAME_DIALOG_MKDIR;
  app->mkdir_name[0] = 0;
  app->rename_src_path[0] = 0;
  app->rename_dst_path[0] = 0;
  app->rename_remap_path[0] = 0;
}

esp_err_t cmd_app_create_directory(CmdApp *app)
{
  CmdPanel *panel;
  char path[CMD_PATH_MAX];
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  if (!cmd_app_mkdir_name_is_valid(app->mkdir_name))
  {
    cmd_app_set_status(app, "invalid folder name");
    return ESP_OK;
  }

  err = cmd_fs_join_path(panel->current_path, app->mkdir_name, path, sizeof(path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "mkdir", err);
    return ESP_OK;
  }

  err = cmd_fs_manager_mount(panel->fs, panel->device_id);
  if (err == ESP_OK)
    err = cmd_fs_manager_mkdir(panel->fs, panel->device_id, path);

  if (err != ESP_OK)
  {
    cmd_show_error(app, "mkdir", err);
    return ESP_OK;
  }

  cmd_app_close_mkdir_dialog(app);

  err = cmd_app_reload_panel(app, panel, "mkdir reload");
  if (err == ESP_OK)
  {
    cmd_app_focus_panel_entry(panel, cmd_fs_filename_ptr(path));
    err = cmd_app_reload_matching_panel(app, cmd_app_passive_panel(app), panel->device_id, panel->current_path, "mkdir reload");
  }

  if (err != ESP_OK)
  {
    cmd_show_error(app, "mkdir", err);
    return ESP_OK;
  }

  cmd_set_status(app, "folder created: %s", cmd_fs_filename_ptr(path));
  return ESP_OK;
}

esp_err_t cmd_app_remap_panel_after_rename(CmdPanel *panel,
  cmd_device_id_t device_id,
  const char *old_path,
  const char *new_path,
  char *work_path,
  size_t work_path_size)
{
  size_t old_len;
  int len;

  if (!panel || !old_path || !new_path || !work_path || work_path_size == 0) return ESP_ERR_INVALID_ARG;
  if (panel->device_id != device_id) return ESP_OK;
  if (!cmd_app_path_is_same_or_child(old_path, panel->current_path)) return ESP_OK;

  if (strcmp(old_path, panel->current_path) == 0)
    return cmd_panel_set_path(panel, new_path);

  old_len = strlen(old_path);
  len = snprintf(work_path, work_path_size, "%s%s", new_path, panel->current_path + old_len);
  if (len < 0 || (size_t)len >= work_path_size) return CMD_ERR_PATH_TOO_LONG;

  return cmd_panel_set_path(panel, work_path);
}

esp_err_t cmd_app_rename_entry(CmdApp *app)
{
  CmdPanel *panel;
  cmd_file_entry_t entry;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  if (!cmd_app_mkdir_name_is_valid(app->mkdir_name))
  {
    cmd_app_set_status(app, "invalid file name");
    return ESP_OK;
  }

  err = cmd_fs_join_path(panel->current_path, app->mkdir_name, app->rename_dst_path, sizeof(app->rename_dst_path));
  if (err != ESP_OK)
  {
    cmd_show_error(app, "rename", err);
    return ESP_OK;
  }

  if (strcmp(app->rename_src_path, app->rename_dst_path) == 0)
  {
    cmd_app_close_mkdir_dialog(app);
    cmd_app_set_status(app, "rename unchanged");
    return ESP_OK;
  }

  err = cmd_fs_manager_mount(panel->fs, panel->device_id);
  if (err == ESP_OK)
  {
    err = cmd_fs_manager_stat(panel->fs, panel->device_id, app->rename_dst_path, &entry);
    if (err == ESP_OK)
      err = ESP_ERR_INVALID_STATE;
    else if (err == ESP_ERR_NOT_FOUND)
      err = cmd_fs_manager_rename(panel->fs, panel->device_id, app->rename_src_path, app->rename_dst_path);
  }

  if (err != ESP_OK)
  {
    cmd_show_error(app, "rename", err);
    return ESP_OK;
  }

  err = cmd_app_remap_panel_after_rename(&app->left_panel,
    panel->device_id,
    app->rename_src_path,
    app->rename_dst_path,
    app->rename_remap_path,
    sizeof(app->rename_remap_path));
  if (err == ESP_OK)
    err = cmd_app_remap_panel_after_rename(&app->right_panel,
      panel->device_id,
      app->rename_src_path,
      app->rename_dst_path,
      app->rename_remap_path,
      sizeof(app->rename_remap_path));

  if (err == ESP_OK)
    err = cmd_app_reload_both_panels(app);
  if (err == ESP_OK)
    cmd_app_focus_panel_entry(panel, cmd_fs_filename_ptr(app->rename_dst_path));

  if (err != ESP_OK)
  {
    cmd_app_close_mkdir_dialog(app);
    cmd_show_error(app, "rename", err);
    return ESP_OK;
  }

  cmd_set_status(app, "renamed to: %s", cmd_fs_filename_ptr(app->rename_dst_path));
  cmd_app_close_mkdir_dialog(app);
  return ESP_OK;
}

void cmd_app_draw_mkdir_dialog(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  cmd_display_attr_t frame_attr;
  char line[CMD_DISPLAY_COLS + 1];
  const char *title;
  const char *label;
  const char *footer;

  if (!app || !app->mkdir_dialog_open) return;

  if (app->mkdir_dialog_op == CMD_APP_NAME_DIALOG_RENAME)
  {
    title = " Rename ";
    label = "New name:";
    footer = "Enter/OK rename   Esc/Cancel";
  }
  else
  {
    title = " MkDir ";
    label = "Folder name:";
    footer = "Enter/OK create   Esc/Cancel";
  }

  rect.x = cmd_app_center_x(app, CMD_APP_MKDIR_DIALOG_W);
  rect.y = cmd_app_center_y(app, CMD_APP_MKDIR_DIALOG_H);
  rect.w = CMD_APP_MKDIR_DIALOG_W;
  rect.h = CMD_APP_MKDIR_DIALOG_H;

  attr = cmd_app_dialog_attr();
  frame_attr = cmd_app_dialog_frame_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_draw_box(&app->buffer, &rect, frame_attr);
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y, title, frame_attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 2, label, attr);

  cmd_app_text_clear(line, sizeof(line));
  cmd_app_append_text(line, sizeof(line), app->mkdir_name);
  cmd_app_append_text(line, sizeof(line), "_");
  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + 3, line, attr);

  cmd_display_buffer_write_text(&app->buffer, rect.x + 2, rect.y + rect.h - 2, footer, attr);
}

esp_err_t cmd_app_submit_mkdir_dialog(CmdApp *app)
{
  if (!app) return ESP_ERR_INVALID_ARG;

  if (app->mkdir_dialog_op == CMD_APP_NAME_DIALOG_RENAME)
    return cmd_app_rename_entry(app);

  return cmd_app_create_directory(app);
}

esp_err_t cmd_app_handle_mkdir_dialog_key(CmdApp *app, const cmd_event_t *event)
{
  size_t len;

  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->mkdir_dialog_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY && event->type != CMD_EVENT_QUIT) return ESP_OK;

  if (event->type == CMD_EVENT_QUIT || event->key == CMD_KEY_ESC || event->key == CMD_KEY_F10)
  {
    if (app->mkdir_dialog_op == CMD_APP_NAME_DIALOG_RENAME)
      cmd_app_set_status(app, "rename cancelled");
    else
      cmd_app_set_status(app, "mkdir cancelled");
    cmd_app_close_mkdir_dialog(app);
    return ESP_OK;
  }

  if (event->key == CMD_KEY_ENTER)
    return cmd_app_submit_mkdir_dialog(app);

  if (event->key == CMD_KEY_BACKSPACE)
  {
    len = cmd_app_text_len(app->mkdir_name);
    if (len > 0) app->mkdir_name[len - 1] = 0;
    return ESP_OK;
  }

  if (event->key == CMD_KEY_CHAR && event->ch >= 32 && event->ch < 127)
  {
    len = cmd_app_text_len(app->mkdir_name);
    if (len + 1 >= sizeof(app->mkdir_name)) return ESP_OK;
    app->mkdir_name[len] = (char)event->ch;
    app->mkdir_name[len + 1] = 0;
    return ESP_OK;
  }

  return ESP_OK;
}

bool cmd_app_progress_poll_cancel(CmdApp *app, bool can_cancel)
{
  cmd_event_t event;
  esp_err_t err;

  if (!app || !can_cancel) return false;

  err = cmd_input_poll_event(app->input, &event, 0);
  if (err != ESP_OK) return false;
  if (event.type != CMD_EVENT_KEY && event.type != CMD_EVENT_QUIT) return false;

  if (event.type == CMD_EVENT_QUIT || event.key == CMD_KEY_ESC)
  {
    app->fileop_cancel_requested = true;
    cmd_app_set_status(app, "cancel requested");
    return true;
  }

  return false;
}

bool cmd_app_fileops_progress_cb(const CmdFileopsProgress *progress, void *ctx)
{
  CmdApp *app = (CmdApp *)ctx;

  if (!app || !progress) return true;

  app->fileop_bytes_done = progress->bytes_done;
  app->fileop_bytes_total = progress->bytes_total;
  app->fileop_files_done = progress->files_done;
  app->fileop_files_total = progress->files_total;
  app->fileop_progress_can_cancel = progress->can_cancel;
  cmd_app_copy_string(app->fileop_progress_name, sizeof(app->fileop_progress_name), progress->filename);

  cmd_app_render(app);

  if (cmd_app_progress_poll_cancel(app, progress->can_cancel)) return false;
  if (app->fileop_cancel_requested && progress->can_cancel) return false;
  return true;
}

esp_err_t cmd_app_run_fileop(CmdApp *app, cmd_app_fileop_t op)
{
  CmdFileopsOptions options = {};
  CmdPanel *active;
  CmdPanel *passive;
  esp_err_t op_err;
  esp_err_t reload_err;

  if (!app) return ESP_ERR_INVALID_ARG;

  active = cmd_app_active_panel(app);
  passive = cmd_app_passive_panel(app);
  if (!active || !passive) return ESP_ERR_INVALID_STATE;

  app->fileop_progress_open = true;
  app->fileop_progress_op = op;
  app->fileop_bytes_done = 0;
  app->fileop_bytes_total = app->fileop_group ? app->fileop_item_size : 0;
  app->fileop_files_done = 0;
  app->fileop_files_total = app->fileop_group ? app->fileop_item_count : 0;
  app->fileop_progress_can_cancel = true;
  app->fileop_cancel_requested = false;
  cmd_app_copy_string(app->fileop_progress_name, sizeof(app->fileop_progress_name), app->fileop_name);

  options.flags = CMD_FILEOPS_FLAG_RECURSIVE;
  options.progress = cmd_app_fileops_progress_cb;
  options.progress_ctx = app;

  cmd_app_set_status(app, "operation started");
  cmd_app_render(app);

  if (op == CMD_APP_FILEOP_DELETE)
    op_err = cmd_app_recover_panels_after_source_removal(app, active);
  else
    op_err = ESP_OK;

  if (op_err == ESP_OK && op == CMD_APP_FILEOP_COPY)
    op_err = cmd_fileops_copy_selected(active, passive, &options);
  else if (op_err == ESP_OK && op == CMD_APP_FILEOP_MOVE)
    op_err = cmd_fileops_move_selected(active, passive, &options);
  else if (op_err == ESP_OK && op == CMD_APP_FILEOP_DELETE)
    op_err = cmd_fileops_delete_selected(active, &options);
  else if (op_err == ESP_OK)
    op_err = ESP_ERR_INVALID_ARG;

  app->fileop_progress_open = false;

  if (op_err == ESP_OK)
    cmd_panel_clear_selection(active);

  reload_err = cmd_app_reload_both_panels(app);

  if (op_err == ESP_OK && reload_err != ESP_OK)
    op_err = reload_err;

  cmd_app_set_fileop_result_status(app, op, app->fileop_name, op_err);
  return ESP_OK;
}

esp_err_t cmd_app_handle_fileop_confirm_key(CmdApp *app, const cmd_event_t *event)
{
  cmd_app_fileop_t op;

  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (!app->fileop_confirm_open) return ESP_ERR_INVALID_STATE;
  if (event->type != CMD_EVENT_KEY) return ESP_OK;

  if (event->key == CMD_KEY_ESC || event->key == CMD_KEY_F10 ||
    (event->key == CMD_KEY_CHAR && (event->ch == 'q' || event->ch == 'Q')))
  {
    cmd_app_close_fileop_confirm(app);
    cmd_app_set_status(app, "operation cancelled");
    return ESP_OK;
  }

  if (event->key == CMD_KEY_ENTER)
  {
    op = app->fileop_confirm_op;
    app->fileop_confirm_open = false;
    app->fileop_confirm_op = CMD_APP_FILEOP_NONE;
    return cmd_app_run_fileop(app, op);
  }

  return ESP_OK;
}

esp_err_t cmd_app_reload_panel(CmdApp *app, CmdPanel *panel, const char *name)
{
  esp_err_t err;

  if (!app || !panel) return ESP_ERR_INVALID_ARG;

  err = cmd_panel_reload(panel);
  if (err != ESP_OK)
  {
    cmd_app_set_status_err(app, name, err);
    return err;
  }

  return ESP_OK;
}

esp_err_t cmd_app_init_panels(CmdApp *app)
{
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  cmd_panel_init(&app->left_panel, &app->fs, CMD_DEVICE_SD);
  cmd_panel_init(&app->right_panel, &app->fs, CMD_DEVICE_SD);

  err = cmd_panel_alloc_entries(&app->left_panel, CMD_PANEL_MAX_ENTRIES);
  if (err != ESP_OK) return err;

  err = cmd_panel_alloc_entries(&app->right_panel, CMD_PANEL_MAX_ENTRIES);
  if (err != ESP_OK) return err;

  cmd_app_set_active_panel(app, CMD_PANEL_LEFT);

  cmd_app_reload_panel(app, &app->left_panel, "left panel reload");
  cmd_app_reload_panel(app, &app->right_panel, "right panel reload");

  return ESP_OK;
}

void cmd_app_draw_top_bar(CmdApp *app)
{
  char line[CMD_DISPLAY_COLS + 1];
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  CmdPanel *panel;
  const char *status;

  if (!app) return;

  rect.x = 0;
  rect.y = CMD_LAYOUT_TOP_Y;
  rect.w = cmd_app_screen_w(app);
  rect.h = 1;
  attr = cmd_app_bar_attr();
  panel = cmd_app_active_panel(app);
  status = app->status[0] ? app->status : "ready";

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);

  cmd_app_make_top_bar_line(line, sizeof(line), panel, status);
  cmd_display_buffer_write_text(&app->buffer, 0, CMD_LAYOUT_TOP_Y, line, attr);
}

void cmd_app_draw_bottom_bar(CmdApp *app)
{
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  int y;

  if (!app) return;

  y = cmd_app_bottom_y(app);
  rect.x = 0;
  rect.y = y;
  rect.w = cmd_app_screen_w(app);
  rect.h = 1;
  attr = cmd_app_bar_attr();

  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_write_text(&app->buffer, 0, y,
    "F1 Help F2 Rename F3 View A+F3 Hex F5 Copy F6 Move F7 MkDir F8 Del", attr);
}

void cmd_app_make_panel_info_line(char *out, size_t out_size, const CmdPanel *panel, int width)
{
  const cmd_file_entry_t *entry;
  char size_text[32];
  size_t size_len;
  size_t name_len;
  size_t line_w;
  size_t size_pos;
  size_t name_max;

  if (!out || out_size == 0) return;

  cmd_app_text_clear(out, out_size);

  if (!panel || width <= 0) return;

  if (panel->selected_count > 0)
  {
    line_w = (size_t)width;
    if (line_w >= out_size) line_w = out_size - 1;
    if (line_w == 0) return;

    memset(out, ' ', line_w);
    out[line_w] = 0;

    snprintf(size_text, sizeof(size_text), "%u selected", (unsigned)panel->selected_count);
    name_len = strlen(size_text);
    if (name_len > line_w) name_len = line_w;
    if (name_len > 0) memcpy(out, size_text, name_len);

    snprintf(size_text, sizeof(size_text), "%llu bytes", (unsigned long long)panel->selected_size);
    size_len = strlen(size_text);
    if (size_len < line_w)
    {
      size_pos = line_w - size_len;
      if (size_pos > 0) size_pos--;
      memcpy(&out[size_pos], size_text, size_len);
    }
    return;
  }

  entry = cmd_panel_get_selected_entry(panel);
  if (!entry || entry->type != CMD_ENTRY_FILE) return;

  line_w = (size_t)width;
  if (line_w >= out_size) line_w = out_size - 1;
  if (line_w == 0) return;

  memset(out, ' ', line_w);
  out[line_w] = 0;

  snprintf(size_text, sizeof(size_text), "%llu bytes", (unsigned long long)entry->size);
  size_len = strlen(size_text);

  if (size_len >= line_w)
  {
    memcpy(out, size_text, line_w);
    return;
  }

  size_pos = line_w - size_len;
  if (size_pos > 0) size_pos--;
  memcpy(&out[size_pos], size_text, size_len);

  name_max = size_pos > 1 ? size_pos - 1 : 0;
  name_len = strlen(entry->name);
  if (name_len > name_max) name_len = name_max;
  if (name_len > 0) memcpy(out, entry->name, name_len);
}

void cmd_app_draw_panel_info_at(CmdApp *app, const CmdPanel *panel, int x, int y, int w)
{
  char line[CMD_DISPLAY_COLS + 1];
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  int text_w;

  if (!app || !panel) return;

  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = 1;
  attr = cmd_app_normal_attr();
  text_w = w;
  if (text_w < 0) text_w = 0;

  cmd_app_make_panel_info_line(line, sizeof(line), panel, text_w);
  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_write_text(&app->buffer, x, y, line, attr);
}

void cmd_app_make_panel_free_line(char *out, size_t out_size, CmdApp *app, const CmdPanel *panel, int width)
{
  char size_text[32];
  size_t line_w;
  esp_err_t err;

  if (!out || out_size == 0) return;

  cmd_app_text_clear(out, out_size);
  if (!app || !panel || width <= 0) return;

  line_w = (size_t)width;
  if (line_w >= out_size) line_w = out_size - 1;
  if (line_w == 0) return;

  memset(out, ' ', line_w);
  out[line_w] = 0;

  err = panel->space_info_valid ? panel->space_info_error : ESP_ERR_INVALID_STATE;
  if (err == ESP_OK)
  {
    cmd_app_format_bytes(size_text, sizeof(size_text), panel->space_info.free_bytes);
    snprintf(out, line_w + 1, "Free: %s", size_text);
    return;
  }

  snprintf(out, line_w + 1, "Free: n/a (%s)", cmd_err_name(err));
}

void cmd_app_draw_panel_free_at(CmdApp *app, const CmdPanel *panel, int x, int y, int w)
{
  char line[CMD_DISPLAY_COLS + 1];
  cmd_rect_t rect;
  cmd_display_attr_t attr;
  int text_w;

  if (!app || !panel) return;

  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = 1;
  attr = cmd_app_normal_attr();
  text_w = w;
  if (text_w < 0) text_w = 0;

  cmd_app_make_panel_free_line(line, sizeof(line), app, panel, text_w);
  cmd_display_buffer_fill_rect(&app->buffer, &rect, ' ', attr);
  cmd_display_buffer_write_text(&app->buffer, x, y, line, attr);
}

void cmd_app_draw_panel_info(CmdApp *app)
{
  cmd_rect_t left_rect;
  cmd_rect_t right_rect;
  int info_y;
  int free_y;
  int bottom_y;

  if (!app) return;

  cmd_app_make_panel_rects(app, &left_rect, &right_rect);
  info_y = cmd_app_panel_info_y(app);
  free_y = info_y + 1;
  bottom_y = cmd_app_bottom_y(app);
  if (free_y > bottom_y) free_y = bottom_y;

  cmd_app_draw_panel_free_at(app, &app->left_panel, left_rect.x, info_y, left_rect.w);
  cmd_app_draw_panel_free_at(app, &app->right_panel, right_rect.x, info_y, right_rect.w);
  cmd_app_draw_panel_info_at(app, &app->left_panel, left_rect.x, free_y, left_rect.w);
  cmd_app_draw_panel_info_at(app, &app->right_panel, right_rect.x, free_y, right_rect.w);
}

esp_err_t cmd_app_render(CmdApp *app)
{
  cmd_rect_t left_rect;
  cmd_rect_t right_rect;
  uint32_t console_flags;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  cmd_app_make_panel_rects(app, &left_rect, &right_rect);

  cmd_display_buffer_clear(&app->buffer, ' ', cmd_app_normal_attr());

  err = cmd_panel_draw(&app->left_panel, &app->buffer, &left_rect);
  if (err != ESP_OK) return err;

  err = cmd_panel_draw(&app->right_panel, &app->buffer, &right_rect);
  if (err != ESP_OK) return err;

  cmd_app_draw_panel_info(app);
  cmd_app_draw_device_dialog(app);
  cmd_app_draw_help_dialog(app);
  cmd_app_draw_mkdir_dialog(app);
  cmd_app_draw_confirm_dialog(app);
  cmd_app_draw_progress_dialog(app);
  cmd_app_draw_text_viewer(app);
  cmd_app_draw_hex_viewer(app);
  cmd_app_draw_module_info_viewer(app);
  cmd_app_draw_error_dialog(app);

  console_flags = cmd_display_console_backend.flags;
  if (app->viewer_open || app->hex_viewer_open || app->module_info_open)
    cmd_display_console_backend.flags = console_flags | CMD_DISPLAY_BACKEND_FLAG_FORCE_PARTIAL;

  err = cmd_display_mux_present(&app->mux, &app->buffer);
  cmd_display_console_backend.flags = console_flags;
  return err;
}

esp_err_t cmd_app_render_console(CmdApp *app)
{
  cmd_rect_t left_rect;
  cmd_rect_t right_rect;
  uint32_t console_flags;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;
  if (!cmd_app_has_console_backend(app)) return ESP_OK;

  cmd_app_make_panel_rects(app, &left_rect, &right_rect);

  cmd_display_buffer_clear(&app->buffer, ' ', cmd_app_normal_attr());

  err = cmd_panel_draw(&app->left_panel, &app->buffer, &left_rect);
  if (err != ESP_OK) return err;

  err = cmd_panel_draw(&app->right_panel, &app->buffer, &right_rect);
  if (err != ESP_OK) return err;

  cmd_app_draw_panel_info(app);
  cmd_app_draw_device_dialog(app);
  cmd_app_draw_help_dialog(app);
  cmd_app_draw_mkdir_dialog(app);
  cmd_app_draw_confirm_dialog(app);
  cmd_app_draw_progress_dialog(app);
  cmd_app_draw_text_viewer(app);
  cmd_app_draw_hex_viewer(app);
  cmd_app_draw_module_info_viewer(app);
  cmd_app_draw_error_dialog(app);

  console_flags = cmd_display_console_backend.flags;
  if (app->viewer_open || app->hex_viewer_open || app->module_info_open)
    cmd_display_console_backend.flags = console_flags | CMD_DISPLAY_BACKEND_FLAG_FORCE_PARTIAL;

  err = cmd_display_backend_present(&cmd_display_console_backend, &app->buffer);
  cmd_display_console_backend.flags = console_flags;
  return err;
}

esp_err_t cmd_app_enter_selected(CmdApp *app)
{
  CmdPanel *panel;
  const cmd_file_entry_t *entry;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  entry = cmd_panel_get_selected_entry(panel);
  if (!entry)
  {
    cmd_app_set_status(app, "no selected entry");
    return ESP_OK;
  }

  if (entry->type == CMD_ENTRY_DIR || entry->type == CMD_ENTRY_PARENT)
  {
    err = cmd_panel_enter_selected_dir(panel);
    if (err != ESP_OK) cmd_app_set_status_err(app, "enter", err);
    else cmd_app_set_status(app, "ready");
    return ESP_OK;
  }

  if (entry->type == CMD_ENTRY_FILE)
  {
    if (cmd_app_entry_is_xm(entry)) return cmd_app_play_xm_file(app, panel, entry);
    if (cmd_app_entry_is_wav(entry)) return cmd_app_play_wav_file(app, panel, entry);
    if (cmd_app_entry_is_ft_image(entry)) return cmd_app_open_jpg_viewer(app, panel, entry);
    cmd_app_set_status(app, "open: unsupported file type");
    return ESP_OK;
  }

  cmd_app_set_status(app, "unknown entry type");
  return ESP_OK;
}

esp_err_t cmd_app_go_parent(CmdApp *app)
{
  CmdPanel *panel;
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  err = cmd_panel_go_parent(panel);
  if (err != ESP_OK) cmd_app_set_status_err(app, "parent", err);
  else cmd_app_set_status(app, "ready");

  return ESP_OK;
}

bool cmd_app_event_is_exit(const cmd_event_t *event)
{
  if (!event) return false;
  if (event->type == CMD_EVENT_QUIT) return true;
  if (event->type != CMD_EVENT_KEY) return false;
  if (event->key == CMD_KEY_F10) return true;
  return false;
}

esp_err_t cmd_app_handle_key(CmdApp *app, const cmd_event_t *event)
{
  CmdPanel *panel;

  if (!app || !event) return ESP_ERR_INVALID_ARG;

  if (app->error_dialog_open)
    return cmd_app_handle_error_dialog_key(app, event);

  if (app->help_dialog_open)
    return cmd_app_handle_help_dialog_key(app, event);

  if (app->mkdir_dialog_open)
    return cmd_app_handle_mkdir_dialog_key(app, event);

  if (app->jpg_viewer_open)
    return cmd_app_handle_jpg_viewer_key(app, event);

  if (app->hex_viewer_open)
    return cmd_app_handle_hex_viewer_key(app, event);

  if (app->module_info_open)
    return cmd_app_handle_module_info_viewer_key(app, event);

  if (app->viewer_open)
    return cmd_app_handle_text_viewer_key(app, event);

  if (app->fileop_confirm_open)
    return cmd_app_handle_fileop_confirm_key(app, event);

  if (app->device_dialog_open)
    return cmd_app_handle_device_dialog_key(app, event);

  if ((event->mods & CMD_KEY_MOD_ALT) && event->key == CMD_KEY_F1)
  {
    cmd_app_open_device_dialog(app, CMD_PANEL_LEFT);
    return ESP_OK;
  }

  if (event->key == CMD_KEY_F1)
  {
    cmd_app_open_help_dialog(app);
    return ESP_OK;
  }

  if ((event->mods & CMD_KEY_MOD_ALT) && event->key == CMD_KEY_F2)
  {
    cmd_app_open_device_dialog(app, CMD_PANEL_RIGHT);
    return ESP_OK;
  }

  if ((event->mods & CMD_KEY_MOD_ALT) && event->key == CMD_KEY_F9)
    return cmd_app_stop_xm_playback(app);

  if (event->key == CMD_KEY_F9)
  {
    cmd_app_open_device_dialog(app, app->active_panel);
    return ESP_OK;
  }

  if (event->key == CMD_KEY_CHAR && event->ch == '1')
  {
    cmd_app_open_device_dialog(app, app->active_panel);
    return ESP_OK;
  }

  panel = cmd_app_active_panel(app);
  if (!panel) return ESP_ERR_INVALID_STATE;

  switch (event->key)
  {
    case CMD_KEY_INSERT:
      cmd_panel_toggle_selection(panel);
      if (cmd_panel_has_selection(panel))
        cmd_set_status(app, "%u selected, %llu bytes",
          (unsigned)panel->selected_count,
          (unsigned long long)panel->selected_size);
      else
        cmd_app_set_status(app, "selection cleared");
      return ESP_OK;

    case CMD_KEY_CHAR:
      if (event->ch != '*') return ESP_OK;
      cmd_panel_invert_selection(panel);
      if (cmd_panel_has_selection(panel))
        cmd_set_status(app, "%u selected, %llu bytes",
          (unsigned)panel->selected_count,
          (unsigned long long)panel->selected_size);
      else
        cmd_app_set_status(app, "selection cleared");
      return ESP_OK;

    case CMD_KEY_F2:
      return cmd_app_open_rename_dialog(app);

    case CMD_KEY_F3:
    {
      const cmd_file_entry_t *entry = cmd_panel_get_selected_entry(panel);

      if (event->mods & CMD_KEY_MOD_ALT)
        return cmd_app_open_hex_viewer(app, panel, entry);
      if (cmd_app_entry_is_module_info(entry))
        return cmd_app_open_module_info_viewer(app, panel, entry);
      return cmd_app_open_text_viewer(app, panel, entry);
    }

    case CMD_KEY_F5:
      return cmd_app_open_fileop_confirm(app, CMD_APP_FILEOP_COPY);

    case CMD_KEY_F6:
      return cmd_app_open_fileop_confirm(app, CMD_APP_FILEOP_MOVE);

    case CMD_KEY_F7:
      return cmd_app_open_mkdir_dialog(app);

    case CMD_KEY_F8:
    case CMD_KEY_DELETE:
      return cmd_app_open_fileop_confirm(app, CMD_APP_FILEOP_DELETE);

    case CMD_KEY_TAB:
      cmd_app_switch_panel(app);
      return ESP_OK;

    case CMD_KEY_UP:
      cmd_panel_cursor_up(panel);
      return ESP_OK;

    case CMD_KEY_DOWN:
      cmd_panel_cursor_down(panel);
      return ESP_OK;

    case CMD_KEY_PAGE_UP:
    case CMD_KEY_LEFT:
      cmd_panel_cursor_page_up(panel);
      return ESP_OK;

    case CMD_KEY_PAGE_DOWN:
    case CMD_KEY_RIGHT:
      cmd_panel_cursor_page_down(panel);
      return ESP_OK;

    case CMD_KEY_HOME:
      cmd_panel_cursor_home(panel);
      return ESP_OK;

    case CMD_KEY_END:
      cmd_panel_cursor_end(panel);
      return ESP_OK;

    case CMD_KEY_ENTER:
      return cmd_app_enter_selected(app);

    case CMD_KEY_BACKSPACE:
      return cmd_app_go_parent(app);

    default:
      return ESP_OK;
  }
}

esp_err_t cmd_app_handle_event(CmdApp *app, const cmd_event_t *event)
{
  if (!app || !event) return ESP_ERR_INVALID_ARG;
  if (event->type != CMD_EVENT_KEY) return ESP_OK;

  return cmd_app_handle_key(app, event);
}


bool cmd_app_parse_baud_arg(const char *text, uint32_t *baud)
{
  char *end = NULL;
  unsigned long value;

  if (!text || !text[0] || !baud) return false;

  errno = 0;
  value = strtoul(text, &end, 10);
  if (errno != 0 || !end || *end || value == 0 || value > 0xffffffffUL) return false;

  *baud = (uint32_t)value;
  return true;
}

bool cmd_app_parse_launch_args(int argc, char **argv, CmdDisplayMode *display_mode, uint32_t *baud)
{
  int arg = 1;

  if (!display_mode || !baud) return false;

  *display_mode = CMD_DISPLAY_MODE_CONSOLE;
  *baud = 0;

  if (argc < 0 || !argv) return false;

  if (arg < argc && cmd_display_mode_parse(argv[arg], display_mode))
    arg++;

  if (arg < argc && cmd_app_parse_baud_arg(argv[arg], baud))
    arg++;

  return arg == argc;
}

void cmd_app_print_usage()
{
  printf("usage: cmd [console|ft|ft812|mirror] [baud]\r\n");
  printf("       cmd [baud]\r\n");
}

esp_err_t cmd_app_uart_wait_tx_done()
{
  return uart_wait_tx_done(
    (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
    pdMS_TO_TICKS(1000));
}

void cmd_app_uart_send_baud_msg(const char *name, uint32_t baud, uint32_t nonce)
{
  printf("\x1b]777;ZIFI32;%s;%lu;%08lX\x1b\\",
    name,
    (unsigned long)baud,
    (unsigned long)nonce);
  fflush(stdout);
}

size_t cmd_app_match_seq(size_t pos, const char *seq, size_t seq_len, uint8_t c)
{
  if (!seq || seq_len == 0) return 0;

  if (c == (uint8_t)seq[pos])
  {
    pos++;
    if (pos >= seq_len) return seq_len;
    return pos;
  }

  if (c == (uint8_t)seq[0]) return 1;
  return 0;
}

bool cmd_app_uart_wait_baud_msg(const char *name, uint32_t baud, uint32_t nonce, uint32_t timeout_ms)
{
  char expected_st[CMD_APP_BAUD_SEQ_MAX];
  char expected_bel[CMD_APP_BAUD_SEQ_MAX];
  size_t expected_st_len;
  size_t expected_bel_len;
  size_t match_st = 0;
  size_t match_bel = 0;
  TickType_t deadline;

  if (!name) return false;

  snprintf(expected_st, sizeof(expected_st), "\x1b]777;ZIFI32;%s;%lu;%08lX\x1b\\",
    name,
    (unsigned long)baud,
    (unsigned long)nonce);
  snprintf(expected_bel, sizeof(expected_bel), "\x1b]777;ZIFI32;%s;%lu;%08lX\a",
    name,
    (unsigned long)baud,
    (unsigned long)nonce);

  expected_st_len = strlen(expected_st);
  expected_bel_len = strlen(expected_bel);
  deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

  while ((int32_t)(deadline - xTaskGetTickCount()) > 0)
  {
    uint8_t c = 0;
    int got = uart_read_bytes(
      (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
      &c,
      1,
      pdMS_TO_TICKS(10));

    if (got <= 0) continue;

    match_st = cmd_app_match_seq(match_st, expected_st, expected_st_len, c);
    match_bel = cmd_app_match_seq(match_bel, expected_bel, expected_bel_len, c);

    if (match_st == expected_st_len || match_bel == expected_bel_len) return true;
  }

  return false;
}

uint32_t cmd_app_uart_canonical_baud(uint32_t baud)
{
  const uint32_t known[] =
  {
    115200,
    230400,
    460800,
    921600,
    1000000,
    1500000,
    2000000
  };

  for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++)
  {
    uint32_t ref = known[i];
    uint32_t diff = baud > ref ? baud - ref : ref - baud;
    uint32_t limit = ref / 1000;

    if (limit < 16) limit = 16;
    if (diff <= limit) return ref;
  }

  return baud;
}

bool cmd_app_uart_restore_needed(uint32_t requested_baud, esp_err_t baud_err)
{
  if (requested_baud == 0) return false;
  if (baud_err != ESP_OK) return false;
  return cmd_app_uart_canonical_baud(requested_baud) != CMD_APP_BAUD_DEFAULT;
}

esp_err_t cmd_app_uart_switch_baud(uint32_t baud, uint32_t *old_baud, bool *switched)
{
  uart_port_t uart_num = (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM;
  uint32_t current_baud = 0;
  uint32_t nonce;
  esp_err_t err;

  if (old_baud) *old_baud = 0;
  if (switched) *switched = false;
  if (baud == 0) return ESP_ERR_INVALID_ARG;

  err = uart_get_baudrate(uart_num, &current_baud);
  if (err != ESP_OK) return err;

  current_baud = cmd_app_uart_canonical_baud(current_baud);
  baud = cmd_app_uart_canonical_baud(baud);

  if (old_baud) *old_baud = current_baud;
  if (current_baud == baud) return ESP_OK;

  uart_flush_input(uart_num);

  nonce = ((uint32_t)xTaskGetTickCount() << 16) ^ baud ^ current_baud;
  cmd_app_uart_send_baud_msg("BAUD-REQ", baud, nonce);
  err = cmd_app_uart_wait_tx_done();
  if (err != ESP_OK) return err;

  if (!cmd_app_uart_wait_baud_msg("BAUD-ACK", baud, nonce, 300))
    return ESP_ERR_TIMEOUT;

  cmd_app_uart_send_baud_msg("BAUD-GO", baud, nonce);
  err = cmd_app_uart_wait_tx_done();
  if (err != ESP_OK) return err;

  vTaskDelay(pdMS_TO_TICKS(CMD_APP_BAUD_SWITCH_DELAY_MS));

  err = uart_set_baudrate(uart_num, baud);
  if (err != ESP_OK) return err;

  vTaskDelay(pdMS_TO_TICKS(CMD_APP_BAUD_SWITCH_DELAY_MS));

  if (!cmd_app_uart_wait_baud_msg("BAUD-RDY", baud, nonce, 1000))
  {
    uart_set_baudrate(uart_num, current_baud);
    vTaskDelay(pdMS_TO_TICKS(CMD_APP_BAUD_SWITCH_DELAY_MS));
    return ESP_ERR_TIMEOUT;
  }

  cmd_app_uart_send_baud_msg("BAUD-OK", baud, nonce);
  err = cmd_app_uart_wait_tx_done();
  if (err != ESP_OK) return err;

  if (!cmd_app_uart_wait_baud_msg("BAUD-DONE", baud, nonce, 500))
  {
    uart_set_baudrate(uart_num, current_baud);
    vTaskDelay(pdMS_TO_TICKS(CMD_APP_BAUD_SWITCH_DELAY_MS));
    return ESP_ERR_TIMEOUT;
  }

  if (switched) *switched = true;
  return ESP_OK;
}

void cmd_app_mount_internal_fs_early(CmdApp *app)
{
  if (!app) return;

  cmd_fs_manager_mount(&app->fs, CMD_DEVICE_FAT);
  cmd_fs_manager_mount(&app->fs, CMD_DEVICE_TSF);
}

esp_err_t cmd_app_init_runtime(CmdApp *app, CmdDisplayMode display_mode)
{
  esp_err_t err;

  if (!app) return ESP_ERR_INVALID_ARG;

  memset(app, 0, sizeof(*app));
  app->sfx_preview_handle = -1;
  app->sfx_preview_channel = -1;

  cmd_display_buffer_init(&app->buffer);
  cmd_display_mux_init(&app->mux);
  cmd_fs_manager_init(&app->fs);

  err = cmd_fs_manager_register_default_drivers(&app->fs);
  if (err != ESP_OK) return err;

  cmd_app_mount_internal_fs_early(app);

  err = cmd_panel_alloc_global_buffers();
  if (err != ESP_OK) return err;

  err = cmd_fileops_alloc_global_buffers();
  if (err != ESP_OK) return err;

  err = cmd_display_mux_add_mode(&app->mux, display_mode);
  if (err != ESP_OK) return err;

  err = cmd_display_mux_init_backends(&app->mux);
  if (err != ESP_OK) return err;

  cmd_size_t display_size;
  err = cmd_display_mux_get_size(&app->mux, &display_size);
  if (err != ESP_OK) return err;

  err = cmd_display_buffer_set_size(&app->buffer, display_size.w, display_size.h);
  if (err != ESP_OK) return err;

  app->input = cmd_input_default_backend();
  err = cmd_input_begin(app->input);
  if (err != ESP_OK) return err;

  cmd_set_status(app, "ready display=%s %dx%d", cmd_display_mode_name(display_mode), app->buffer.size.w, app->buffer.size.h);
  return cmd_app_init_panels(app);
}

void cmd_app_shutdown_runtime(CmdApp *app)
{
  if (!app) return;

  cmd_app_delete_sfx_preview(app);
  cmd_app_close_module_info_viewer(app);
  cmd_input_end(app->input);
  cmd_display_mux_deinit_backends(&app->mux);
  cmd_fs_manager_unmount_all(&app->fs);
  cmd_panel_free_entries(&app->left_panel);
  cmd_panel_free_entries(&app->right_panel);
  cmd_fileops_free_global_buffers();
  cmd_panel_free_global_buffers();
  printf("\x1b[0m\x1b[?25h\x1b[2J\x1b[H");
  fflush(stdout);
}

int cmd_cmd(int argc, char **argv)
{
  CmdApp *app;
  CmdDisplayMode display_mode;
  uint32_t requested_baud = 0;
  uint32_t old_baud = 0;
  bool baud_switched = false;
  bool baud_restore_needed = false;
  esp_err_t baud_err = ESP_OK;
  esp_err_t err;

  if (argc < 0 || !argv) return 1;
  if (g_cmd_app) return 1;

  if (!cmd_app_parse_launch_args(argc, argv, &display_mode, &requested_baud))
  {
    cmd_app_print_usage();
    return 1;
  }

  if (requested_baud > 0)
  {
    baud_err = cmd_app_uart_switch_baud(requested_baud, &old_baud, &baud_switched);
    baud_restore_needed = cmd_app_uart_restore_needed(requested_baud, baud_err);
    if (baud_err != ESP_OK && baud_err != ESP_ERR_TIMEOUT)
      printf("W: cmd baud switch failed: %s\r\n", cmd_err_name(baud_err));
  }

  app = cmd_app_alloc();
  if (!app)
  {
    if (baud_restore_needed) cmd_app_uart_switch_baud(CMD_APP_BAUD_DEFAULT, NULL, NULL);
    printf("E: cmd alloc failed: ESP_ERR_NO_MEM\r\n");
    return 1;
  }

  g_cmd_app = app;

  err = cmd_app_init_runtime(app, display_mode);
  if (err != ESP_OK)
  {
    if (baud_restore_needed) cmd_app_uart_switch_baud(CMD_APP_BAUD_DEFAULT, NULL, NULL);
    printf("E: cmd init failed: %s\r\n", cmd_err_name(err));
    cmd_app_shutdown_runtime(app);
    cmd_app_free(app);
    g_cmd_app = NULL;
    return 1;
  }

  printf("\x1b[?25l");
  cmd_app_render(app);

  while (true)
  {
    cmd_event_t event;
    uint32_t input_timeout_ms = app->help_dialog_open ? CMD_APP_HELP_DIALOG_REFRESH_MS : CMD_INPUT_WAIT_FOREVER;

    err = cmd_input_poll_event(app->input, &event, input_timeout_ms);
    if (err == ESP_ERR_TIMEOUT)
    {
      if (app->help_dialog_open)
        cmd_app_render(app);
      continue;
    }
    if (err != ESP_OK) continue;

    if (!cmd_app_has_modal(app) && cmd_app_event_is_exit(&event)) break;

    err = cmd_app_handle_event(app, &event);
    if (err != ESP_OK) cmd_show_error(app, "event", err);

    if (!app->jpg_viewer_open)
      cmd_app_render(app);
  }

  if (baud_restore_needed) cmd_app_uart_switch_baud(CMD_APP_BAUD_DEFAULT, NULL, NULL);

  cmd_app_shutdown_runtime(app);
  cmd_app_free(app);
  g_cmd_app = NULL;
  return 0;
}

void cmd_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "cmd",
    .help     = "Two-panel file commander",
    .hint     = NULL,
    .func     = &cmd_cmd,
    .argtable = NULL
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// --------------- cmd_display.cpp ---------------

#ifndef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM UART_NUM_0
#endif

#define CMD_DISPLAY_TERMINAL_QUERY_TIMEOUT_MS 250

CmdDisplayBackend cmd_display_console_backend =
{
  .name = "console",
  .ctx = NULL,
  .flags = 0,
  .init = cmd_display_console_init,
  .deinit = cmd_display_console_deinit,
  .clear = cmd_display_console_clear,
  .present = cmd_display_console_present,
  .put_char = cmd_display_console_put_char,
  .write_text = cmd_display_console_write_text,
  .get_size = cmd_display_console_get_size
};

CmdDisplayBackend cmd_display_ft812_backend =
{
  .name = "ft812",
  .ctx = NULL,
  .flags = 0,
  .init = cmd_display_ft812_begin,
  .deinit = cmd_display_ft812_end,
  .clear = cmd_display_ft812_clear,
  .present = cmd_display_ft812_present,
  .put_char = cmd_display_ft812_put_char,
  .write_text = cmd_display_ft812_write_text,
  .get_size = cmd_display_ft812_get_size
};

typedef struct
{
  CmdDisplayBuffer prev;
  bool prev_valid;
} CmdDisplayConsoleContext;

typedef struct
{
  cmd_display_attr_t attr;
  int x;
  int y;
  int width;
  bool attr_valid;
  bool pos_valid;
} CmdDisplayConsoleEmitState;

typedef struct
{
  uint8_t chars[CMD_DISPLAY_CELL_COUNT];
  uint8_t attrs[CMD_DISPLAY_CELL_COUNT];
  bool active;
} CmdDisplayFt812Context;

EXT_RAM_BSS_ATTR CmdDisplayConsoleContext cmd_display_console_ctx;
EXT_RAM_BSS_ATTR CmdDisplayFt812Context cmd_display_ft812_ctx;

cmd_display_attr_t cmd_display_make_attr(cmd_display_color_t fg, cmd_display_color_t bg, uint8_t flags)
{
  return (cmd_display_attr_t)(((uint16_t)fg & 0x0f) |
    (((uint16_t)bg & 0x0f) << CMD_DISPLAY_ATTR_BG_SHIFT) |
    ((uint16_t)flags << CMD_DISPLAY_ATTR_FLAGS_SHIFT));
}

cmd_display_color_t cmd_display_attr_fg(cmd_display_attr_t attr)
{
  return (cmd_display_color_t)(attr & CMD_DISPLAY_ATTR_FG_MASK);
}

cmd_display_color_t cmd_display_attr_bg(cmd_display_attr_t attr)
{
  return (cmd_display_color_t)((attr & CMD_DISPLAY_ATTR_BG_MASK) >> CMD_DISPLAY_ATTR_BG_SHIFT);
}

uint8_t cmd_display_attr_flags(cmd_display_attr_t attr)
{
  return (uint8_t)((attr & CMD_DISPLAY_ATTR_FLAGS_MASK) >> CMD_DISPLAY_ATTR_FLAGS_SHIFT);
}

const char *cmd_display_mode_name(CmdDisplayMode mode)
{
  switch (mode)
  {
    case CMD_DISPLAY_MODE_CONSOLE: return "console";
    case CMD_DISPLAY_MODE_FT812: return "ft812";
    case CMD_DISPLAY_MODE_MIRROR: return "mirror";
    default: return "?";
  }
}

bool cmd_display_mode_parse(const char *text, CmdDisplayMode *mode)
{
  if (!mode) return false;

  if (!text || !text[0] || !strcmp(text, "console"))
  {
    *mode = CMD_DISPLAY_MODE_CONSOLE;
    return true;
  }

  if (!strcmp(text, "ft") || !strcmp(text, "ft812"))
  {
    *mode = CMD_DISPLAY_MODE_FT812;
    return true;
  }

  if (!strcmp(text, "mirror") || !strcmp(text, "both"))
  {
    *mode = CMD_DISPLAY_MODE_MIRROR;
    return true;
  }

  return false;
}

bool cmd_display_mode_has_console(CmdDisplayMode mode)
{
  return mode == CMD_DISPLAY_MODE_CONSOLE || mode == CMD_DISPLAY_MODE_MIRROR;
}

bool cmd_display_mode_has_ft812(CmdDisplayMode mode)
{
  return mode == CMD_DISPLAY_MODE_FT812 || mode == CMD_DISPLAY_MODE_MIRROR;
}

bool cmd_display_point_inside(int x, int y)
{
  if (x < 0 || y < 0) return false;
  if (x >= CMD_DISPLAY_COLS || y >= CMD_DISPLAY_ROWS) return false;
  return true;
}

bool cmd_display_buffer_point_inside(const CmdDisplayBuffer *buffer, int x, int y)
{
  if (!buffer) return false;
  if (x < 0 || y < 0) return false;
  if (x >= buffer->size.w || y >= buffer->size.h) return false;
  return true;
}

esp_err_t cmd_display_buffer_set_size(CmdDisplayBuffer *buffer, int w, int h)
{
  if (!buffer) return ESP_ERR_INVALID_ARG;

  if (w <= 0) w = CMD_DISPLAY_DEFAULT_COLS;
  if (h <= 0) h = CMD_DISPLAY_DEFAULT_ROWS;
  if (w > CMD_DISPLAY_COLS) w = CMD_DISPLAY_COLS;
  if (h > CMD_DISPLAY_ROWS) h = CMD_DISPLAY_ROWS;

  buffer->size.w = w;
  buffer->size.h = h;
  return ESP_OK;
}

void cmd_display_buffer_init(CmdDisplayBuffer *buffer)
{
  if (!buffer) return;

  memset(buffer, 0, sizeof(*buffer));
  cmd_display_buffer_set_size(buffer, CMD_DISPLAY_DEFAULT_COLS, CMD_DISPLAY_DEFAULT_ROWS);
  cmd_display_buffer_clear(buffer, ' ', CMD_DISPLAY_ATTR_DEFAULT);
}

void cmd_display_buffer_clear(CmdDisplayBuffer *buffer, char ch, cmd_display_attr_t attr)
{
  if (!buffer) return;

  if (ch == 0)
    ch = ' ';

  for (int y = 0; y < buffer->size.h; y++)
  {
    for (int x = 0; x < buffer->size.w; x++)
    {
      buffer->cells[y][x].ch = ch;
      buffer->cells[y][x].attr = attr;
    }
  }
}

CmdDisplayCell *cmd_display_buffer_cell(CmdDisplayBuffer *buffer, int x, int y)
{
  if (!cmd_display_buffer_point_inside(buffer, x, y)) return NULL;
  return &buffer->cells[y][x];
}

const CmdDisplayCell *cmd_display_buffer_cell_const(const CmdDisplayBuffer *buffer, int x, int y)
{
  if (!cmd_display_buffer_point_inside(buffer, x, y)) return NULL;
  return &buffer->cells[y][x];
}

esp_err_t cmd_display_buffer_put_char(CmdDisplayBuffer *buffer, int x, int y, char ch, cmd_display_attr_t attr)
{
  CmdDisplayCell *cell;

  if (!buffer) return ESP_ERR_INVALID_ARG;
  if (!cmd_display_buffer_point_inside(buffer, x, y)) return ESP_ERR_INVALID_ARG;

  if (ch == 0)
    ch = ' ';

  cell = &buffer->cells[y][x];
  cell->ch = ch;
  cell->attr = attr;
  return ESP_OK;
}

esp_err_t cmd_display_buffer_write_text(CmdDisplayBuffer *buffer, int x, int y, const char *text, cmd_display_attr_t attr)
{
  int pos = x;

  if (!buffer || !text) return ESP_ERR_INVALID_ARG;
  if (y < 0 || y >= buffer->size.h) return ESP_ERR_INVALID_ARG;
  if (x >= buffer->size.w) return ESP_OK;
  if (pos < 0) pos = 0;

  while (*text && pos < buffer->size.w)
  {
    esp_err_t err = cmd_display_buffer_put_char(buffer, pos, y, *text, attr);
    if (err != ESP_OK) return err;
    text++;
    pos++;
  }

  return ESP_OK;
}

esp_err_t cmd_display_buffer_fill_rect(CmdDisplayBuffer *buffer, const cmd_rect_t *rect, char ch, cmd_display_attr_t attr)
{
  int x0;
  int y0;
  int x1;
  int y1;

  if (!buffer || !rect) return ESP_ERR_INVALID_ARG;
  if (rect->w <= 0 || rect->h <= 0) return ESP_OK;

  x0 = rect->x;
  y0 = rect->y;
  x1 = rect->x + rect->w;
  y1 = rect->y + rect->h;

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > buffer->size.w) x1 = buffer->size.w;
  if (y1 > buffer->size.h) y1 = buffer->size.h;
  if (x0 >= x1 || y0 >= y1) return ESP_OK;

  for (int y = y0; y < y1; y++)
  {
    for (int x = x0; x < x1; x++)
    {
      esp_err_t err = cmd_display_buffer_put_char(buffer, x, y, ch, attr);
      if (err != ESP_OK) return err;
    }
  }

  return ESP_OK;
}

esp_err_t cmd_display_buffer_draw_hline(CmdDisplayBuffer *buffer, int x, int y, int w, char ch, cmd_display_attr_t attr)
{
  if (!buffer) return ESP_ERR_INVALID_ARG;
  if (y < 0 || y >= buffer->size.h) return ESP_ERR_INVALID_ARG;
  if (w <= 0) return ESP_OK;

  for (int i = 0; i < w; i++)
  {
    int px = x + i;
    if (px < 0) continue;
    if (px >= buffer->size.w) break;

    esp_err_t err = cmd_display_buffer_put_char(buffer, px, y, ch, attr);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}

esp_err_t cmd_display_buffer_draw_vline(CmdDisplayBuffer *buffer, int x, int y, int h, char ch, cmd_display_attr_t attr)
{
  if (!buffer) return ESP_ERR_INVALID_ARG;
  if (x < 0 || x >= buffer->size.w) return ESP_ERR_INVALID_ARG;
  if (h <= 0) return ESP_OK;

  for (int i = 0; i < h; i++)
  {
    int py = y + i;
    if (py < 0) continue;
    if (py >= buffer->size.h) break;

    esp_err_t err = cmd_display_buffer_put_char(buffer, x, py, ch, attr);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}

esp_err_t cmd_display_buffer_draw_box(CmdDisplayBuffer *buffer, const cmd_rect_t *rect, cmd_display_attr_t attr)
{
  esp_err_t err;

  if (!buffer || !rect) return ESP_ERR_INVALID_ARG;
  if (rect->w <= 0 || rect->h <= 0) return ESP_OK;

  if (rect->w == 1 && rect->h == 1)
    return cmd_display_buffer_put_char(buffer, rect->x, rect->y, CMD_DISPLAY_BOX_TL, attr);

  err = cmd_display_buffer_draw_hline(buffer, rect->x, rect->y, rect->w, CMD_DISPLAY_BOX_H, attr);
  if (err != ESP_OK) return err;

  err = cmd_display_buffer_draw_hline(buffer, rect->x, rect->y + rect->h - 1, rect->w, CMD_DISPLAY_BOX_H, attr);
  if (err != ESP_OK) return err;

  err = cmd_display_buffer_draw_vline(buffer, rect->x, rect->y, rect->h, CMD_DISPLAY_BOX_V, attr);
  if (err != ESP_OK) return err;

  err = cmd_display_buffer_draw_vline(buffer, rect->x + rect->w - 1, rect->y, rect->h, CMD_DISPLAY_BOX_V, attr);
  if (err != ESP_OK) return err;

  cmd_display_buffer_put_char(buffer, rect->x, rect->y, CMD_DISPLAY_BOX_TL, attr);
  cmd_display_buffer_put_char(buffer, rect->x + rect->w - 1, rect->y, CMD_DISPLAY_BOX_TR, attr);
  cmd_display_buffer_put_char(buffer, rect->x, rect->y + rect->h - 1, CMD_DISPLAY_BOX_BL, attr);
  cmd_display_buffer_put_char(buffer, rect->x + rect->w - 1, rect->y + rect->h - 1, CMD_DISPLAY_BOX_BR, attr);

  return ESP_OK;
}

esp_err_t cmd_display_backend_init(CmdDisplayBackend *backend)
{
  if (!backend) return ESP_ERR_INVALID_ARG;
  if (!backend->init) return ESP_OK;
  return backend->init(backend);
}

void cmd_display_backend_deinit(CmdDisplayBackend *backend)
{
  if (!backend || !backend->deinit) return;
  backend->deinit(backend);
}

esp_err_t cmd_display_backend_clear(CmdDisplayBackend *backend)
{
  if (!backend) return ESP_ERR_INVALID_ARG;
  if (!backend->clear) return ESP_ERR_NOT_SUPPORTED;
  return backend->clear(backend);
}

esp_err_t cmd_display_backend_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer)
{
  if (!backend || !buffer) return ESP_ERR_INVALID_ARG;
  if (!backend->present) return ESP_ERR_NOT_SUPPORTED;
  return backend->present(backend, buffer);
}

esp_err_t cmd_display_backend_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr)
{
  if (!backend) return ESP_ERR_INVALID_ARG;
  if (!backend->put_char) return ESP_ERR_NOT_SUPPORTED;
  return backend->put_char(backend, x, y, ch, attr);
}

esp_err_t cmd_display_backend_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr)
{
  if (!backend || !text) return ESP_ERR_INVALID_ARG;
  if (!backend->write_text) return ESP_ERR_NOT_SUPPORTED;
  return backend->write_text(backend, x, y, text, attr);
}

esp_err_t cmd_display_backend_get_size(CmdDisplayBackend *backend, cmd_size_t *size)
{
  if (!backend || !size) return ESP_ERR_INVALID_ARG;

  size->w = CMD_DISPLAY_DEFAULT_COLS;
  size->h = CMD_DISPLAY_DEFAULT_ROWS;

  if (!backend->get_size) return ESP_OK;
  return backend->get_size(backend, size);
}

void cmd_display_mux_init(CmdDisplayMux *mux)
{
  if (!mux) return;
  memset(mux, 0, sizeof(*mux));
}

esp_err_t cmd_display_mux_add_backend(CmdDisplayMux *mux, CmdDisplayBackend *backend)
{
  if (!mux || !backend) return ESP_ERR_INVALID_ARG;
  if (mux->count >= CMD_DISPLAY_MUX_MAX_BACKENDS) return ESP_ERR_INVALID_SIZE;

  for (size_t i = 0; i < mux->count; i++)
  {
    if (mux->backends[i] == backend) return ESP_OK;
  }

  mux->backends[mux->count] = backend;
  mux->count++;
  return ESP_OK;
}

esp_err_t cmd_display_mux_add_mode(CmdDisplayMux *mux, CmdDisplayMode mode)
{
  esp_err_t err;

  if (!mux) return ESP_ERR_INVALID_ARG;

  if (mode == CMD_DISPLAY_MODE_FT812 || mode == CMD_DISPLAY_MODE_MIRROR)
  {
    err = cmd_display_mux_add_backend(mux, &cmd_display_ft812_backend);
    if (err != ESP_OK) return err;
  }

  if (mode == CMD_DISPLAY_MODE_CONSOLE || mode == CMD_DISPLAY_MODE_MIRROR)
  {
    err = cmd_display_mux_add_backend(mux, &cmd_display_console_backend);
    if (err != ESP_OK) return err;
  }

  if (mux->count == 0) return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}

esp_err_t cmd_display_mux_remove_backend(CmdDisplayMux *mux, CmdDisplayBackend *backend)
{
  if (!mux || !backend) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    if (mux->backends[i] != backend)
      continue;

    for (size_t j = i; j + 1 < mux->count; j++)
      mux->backends[j] = mux->backends[j + 1];

    mux->count--;
    mux->backends[mux->count] = NULL;
    return ESP_OK;
  }

  return ESP_ERR_NOT_FOUND;
}

esp_err_t cmd_display_mux_init_backends(CmdDisplayMux *mux)
{
  if (!mux) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    esp_err_t err = cmd_display_backend_init(mux->backends[i]);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}

esp_err_t cmd_display_mux_get_size(CmdDisplayMux *mux, cmd_size_t *size)
{
  bool have = false;

  if (!mux || !size) return ESP_ERR_INVALID_ARG;

  size->w = CMD_DISPLAY_DEFAULT_COLS;
  size->h = CMD_DISPLAY_DEFAULT_ROWS;

  for (size_t i = 0; i < mux->count; i++)
  {
    cmd_size_t backend_size;
    esp_err_t err = cmd_display_backend_get_size(mux->backends[i], &backend_size);

    if (err != ESP_OK) continue;
    if (backend_size.w <= 0 || backend_size.h <= 0) continue;

    if (backend_size.w > CMD_DISPLAY_COLS) backend_size.w = CMD_DISPLAY_COLS;
    if (backend_size.h > CMD_DISPLAY_ROWS) backend_size.h = CMD_DISPLAY_ROWS;

    if (!have)
    {
      *size = backend_size;
      have = true;
    }
    else
    {
      if (backend_size.w < size->w) size->w = backend_size.w;
      if (backend_size.h < size->h) size->h = backend_size.h;
    }
  }

  return ESP_OK;
}

void cmd_display_mux_deinit_backends(CmdDisplayMux *mux)
{
  if (!mux) return;

  for (size_t i = 0; i < mux->count; i++)
    cmd_display_backend_deinit(mux->backends[i]);
}

esp_err_t cmd_display_mux_clear(CmdDisplayMux *mux)
{
  esp_err_t first_err = ESP_OK;

  if (!mux) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    esp_err_t err = cmd_display_backend_clear(mux->backends[i]);
    if (first_err == ESP_OK && err != ESP_OK) first_err = err;
  }

  return first_err;
}

esp_err_t cmd_display_mux_present(CmdDisplayMux *mux, const CmdDisplayBuffer *buffer)
{
  esp_err_t first_err = ESP_OK;

  if (!mux || !buffer) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    esp_err_t err = cmd_display_backend_present(mux->backends[i], buffer);
    if (first_err == ESP_OK && err != ESP_OK) first_err = err;
  }

  return first_err;
}

esp_err_t cmd_display_mux_put_char(CmdDisplayMux *mux, int x, int y, char ch, cmd_display_attr_t attr)
{
  esp_err_t first_err = ESP_OK;

  if (!mux) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    esp_err_t err = cmd_display_backend_put_char(mux->backends[i], x, y, ch, attr);
    if (first_err == ESP_OK && err != ESP_OK) first_err = err;
  }

  return first_err;
}

esp_err_t cmd_display_mux_write_text(CmdDisplayMux *mux, int x, int y, const char *text, cmd_display_attr_t attr)
{
  esp_err_t first_err = ESP_OK;

  if (!mux || !text) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < mux->count; i++)
  {
    esp_err_t err = cmd_display_backend_write_text(mux->backends[i], x, y, text, attr);
    if (first_err == ESP_OK && err != ESP_OK) first_err = err;
  }

  return first_err;
}

int cmd_display_console_fg_code(cmd_display_color_t color)
{
  switch (color)
  {
    case CMD_DISPLAY_COLOR_BLACK: return 30;
    case CMD_DISPLAY_COLOR_BLUE: return 34;
    case CMD_DISPLAY_COLOR_GREEN: return 32;
    case CMD_DISPLAY_COLOR_CYAN: return 36;
    case CMD_DISPLAY_COLOR_RED: return 31;
    case CMD_DISPLAY_COLOR_MAGENTA: return 35;
    case CMD_DISPLAY_COLOR_YELLOW: return 33;
    case CMD_DISPLAY_COLOR_WHITE: return 37;
    case CMD_DISPLAY_COLOR_BRIGHT_BLACK: return 90;
    case CMD_DISPLAY_COLOR_BRIGHT_BLUE: return 94;
    case CMD_DISPLAY_COLOR_BRIGHT_GREEN: return 92;
    case CMD_DISPLAY_COLOR_BRIGHT_CYAN: return 96;
    case CMD_DISPLAY_COLOR_BRIGHT_RED: return 91;
    case CMD_DISPLAY_COLOR_BRIGHT_MAGENTA: return 95;
    case CMD_DISPLAY_COLOR_BRIGHT_YELLOW: return 93;
    case CMD_DISPLAY_COLOR_BRIGHT_WHITE: return 97;
    default: return 37;
  }
}

int cmd_display_console_bg_code(cmd_display_color_t color)
{
  switch (color)
  {
    case CMD_DISPLAY_COLOR_BLACK: return 40;
    case CMD_DISPLAY_COLOR_BLUE: return 44;
    case CMD_DISPLAY_COLOR_GREEN: return 42;
    case CMD_DISPLAY_COLOR_CYAN: return 46;
    case CMD_DISPLAY_COLOR_RED: return 41;
    case CMD_DISPLAY_COLOR_MAGENTA: return 45;
    case CMD_DISPLAY_COLOR_YELLOW: return 43;
    case CMD_DISPLAY_COLOR_WHITE: return 47;
    case CMD_DISPLAY_COLOR_BRIGHT_BLACK: return 100;
    case CMD_DISPLAY_COLOR_BRIGHT_BLUE: return 104;
    case CMD_DISPLAY_COLOR_BRIGHT_GREEN: return 102;
    case CMD_DISPLAY_COLOR_BRIGHT_CYAN: return 106;
    case CMD_DISPLAY_COLOR_BRIGHT_RED: return 101;
    case CMD_DISPLAY_COLOR_BRIGHT_MAGENTA: return 105;
    case CMD_DISPLAY_COLOR_BRIGHT_YELLOW: return 103;
    case CMD_DISPLAY_COLOR_BRIGHT_WHITE: return 107;
    default: return 40;
  }
}

const char *cmd_display_console_utf8_char(char ch)
{
  switch ((uint8_t)ch)
  {
    case 0xC9: return "╔";
    case 0xBB: return "╗";
    case 0xC8: return "╚";
    case 0xBC: return "╝";
    case 0xCD: return "═";
    case 0xBA: return "║";
    default: return NULL;
  }
}

void cmd_display_console_emit_char(char ch)
{
  const char *utf8 = cmd_display_console_utf8_char(ch);

  if (utf8)
  {
    fputs(utf8, stdout);
    return;
  }

  fputc((unsigned char)(ch ? ch : ' '), stdout);
}

void cmd_display_console_emit_attr(cmd_display_attr_t attr)
{
  int fg = cmd_display_console_fg_code(cmd_display_attr_fg(attr));
  int bg = cmd_display_console_bg_code(cmd_display_attr_bg(attr));
  uint8_t flags = cmd_display_attr_flags(attr);

  printf("\x1b[0");

  if (flags & CMD_DISPLAY_ATTR_FLAG_BOLD)
    printf(";1");

  if (flags & CMD_DISPLAY_ATTR_FLAG_UNDERLINE)
    printf(";4");

  if (flags & CMD_DISPLAY_ATTR_FLAG_INVERSE)
    printf(";7");

  printf(";%d;%dm", fg, bg);
}

bool cmd_display_cell_equal(const CmdDisplayCell *a, const CmdDisplayCell *b)
{
  if (!a || !b) return false;
  return a->ch == b->ch && a->attr == b->attr;
}

size_t cmd_display_console_decimal_len(int value)
{
  size_t len = 1;

  while (value >= 10)
  {
    value /= 10;
    len++;
  }

  return len;
}

size_t cmd_display_console_attr_bytes(cmd_display_attr_t attr)
{
  uint8_t flags = cmd_display_attr_flags(attr);
  size_t len = 6;

  len += cmd_display_console_decimal_len(cmd_display_console_fg_code(cmd_display_attr_fg(attr)));
  len += cmd_display_console_decimal_len(cmd_display_console_bg_code(cmd_display_attr_bg(attr)));

  if (flags & CMD_DISPLAY_ATTR_FLAG_BOLD) len += 2;
  if (flags & CMD_DISPLAY_ATTR_FLAG_UNDERLINE) len += 2;
  if (flags & CMD_DISPLAY_ATTR_FLAG_INVERSE) len += 2;

  return len;
}

void cmd_display_console_count_range(const CmdDisplayBuffer *buffer, int y, int x0, int x1, CmdDisplayConsoleEmitState *state, size_t *bytes)
{
  if (!buffer || !state || !bytes) return;

  if (!state->pos_valid || state->x != x0 || state->y != y)
  {
    *bytes += 4 + cmd_display_console_decimal_len(y + 1) + cmd_display_console_decimal_len(x0 + 1);
    state->x = x0;
    state->y = y;
    state->pos_valid = true;
  }

  for (int x = x0; x < x1; x++)
  {
    const CmdDisplayCell *cell = &buffer->cells[y][x];

    if (!state->attr_valid || cell->attr != state->attr)
    {
      *bytes += cmd_display_console_attr_bytes(cell->attr);
      state->attr = cell->attr;
      state->attr_valid = true;
    }

    *bytes += (cmd_display_console_utf8_char(cell->ch) ? 3 : 1);
    state->x++;
    if (state->width > 0 && state->x >= state->width)
      state->pos_valid = false;
  }
}

size_t cmd_display_console_count_full_present(const CmdDisplayBuffer *buffer)
{
  CmdDisplayConsoleEmitState state;
  size_t bytes = 11;

  if (!buffer) return 0;

  state.attr = 0;
  state.x = 0;
  state.y = 0;
  state.width = buffer->size.w;
  state.attr_valid = false;
  state.pos_valid = true;

  for (int y = 0; y < buffer->size.h; y++)
  {
    cmd_display_console_count_range(buffer, y, 0, buffer->size.w, &state, &bytes);

    if (y + 1 < buffer->size.h)
    {
      bytes += 2;
      state.x = 0;
      state.y = y + 1;
      state.pos_valid = true;
    }
  }

  bytes += 4;
  return bytes;
}

size_t cmd_display_console_count_partial_present(const CmdDisplayBuffer *buffer, const CmdDisplayBuffer *prev, bool *any_dirty)
{
  CmdDisplayConsoleEmitState state;
  size_t bytes = 0;

  if (any_dirty) *any_dirty = false;
  if (!buffer || !prev) return 0;

  state.attr = 0;
  state.x = -1;
  state.y = -1;
  state.width = buffer->size.w;
  state.attr_valid = false;
  state.pos_valid = false;

  for (int y = 0; y < buffer->size.h; y++)
  {
    int x = 0;

    while (x < buffer->size.w)
    {
      while (x < buffer->size.w && cmd_display_cell_equal(&buffer->cells[y][x], &prev->cells[y][x]))
        x++;

      if (x >= buffer->size.w)
        break;

      int x0 = x;

      while (x < buffer->size.w && !cmd_display_cell_equal(&buffer->cells[y][x], &prev->cells[y][x]))
        x++;

      cmd_display_console_count_range(buffer, y, x0, x, &state, &bytes);
      if (any_dirty) *any_dirty = true;
    }
  }

  if (any_dirty && *any_dirty)
    bytes += 4;

  return bytes;
}

void cmd_display_console_store_prev(CmdDisplayConsoleContext *ctx, const CmdDisplayBuffer *buffer)
{
  if (!ctx || !buffer) return;
  ctx->prev = *buffer;
  ctx->prev_valid = true;
}

void cmd_display_console_emit_range(const CmdDisplayBuffer *buffer, int y, int x0, int x1, CmdDisplayConsoleEmitState *state)
{
  if (!state || !state->pos_valid || state->x != x0 || state->y != y)
  {
    printf("\x1b[%d;%dH", y + 1, x0 + 1);

    if (state)
    {
      state->x = x0;
      state->y = y;
      state->pos_valid = true;
    }
  }

  for (int x = x0; x < x1; x++)
  {
    const CmdDisplayCell *cell = &buffer->cells[y][x];

    if (!state || !state->attr_valid || cell->attr != state->attr)
    {
      cmd_display_console_emit_attr(cell->attr);

      if (state)
      {
        state->attr = cell->attr;
        state->attr_valid = true;
      }
    }

    cmd_display_console_emit_char(cell->ch);

    if (state && state->pos_valid)
    {
      state->x++;
      if (state->width > 0 && state->x >= state->width)
        state->pos_valid = false;
    }
  }
}

esp_err_t cmd_display_console_full_present(CmdDisplayConsoleContext *ctx, const CmdDisplayBuffer *buffer)
{
  CmdDisplayConsoleEmitState state =
  {
    .attr = 0,
    .x = 0,
    .y = 0,
    .width = buffer->size.w,
    .attr_valid = false,
    .pos_valid = true
  };

  printf("\x1b[0m\x1b[2J\x1b[H");

  for (int y = 0; y < buffer->size.h; y++)
  {
    cmd_display_console_emit_range(buffer, y, 0, buffer->size.w, &state);

    if (y + 1 < buffer->size.h)
    {
      printf("\r\n");
      state.x = 0;
      state.y = y + 1;
      state.pos_valid = true;
    }
  }

  printf("\x1b[0m");
  fflush(stdout);
  cmd_display_console_store_prev(ctx, buffer);
  return ESP_OK;
}

bool cmd_display_console_parse_size_response(const char *seq, int *rows, int *cols)
{
  int first = 0;
  int second = 0;
  int third = 0;
  size_t i = 0;

  if (!seq || !rows || !cols) return false;

  while (seq[i] && seq[i] != '[')
    i++;

  if (seq[i] != '[') return false;
  i++;

  while (seq[i] >= '0' && seq[i] <= '9')
  {
    first = first * 10 + (seq[i] - '0');
    i++;
  }

  if (seq[i] != ';') return false;
  i++;

  while (seq[i] >= '0' && seq[i] <= '9')
  {
    second = second * 10 + (seq[i] - '0');
    i++;
  }

  if (seq[i] == 'R')
  {
    if (first <= 0 || second <= 0) return false;
    *rows = first;
    *cols = second;
    return true;
  }

  if (seq[i] != ';') return false;
  i++;

  while (seq[i] >= '0' && seq[i] <= '9')
  {
    third = third * 10 + (seq[i] - '0');
    i++;
  }

  if (seq[i] != 't') return false;
  if (first != 8 || second <= 0 || third <= 0) return false;

  *rows = second;
  *cols = third;
  return true;
}

bool cmd_display_console_read_ansi_response(char *out, size_t out_size, uint32_t timeout_ms)
{
  size_t len = 0;
  uint8_t c;
  TickType_t ticks;

  if (!out || out_size == 0) return false;

  out[0] = 0;
  ticks = pdMS_TO_TICKS(timeout_ms);

  while (len + 1 < out_size)
  {
    int n = uart_read_bytes((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &c, 1, ticks);
    if (n <= 0) break;

    out[len++] = (char)c;
    out[len] = 0;

    if (c == 'R' || c == 't')
      return true;
  }

  return false;
}

bool cmd_display_console_query_size(cmd_size_t *size)
{
  char seq[48];
  int rows;
  int cols;

  if (!size) return false;

  printf("\x1b[18t");
  fflush(stdout);
  if (cmd_display_console_read_ansi_response(seq, sizeof(seq), CMD_DISPLAY_TERMINAL_QUERY_TIMEOUT_MS) &&
      cmd_display_console_parse_size_response(seq, &rows, &cols))
  {
    size->w = cols;
    size->h = rows;
    return true;
  }

  printf("\x1b[s\x1b[999;999H\x1b[6n");
  fflush(stdout);
  if (cmd_display_console_read_ansi_response(seq, sizeof(seq), CMD_DISPLAY_TERMINAL_QUERY_TIMEOUT_MS) &&
      cmd_display_console_parse_size_response(seq, &rows, &cols))
  {
    printf("\x1b[u");
    fflush(stdout);
    size->w = cols;
    size->h = rows;
    return true;
  }

  printf("\x1b[u");
  fflush(stdout);
  return false;
}

esp_err_t cmd_display_console_get_size(CmdDisplayBackend *backend, cmd_size_t *size)
{
  if (!backend || !size) return ESP_ERR_INVALID_ARG;

  size->w = CMD_DISPLAY_DEFAULT_COLS;
  size->h = CMD_DISPLAY_DEFAULT_ROWS;

  if (!cmd_display_console_query_size(size))
    return ESP_OK;

  if (size->w > CMD_DISPLAY_COLS) size->w = CMD_DISPLAY_COLS;
  if (size->h > CMD_DISPLAY_ROWS) size->h = CMD_DISPLAY_ROWS;
  if (size->w <= 0) size->w = CMD_DISPLAY_DEFAULT_COLS;
  if (size->h <= 0) size->h = CMD_DISPLAY_DEFAULT_ROWS;
  return ESP_OK;
}

esp_err_t cmd_display_console_init(CmdDisplayBackend *backend)
{
  if (!backend) return ESP_ERR_INVALID_ARG;

  memset(&cmd_display_console_ctx, 0, sizeof(cmd_display_console_ctx));
  backend->ctx = &cmd_display_console_ctx;

  printf("\x1b[0m\x1b[2J\x1b[H");
  fflush(stdout);
  return ESP_OK;
}

void cmd_display_console_deinit(CmdDisplayBackend *backend)
{
  if (!backend) return;
  backend->ctx = NULL;
  memset(&cmd_display_console_ctx, 0, sizeof(cmd_display_console_ctx));
}

esp_err_t cmd_display_console_clear(CmdDisplayBackend *backend)
{
  CmdDisplayConsoleContext *ctx;

  if (!backend) return ESP_ERR_INVALID_ARG;

  ctx = (CmdDisplayConsoleContext *)backend->ctx;
  if (ctx)
    ctx->prev_valid = false;

  printf("\x1b[0m\x1b[2J\x1b[H");
  fflush(stdout);
  return ESP_OK;
}

esp_err_t cmd_display_console_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer)
{
  CmdDisplayConsoleContext *ctx;
  CmdDisplayConsoleEmitState state =
  {
    .attr = 0,
    .x = -1,
    .y = -1,
    .width = buffer ? buffer->size.w : 0,
    .attr_valid = false,
    .pos_valid = false
  };
  size_t full_bytes;
  size_t partial_bytes;
  bool any_dirty = false;

  if (!backend || !buffer) return ESP_ERR_INVALID_ARG;

  ctx = (CmdDisplayConsoleContext *)backend->ctx;
  if (!ctx) return ESP_ERR_INVALID_STATE;

  if (!ctx->prev_valid || ctx->prev.size.w != buffer->size.w || ctx->prev.size.h != buffer->size.h)
    return cmd_display_console_full_present(ctx, buffer);

  partial_bytes = cmd_display_console_count_partial_present(buffer, &ctx->prev, &any_dirty);

  if (!any_dirty)
  {
    cmd_display_console_store_prev(ctx, buffer);
    return ESP_OK;
  }

  if (!(backend->flags & CMD_DISPLAY_BACKEND_FLAG_FORCE_PARTIAL))
  {
    full_bytes = cmd_display_console_count_full_present(buffer);
    if (full_bytes <= partial_bytes)
      return cmd_display_console_full_present(ctx, buffer);
  }

  for (int y = 0; y < buffer->size.h; y++)
  {
    int x = 0;

    while (x < buffer->size.w)
    {
      while (x < buffer->size.w && cmd_display_cell_equal(&buffer->cells[y][x], &ctx->prev.cells[y][x]))
        x++;

      if (x >= buffer->size.w)
        break;

      int x0 = x;

      while (x < buffer->size.w && !cmd_display_cell_equal(&buffer->cells[y][x], &ctx->prev.cells[y][x]))
        x++;

      cmd_display_console_emit_range(buffer, y, x0, x, &state);
    }
  }

  printf("\x1b[0m");
  fflush(stdout);

  cmd_display_console_store_prev(ctx, buffer);
  return ESP_OK;
}

esp_err_t cmd_display_console_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr)
{
  CmdDisplayConsoleContext *ctx;

  if (!backend) return ESP_ERR_INVALID_ARG;
  if (!cmd_display_point_inside(x, y)) return ESP_ERR_INVALID_ARG;

  if (ch == 0)
    ch = ' ';

  printf("\x1b[%d;%dH", y + 1, x + 1);
  cmd_display_console_emit_attr(attr);
  cmd_display_console_emit_char(ch);
  printf("\x1b[0m");
  fflush(stdout);

  ctx = (CmdDisplayConsoleContext *)backend->ctx;
  if (ctx && ctx->prev_valid)
  {
    ctx->prev.cells[y][x].ch = ch;
    ctx->prev.cells[y][x].attr = attr;
  }

  return ESP_OK;
}

esp_err_t cmd_display_console_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr)
{
  CmdDisplayConsoleContext *ctx;
  int pos = x;

  if (!backend || !text) return ESP_ERR_INVALID_ARG;
  if (y < 0 || y >= CMD_DISPLAY_ROWS) return ESP_ERR_INVALID_ARG;
  if (pos >= CMD_DISPLAY_COLS) return ESP_OK;
  if (pos < 0) pos = 0;

  printf("\x1b[%d;%dH", y + 1, pos + 1);
  cmd_display_console_emit_attr(attr);

  ctx = (CmdDisplayConsoleContext *)backend->ctx;

  while (*text && pos < CMD_DISPLAY_COLS)
  {
    cmd_display_console_emit_char(*text);

    if (ctx && ctx->prev_valid)
    {
      ctx->prev.cells[y][pos].ch = *text;
      ctx->prev.cells[y][pos].attr = attr;
    }

    text++;
    pos++;
  }

  printf("\x1b[0m");
  fflush(stdout);
  return ESP_OK;
}

uint8_t cmd_display_ft812_color(cmd_display_color_t color)
{
  uint8_t c = (uint8_t)color & 0x0f;
  uint8_t out = 0;

  if (c & 0x01) out |= 0x01;
  if (c & 0x02) out |= 0x04;
  if (c & 0x04) out |= 0x02;
  if (c & 0x08) out |= 0x08;

  return out;
}

uint8_t cmd_display_ft812_attr(cmd_display_attr_t attr)
{
  uint8_t fg = cmd_display_ft812_color(cmd_display_attr_fg(attr));
  uint8_t bg = cmd_display_ft812_color(cmd_display_attr_bg(attr));
  uint8_t flags = cmd_display_attr_flags(attr);

  if (flags & CMD_DISPLAY_ATTR_FLAG_BOLD)
    fg |= 0x08;

  if (flags & CMD_DISPLAY_ATTR_FLAG_INVERSE)
  {
    uint8_t tmp = fg;
    fg = bg;
    bg = tmp;
  }

  return (uint8_t)((bg << 4) | fg);
}

CmdDisplayFt812Context *cmd_display_ft812_context(CmdDisplayBackend *backend)
{
  if (!backend) return NULL;
  return (CmdDisplayFt812Context *)backend->ctx;
}

void cmd_display_ft812_fill_local(CmdDisplayFt812Context *ctx, char ch, cmd_display_attr_t attr)
{
  uint8_t glyph = ch ? (uint8_t)ch : (uint8_t)' ';
  uint8_t ft_attr = cmd_display_ft812_attr(attr);

  if (!ctx) return;

  memset(ctx->chars, glyph, sizeof(ctx->chars));
  memset(ctx->attrs, ft_attr, sizeof(ctx->attrs));
}

void cmd_display_ft812_pack(CmdDisplayFt812Context *ctx, const CmdDisplayBuffer *buffer)
{
  uint16_t cols;
  uint16_t rows;

  if (!ctx || !buffer) return;

  ft_text_attr_mode_get_size(&cols, &rows);
  cmd_display_ft812_fill_local(ctx, ' ', CMD_DISPLAY_ATTR_DEFAULT);

  for (int y = 0; y < buffer->size.h && y < (int)rows; y++)
  {
    for (int x = 0; x < buffer->size.w && x < (int)cols; x++)
    {
      const CmdDisplayCell *cell = &buffer->cells[y][x];
      size_t off = (size_t)y * (size_t)cols + (size_t)x;

      ctx->chars[off] = cell->ch ? (uint8_t)cell->ch : (uint8_t)' ';
      ctx->attrs[off] = cmd_display_ft812_attr(cell->attr);
    }
  }
}

esp_err_t cmd_display_ft812_get_size(CmdDisplayBackend *backend, cmd_size_t *size)
{
  uint16_t cols;
  uint16_t rows;

  if (!backend || !size) return ESP_ERR_INVALID_ARG;

  ft_text_attr_mode_get_size(&cols, &rows);
  size->w = (int)cols;
  size->h = (int)rows;

  if (size->w <= 0) size->w = CMD_DISPLAY_DEFAULT_COLS;
  if (size->h <= 0) size->h = CMD_DISPLAY_DEFAULT_ROWS;
  if (size->w > CMD_DISPLAY_COLS) size->w = CMD_DISPLAY_COLS;
  if (size->h > CMD_DISPLAY_ROWS) size->h = CMD_DISPLAY_ROWS;
  return ESP_OK;
}

esp_err_t cmd_display_ft812_begin(CmdDisplayBackend *backend)
{
  esp_err_t err;

  if (!backend) return ESP_ERR_INVALID_ARG;

  memset(&cmd_display_ft812_ctx, 0, sizeof(cmd_display_ft812_ctx));
  cmd_display_ft812_fill_local(&cmd_display_ft812_ctx, ' ', CMD_DISPLAY_ATTR_DEFAULT);

  err = ft_text_attr_mode_begin();
  if (err != ESP_OK) return err;

  backend->ctx = &cmd_display_ft812_ctx;
  cmd_display_ft812_ctx.active = true;
  return ESP_OK;
}

void cmd_display_ft812_end(CmdDisplayBackend *backend)
{
  if (!backend) return;

  ft_text_attr_mode_end();
  backend->ctx = NULL;
  memset(&cmd_display_ft812_ctx, 0, sizeof(cmd_display_ft812_ctx));
}

esp_err_t cmd_display_ft812_clear(CmdDisplayBackend *backend)
{
  CmdDisplayFt812Context *ctx = cmd_display_ft812_context(backend);

  if (!ctx || !ctx->active) return ESP_ERR_INVALID_STATE;

  cmd_display_ft812_fill_local(ctx, ' ', CMD_DISPLAY_ATTR_DEFAULT);
  return ft_text_attr_mode_clear(' ', cmd_display_ft812_attr(CMD_DISPLAY_ATTR_DEFAULT));
}

esp_err_t cmd_display_ft812_present(CmdDisplayBackend *backend, const CmdDisplayBuffer *buffer)
{
  CmdDisplayFt812Context *ctx = cmd_display_ft812_context(backend);

  if (!ctx || !ctx->active) return ESP_ERR_INVALID_STATE;
  if (!buffer) return ESP_ERR_INVALID_ARG;

  cmd_display_ft812_pack(ctx, buffer);
  return ft_text_attr_mode_present(ctx->chars, ctx->attrs);
}

esp_err_t cmd_display_ft812_put_char(CmdDisplayBackend *backend, int x, int y, char ch, cmd_display_attr_t attr)
{
  CmdDisplayFt812Context *ctx = cmd_display_ft812_context(backend);
  uint16_t cols;
  uint16_t rows;
  size_t off;

  if (!ctx || !ctx->active) return ESP_ERR_INVALID_STATE;

  ft_text_attr_mode_get_size(&cols, &rows);
  if (x < 0 || y < 0 || x >= (int)cols || y >= (int)rows) return ESP_ERR_INVALID_ARG;

  off = (size_t)y * (size_t)cols + (size_t)x;
  ctx->chars[off] = ch ? (uint8_t)ch : (uint8_t)' ';
  ctx->attrs[off] = cmd_display_ft812_attr(attr);

  return ft_text_attr_mode_present(ctx->chars, ctx->attrs);
}

esp_err_t cmd_display_ft812_write_text(CmdDisplayBackend *backend, int x, int y, const char *text, cmd_display_attr_t attr)
{
  CmdDisplayFt812Context *ctx = cmd_display_ft812_context(backend);
  uint16_t cols;
  uint16_t rows;
  uint8_t ft_attr = cmd_display_ft812_attr(attr);
  int pos = x;

  if (!ctx || !ctx->active) return ESP_ERR_INVALID_STATE;
  if (!text) return ESP_ERR_INVALID_ARG;

  ft_text_attr_mode_get_size(&cols, &rows);
  if (y < 0 || y >= (int)rows) return ESP_ERR_INVALID_ARG;
  if (pos >= (int)cols) return ESP_OK;
  if (pos < 0) pos = 0;

  while (*text && pos < (int)cols)
  {
    size_t off = (size_t)y * (size_t)cols + (size_t)pos;
    ctx->chars[off] = (uint8_t)*text;
    ctx->attrs[off] = ft_attr;
    text++;
    pos++;
  }

  return ft_text_attr_mode_present(ctx->chars, ctx->attrs);
}

// --------------- cmd_fileops.cpp ---------------

typedef struct
{
  CmdFsList lists[CMD_FILEOPS_MAX_DEPTH];
  cmd_file_entry_t entries[CMD_FILEOPS_MAX_DEPTH];
  char src_paths[CMD_FILEOPS_MAX_DEPTH + 1][CMD_PATH_MAX];
  char dst_paths[CMD_FILEOPS_MAX_DEPTH + 1][CMD_PATH_MAX];
  char scratch_path[CMD_PATH_MAX];
  char scratch_src_path[CMD_PATH_MAX];
  char scratch_dst_path[CMD_PATH_MAX];
  uint8_t copy_buffer[CMD_FILEOPS_BUFFER_SIZE];
} CmdFileopsWorkspace;

CmdFileopsWorkspace *g_cmd_fileops_workspace;

esp_err_t cmd_fileops_alloc_global_buffers()
{
  if (g_cmd_fileops_workspace) return ESP_OK;

  g_cmd_fileops_workspace = (CmdFileopsWorkspace *)heap_caps_calloc(
    1,
    sizeof(CmdFileopsWorkspace),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!g_cmd_fileops_workspace) return ESP_ERR_NO_MEM;
  return ESP_OK;
}

void cmd_fileops_free_global_buffers()
{
  if (!g_cmd_fileops_workspace) return;

  heap_caps_free(g_cmd_fileops_workspace);
  g_cmd_fileops_workspace = NULL;
}

CmdFileopsWorkspace *cmd_fileops_workspace()
{
  return g_cmd_fileops_workspace;
}

esp_err_t cmd_fileops_copy_path_to_slot(char *dst, size_t dst_size, const char *src)
{
  size_t len;

  if (!dst || dst_size == 0 || !src) return ESP_ERR_INVALID_ARG;
  if (dst == src) return ESP_OK;

  len = strlen(src);
  if (len >= dst_size) return CMD_ERR_PATH_TOO_LONG;

  memmove(dst, src, len + 1);
  return ESP_OK;
}

esp_err_t cmd_fileops_ensure_device(CmdFsManager *fs, cmd_device_id_t device_id)
{
  if (!fs) return ESP_ERR_INVALID_ARG;
  if (!cmd_fs_is_valid_device_id(device_id)) return CMD_ERR_INVALID_DEVICE;
  return cmd_fs_manager_mount(fs, device_id);
}

bool cmd_fileops_should_overwrite(const CmdFileopsOptions *options)
{
  if (!options) return false;
  return (options->flags & CMD_FILEOPS_FLAG_OVERWRITE) != 0;
}

bool cmd_fileops_should_recurse(const CmdFileopsOptions *options)
{
  if (!options) return false;
  return (options->flags & CMD_FILEOPS_FLAG_RECURSIVE) != 0;
}

bool cmd_fileops_same_location(cmd_device_id_t a_device,
  const char *a_path,
  cmd_device_id_t b_device,
  const char *b_path)
{
  if (a_device != b_device) return false;
  if (!a_path || !b_path) return false;
  return strcmp(a_path, b_path) == 0;
}

size_t cmd_fileops_trim_path_len(const char *path)
{
  size_t len;

  if (!path || !path[0]) return 1;

  len = strlen(path);
  while (len > 1 && path[len - 1] == '/')
    len--;

  return len;
}

bool cmd_fileops_path_contains(const char *parent, const char *child)
{
  size_t parent_len;
  size_t child_len;

  if (!parent || !child) return false;

  parent_len = cmd_fileops_trim_path_len(parent);
  child_len = cmd_fileops_trim_path_len(child);

  if (parent_len == 1 && parent[0] == '/') return child_len >= 1 && child[0] == '/';
  if (child_len < parent_len) return false;
  if (strncmp(parent, child, parent_len) != 0) return false;
  if (child_len == parent_len) return true;
  return child[parent_len] == '/';
}

esp_err_t cmd_fileops_check_dst_available(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const CmdFileopsOptions *options,
  bool *out_existed)
{
  cmd_file_entry_t entry;
  esp_err_t err;

  if (out_existed) *out_existed = false;
  if (!fs || !path) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_stat(fs, device_id, path, &entry);
  if (err == ESP_ERR_NOT_FOUND) return ESP_OK;
  if (err != ESP_OK) return err;

  if (out_existed) *out_existed = true;
  if (cmd_fileops_should_overwrite(options)) return ESP_OK;
  return ESP_ERR_INVALID_STATE;
}

esp_err_t cmd_fileops_prepare_dst_dir(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const CmdFileopsOptions *options)
{
  cmd_file_entry_t entry;
  esp_err_t err;

  if (!fs || !path) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_stat(fs, device_id, path, &entry);
  if (err == ESP_ERR_NOT_FOUND) return cmd_fs_manager_mkdir(fs, device_id, path);
  if (err != ESP_OK) return err;
  if (entry.type != CMD_ENTRY_DIR) return ESP_ERR_INVALID_STATE;
  if (!cmd_fileops_should_overwrite(options)) return ESP_ERR_INVALID_STATE;
  return ESP_OK;
}

bool cmd_fileops_progress_call_ex(const CmdFileopsOptions *options,
  cmd_fileops_op_t op,
  const char *filename,
  uint64_t bytes_done,
  uint64_t bytes_total,
  size_t files_done,
  size_t files_total,
  bool can_cancel)
{
  CmdFileopsProgress progress;

  if (!options || !options->progress) return true;

  progress.op = op;
  progress.filename = filename;
  progress.bytes_done = bytes_done;
  progress.bytes_total = bytes_total;
  progress.files_done = files_done;
  progress.files_total = files_total;
  progress.can_cancel = can_cancel;

  if (options->progress(&progress, options->progress_ctx)) return true;
  return !can_cancel;
}

typedef struct
{
  const CmdFileopsOptions *options;
  cmd_fileops_op_t op;
  uint64_t bytes_done;
  uint64_t bytes_total;
  size_t files_done;
  size_t files_total;
} CmdFileopsTreeCtx;

esp_err_t cmd_fileops_prepare_tree_context(CmdFileopsTreeCtx *ctx,
  const CmdFileopsOptions *options,
  cmd_fileops_op_t op,
  uint64_t bytes_total,
  size_t files_total)
{
  if (!ctx) return ESP_ERR_INVALID_ARG;

  memset(ctx, 0, sizeof(*ctx));
  ctx->options = options;
  ctx->op = op;
  ctx->bytes_total = bytes_total;
  ctx->files_total = files_total;
  return ESP_OK;
}

bool cmd_fileops_tree_progress(CmdFileopsTreeCtx *ctx, const char *filename, bool can_cancel)
{
  if (!ctx) return true;

  return cmd_fileops_progress_call_ex(ctx->options,
    ctx->op,
    filename,
    ctx->bytes_done,
    ctx->bytes_total,
    ctx->files_done,
    ctx->files_total,
    can_cancel);
}

esp_err_t cmd_fileops_close_keep_first(CmdFsManager *fs, CmdFsFile *file, bool *opened, esp_err_t first_err)
{
  esp_err_t close_err;

  if (!opened || !*opened) return first_err;

  close_err = cmd_fs_manager_close(fs, file);
  *opened = false;
  if (first_err != ESP_OK) return first_err;
  return close_err;
}

esp_err_t cmd_fileops_join_selected_path(const CmdPanel *panel,
  const cmd_file_entry_t *entry,
  char *out,
  size_t out_size)
{
  if (!panel || !entry || !out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (entry->type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;
  return cmd_fs_join_path(panel->current_path, entry->name, out, out_size);
}

bool cmd_fileops_entry_is_marked(const cmd_file_entry_t *entry);

esp_err_t cmd_fileops_scan_tree(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  size_t depth,
  uint64_t *bytes_total,
  size_t *files_total)
{
  CmdFileopsWorkspace *workspace;
  CmdFsList *list;
  cmd_file_entry_t *entry;
  char *child_path;
  bool list_open = false;
  esp_err_t err;

  if (!fs || !path || !bytes_total || !files_total) return ESP_ERR_INVALID_ARG;
  if (depth >= CMD_FILEOPS_MAX_DEPTH) return ESP_ERR_INVALID_SIZE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  list = &workspace->lists[depth];
  entry = &workspace->entries[depth];
  child_path = workspace->src_paths[depth + 1];
  memset(list, 0, sizeof(*list));

  err = cmd_fs_manager_list_begin(fs, device_id, path, list);
  if (err != ESP_OK) return err;
  list_open = true;

  for (;;)
  {
    err = cmd_fs_manager_list_next(fs, list, entry);
    if (err == ESP_ERR_NOT_FOUND)
    {
      err = ESP_OK;
      break;
    }
    if (err != ESP_OK) break;
    if (entry->type == CMD_ENTRY_PARENT) continue;

    err = cmd_fs_join_path(path, entry->name, child_path, CMD_PATH_MAX);
    if (err != ESP_OK) break;

    if (entry->type == CMD_ENTRY_FILE)
    {
      *bytes_total += entry->size;
      (*files_total)++;
    }
    else if (entry->type == CMD_ENTRY_DIR)
    {
      err = cmd_fileops_scan_tree(fs, device_id, child_path, depth + 1, bytes_total, files_total);
      if (err != ESP_OK) break;
    }
    else
    {
      err = ESP_ERR_NOT_SUPPORTED;
      break;
    }
  }

  if (list_open)
    cmd_fs_manager_list_close(fs, list);

  return err;
}

esp_err_t cmd_fileops_delete_file_with_progress(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const cmd_file_entry_t *entry,
  const CmdFileopsOptions *options,
  uint64_t *bytes_done,
  uint64_t bytes_total,
  size_t *files_done,
  size_t files_total,
  bool can_cancel)
{
  uint64_t size = entry ? entry->size : 0;
  const char *filename;
  esp_err_t err;

  if (!fs || !path || !bytes_done || !files_done) return ESP_ERR_INVALID_ARG;

  filename = cmd_fs_filename_ptr(path);
  if (!cmd_fileops_progress_call_ex(options,
    CMD_FILEOPS_OP_DELETE,
    filename,
    *bytes_done,
    bytes_total,
    *files_done,
    files_total,
    can_cancel)) return CMD_ERR_CANCELLED;

  err = cmd_fs_manager_remove(fs, device_id, path);
  if (err != ESP_OK) return err;

  *bytes_done += size;
  (*files_done)++;

  if (!cmd_fileops_progress_call_ex(options,
    CMD_FILEOPS_OP_DELETE,
    filename,
    *bytes_done,
    bytes_total,
    *files_done,
    files_total,
    can_cancel)) return CMD_ERR_CANCELLED;

  return ESP_OK;
}

esp_err_t cmd_fileops_delete_dir_recursive(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const CmdFileopsOptions *options,
  uint64_t *bytes_done,
  uint64_t bytes_total,
  size_t *files_done,
  size_t files_total,
  size_t depth,
  bool can_cancel)
{
  CmdFileopsWorkspace *workspace;
  CmdFsList *list;
  cmd_file_entry_t *entry;
  char *child_path;
  bool list_open = false;
  const char *filename;
  esp_err_t err;

  if (!fs || !path || !bytes_done || !files_done) return ESP_ERR_INVALID_ARG;
  if (depth >= CMD_FILEOPS_MAX_DEPTH) return ESP_ERR_INVALID_SIZE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  list = &workspace->lists[depth];
  entry = &workspace->entries[depth];
  child_path = workspace->src_paths[depth + 1];
  memset(list, 0, sizeof(*list));

  filename = cmd_fs_filename_ptr(path);
  if (!cmd_fileops_progress_call_ex(options,
    CMD_FILEOPS_OP_DELETE,
    filename,
    *bytes_done,
    bytes_total,
    *files_done,
    files_total,
    can_cancel)) return CMD_ERR_CANCELLED;

  err = cmd_fs_manager_list_begin(fs, device_id, path, list);
  if (err != ESP_OK) return err;
  list_open = true;

  for (;;)
  {
    err = cmd_fs_manager_list_next(fs, list, entry);
    if (err == ESP_ERR_NOT_FOUND)
    {
      err = ESP_OK;
      break;
    }
    if (err != ESP_OK) break;
    if (entry->type == CMD_ENTRY_PARENT) continue;

    err = cmd_fs_join_path(path, entry->name, child_path, CMD_PATH_MAX);
    if (err != ESP_OK) break;

    if (entry->type == CMD_ENTRY_FILE)
    {
      err = cmd_fileops_delete_file_with_progress(fs,
        device_id,
        child_path,
        entry,
        options,
        bytes_done,
        bytes_total,
        files_done,
        files_total,
        can_cancel);
      if (err != ESP_OK) break;
    }
    else if (entry->type == CMD_ENTRY_DIR)
    {
      err = cmd_fileops_delete_dir_recursive(fs,
        device_id,
        child_path,
        options,
        bytes_done,
        bytes_total,
        files_done,
        files_total,
        depth + 1,
        can_cancel);
      if (err != ESP_OK) break;
    }
    else
    {
      err = ESP_ERR_NOT_SUPPORTED;
      break;
    }
  }

  if (list_open)
    cmd_fs_manager_list_close(fs, list);

  if (err != ESP_OK) return err;

  err = cmd_fs_manager_remove(fs, device_id, path);
  if (err != ESP_OK) return err;

  if (!cmd_fileops_progress_call_ex(options,
    CMD_FILEOPS_OP_DELETE,
    filename,
    *bytes_done,
    bytes_total,
    *files_done,
    files_total,
    can_cancel)) return CMD_ERR_CANCELLED;

  return ESP_OK;
}

esp_err_t cmd_fileops_count_selected_tree_bytes(CmdPanel *panel, uint64_t *out_bytes_total)
{
  CmdFileopsWorkspace *workspace;
  uint64_t bytes_total = 0;
  size_t files_total = 0;
  char *path;
  esp_err_t err;

  if (!panel || !out_bytes_total) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  path = workspace->scratch_path;

  err = cmd_fileops_ensure_device(panel->fs, panel->device_id);
  if (err != ESP_OK) return err;

  for (size_t i = 0; i < panel->count; i++)
  {
    const cmd_file_entry_t *entry = &panel->entries[i];

    if (!cmd_fileops_entry_is_marked(entry)) continue;

    if (entry->type == CMD_ENTRY_FILE)
    {
      bytes_total += entry->size;
      continue;
    }

    if (entry->type != CMD_ENTRY_DIR) return ESP_ERR_NOT_SUPPORTED;

    err = cmd_fileops_join_selected_path(panel, entry, path, CMD_PATH_MAX);
    if (err != ESP_OK) return err;

    err = cmd_fileops_scan_tree(panel->fs,
      panel->device_id,
      path,
      0,
      &bytes_total,
      &files_total);
    if (err != ESP_OK) return err;
  }

  *out_bytes_total = bytes_total;
  return ESP_OK;
}

esp_err_t cmd_fileops_copy_file_with_context(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  const cmd_file_entry_t *src_entry,
  cmd_device_id_t dst_device,
  const char *dst_path,
  CmdFileopsTreeCtx *ctx)
{
  CmdFileopsWorkspace *workspace;
  CmdFsFile src_file = {};
  CmdFsFile dst_file = {};
  cmd_file_entry_t entry;
  const CmdFileopsOptions *options;
  const char *filename;
  bool src_open = false;
  bool dst_open = false;
  bool dst_existed = false;
  bool dst_created = false;
  uint64_t file_done = 0;
  uint64_t file_total;
  esp_err_t err;

  if (!fs || !src_path || !dst_path || !ctx) return ESP_ERR_INVALID_ARG;
  if (cmd_fileops_same_location(src_device, src_path, dst_device, dst_path)) return ESP_ERR_INVALID_STATE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  options = ctx->options;

  if (!src_entry)
  {
    err = cmd_fs_manager_stat(fs, src_device, src_path, &entry);
    if (err != ESP_OK) return err;
    src_entry = &entry;
  }

  if (src_entry->type != CMD_ENTRY_FILE) return ESP_ERR_NOT_SUPPORTED;

  err = cmd_fileops_check_dst_available(fs, dst_device, dst_path, options, &dst_existed);
  if (err != ESP_OK) return err;

  file_total = src_entry->size;
  filename = cmd_fs_filename_ptr(src_path);

  if (!cmd_fileops_tree_progress(ctx, filename, true)) return CMD_ERR_CANCELLED;

  err = cmd_fs_manager_open_read(fs, src_device, src_path, &src_file);
  if (err != ESP_OK) goto done;
  src_open = true;

  err = cmd_fs_manager_open_write(fs, dst_device, dst_path, &dst_file);
  if (err != ESP_OK) goto done;
  dst_open = true;
  dst_created = !dst_existed;

  while (file_done < file_total)
  {
    uint64_t left = file_total - file_done;
    size_t todo = left > CMD_FILEOPS_BUFFER_SIZE ? CMD_FILEOPS_BUFFER_SIZE : (size_t)left;
    size_t got = 0;
    size_t wrote = 0;

    err = cmd_fs_manager_read(fs, &src_file, workspace->copy_buffer, todo, &got);
    if (err != ESP_OK) goto done;
    if (got == 0)
    {
      err = ESP_FAIL;
      goto done;
    }

    err = cmd_fs_manager_write(fs, &dst_file, workspace->copy_buffer, got, &wrote);
    if (err != ESP_OK) goto done;
    if (wrote != got)
    {
      err = ESP_FAIL;
      goto done;
    }

    file_done += got;
    ctx->bytes_done += got;
    if (!cmd_fileops_tree_progress(ctx, filename, file_done < file_total))
    {
      err = CMD_ERR_CANCELLED;
      goto done;
    }
  }

  ctx->files_done++;
  if (!cmd_fileops_tree_progress(ctx, filename, false))
  {
    err = CMD_ERR_CANCELLED;
    goto done;
  }

  err = ESP_OK;

done:
  err = cmd_fileops_close_keep_first(fs, &dst_file, &dst_open, err);
  err = cmd_fileops_close_keep_first(fs, &src_file, &src_open, err);

  if (err != ESP_OK && dst_created)
    cmd_fs_manager_remove(fs, dst_device, dst_path);

  return err;
}

esp_err_t cmd_fileops_copy_dir_with_context(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  CmdFileopsTreeCtx *ctx,
  size_t depth)
{
  CmdFileopsWorkspace *workspace;
  CmdFsList *list;
  cmd_file_entry_t *entry;
  char *src_child;
  char *dst_child;
  bool list_open = false;
  esp_err_t err;

  if (!fs || !src_path || !dst_path || !ctx) return ESP_ERR_INVALID_ARG;
  if (depth >= CMD_FILEOPS_MAX_DEPTH) return ESP_ERR_INVALID_SIZE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  list = &workspace->lists[depth];
  entry = &workspace->entries[depth];
  src_child = workspace->src_paths[depth + 1];
  dst_child = workspace->dst_paths[depth + 1];
  memset(list, 0, sizeof(*list));

  err = cmd_fileops_prepare_dst_dir(fs, dst_device, dst_path, ctx->options);
  if (err != ESP_OK) return err;

  if (!cmd_fileops_tree_progress(ctx, cmd_fs_filename_ptr(src_path), true)) return CMD_ERR_CANCELLED;

  err = cmd_fs_manager_list_begin(fs, src_device, src_path, list);
  if (err != ESP_OK) return err;
  list_open = true;

  for (;;)
  {
    err = cmd_fs_manager_list_next(fs, list, entry);
    if (err == ESP_ERR_NOT_FOUND)
    {
      err = ESP_OK;
      break;
    }
    if (err != ESP_OK) break;
    if (entry->type == CMD_ENTRY_PARENT) continue;

    err = cmd_fs_join_path(src_path, entry->name, src_child, CMD_PATH_MAX);
    if (err != ESP_OK) break;

    err = cmd_fs_join_path(dst_path, entry->name, dst_child, CMD_PATH_MAX);
    if (err != ESP_OK) break;

    if (entry->type == CMD_ENTRY_FILE)
    {
      err = cmd_fileops_copy_file_with_context(fs,
        src_device,
        src_child,
        entry,
        dst_device,
        dst_child,
        ctx);
      if (err != ESP_OK) break;
    }
    else if (entry->type == CMD_ENTRY_DIR)
    {
      err = cmd_fileops_copy_dir_with_context(fs,
        src_device,
        src_child,
        dst_device,
        dst_child,
        ctx,
        depth + 1);
      if (err != ESP_OK) break;
    }
    else
    {
      err = ESP_ERR_NOT_SUPPORTED;
      break;
    }
  }

  if (list_open)
    cmd_fs_manager_list_close(fs, list);

  return err;
}

esp_err_t cmd_fileops_copy_path_as_op(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options,
  cmd_fileops_op_t op)
{
  CmdFileopsWorkspace *workspace;
  CmdFileopsTreeCtx ctx;
  cmd_file_entry_t src_entry;
  const char *src_root;
  const char *dst_root;
  uint64_t bytes_total = 0;
  size_t files_total = 0;
  esp_err_t err;

  if (!fs || !src_path || !dst_path) return ESP_ERR_INVALID_ARG;
  if (cmd_fileops_same_location(src_device, src_path, dst_device, dst_path)) return ESP_ERR_INVALID_STATE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  err = cmd_fileops_copy_path_to_slot(workspace->src_paths[0], CMD_PATH_MAX, src_path);
  if (err != ESP_OK) return err;

  err = cmd_fileops_copy_path_to_slot(workspace->dst_paths[0], CMD_PATH_MAX, dst_path);
  if (err != ESP_OK) return err;

  src_root = workspace->src_paths[0];
  dst_root = workspace->dst_paths[0];

  err = cmd_fileops_ensure_device(fs, src_device);
  if (err != ESP_OK) return err;

  err = cmd_fileops_ensure_device(fs, dst_device);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_stat(fs, src_device, src_root, &src_entry);
  if (err != ESP_OK) return err;

  if (src_entry.type == CMD_ENTRY_FILE)
  {
    err = cmd_fileops_prepare_tree_context(&ctx, options, op, src_entry.size, 1);
    if (err != ESP_OK) return err;

    return cmd_fileops_copy_file_with_context(fs,
      src_device,
      src_root,
      &src_entry,
      dst_device,
      dst_root,
      &ctx);
  }

  if (src_entry.type != CMD_ENTRY_DIR) return ESP_ERR_NOT_SUPPORTED;
  if (!cmd_fileops_should_recurse(options)) return ESP_ERR_NOT_SUPPORTED;
  if (src_device == dst_device && cmd_fileops_path_contains(src_root, dst_root))
    return ESP_ERR_INVALID_STATE;

  err = cmd_fileops_scan_tree(fs, src_device, src_root, 0, &bytes_total, &files_total);
  if (err != ESP_OK) return err;

  err = cmd_fileops_prepare_tree_context(&ctx, options, op, bytes_total, files_total);
  if (err != ESP_OK) return err;

  err = cmd_fileops_copy_dir_with_context(fs, src_device, src_root, dst_device, dst_root, &ctx, 0);
  if (err != ESP_OK) return err;

  if (!cmd_fileops_tree_progress(&ctx, cmd_fs_filename_ptr(src_root), false)) return CMD_ERR_CANCELLED;
  return ESP_OK;
}

esp_err_t cmd_fileops_copy_path(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options)
{
  return cmd_fileops_copy_path_as_op(fs, src_device, src_path, dst_device, dst_path, options, CMD_FILEOPS_OP_COPY);
}

esp_err_t cmd_fileops_copy_file(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options)
{
  CmdFileopsTreeCtx ctx;
  cmd_file_entry_t src_entry;
  esp_err_t err;

  if (!fs || !src_path || !dst_path) return ESP_ERR_INVALID_ARG;
  if (cmd_fileops_same_location(src_device, src_path, dst_device, dst_path)) return ESP_ERR_INVALID_STATE;

  err = cmd_fileops_ensure_device(fs, src_device);
  if (err != ESP_OK) return err;

  err = cmd_fileops_ensure_device(fs, dst_device);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_stat(fs, src_device, src_path, &src_entry);
  if (err != ESP_OK) return err;
  if (src_entry.type != CMD_ENTRY_FILE) return ESP_ERR_NOT_SUPPORTED;

  err = cmd_fileops_prepare_tree_context(&ctx, options, CMD_FILEOPS_OP_COPY, src_entry.size, 1);
  if (err != ESP_OK) return err;

  return cmd_fileops_copy_file_with_context(fs,
    src_device,
    src_path,
    &src_entry,
    dst_device,
    dst_path,
    &ctx);
}

esp_err_t cmd_fileops_move_path(CmdFsManager *fs,
  cmd_device_id_t src_device,
  const char *src_path,
  cmd_device_id_t dst_device,
  const char *dst_path,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  CmdFileopsTreeCtx ctx;
  CmdFileopsOptions delete_options = {};
  CmdFsDriver *driver;
  cmd_file_entry_t src_entry;
  const char *src_root;
  const char *dst_root;
  const char *filename;
  uint64_t bytes_total = 0;
  size_t files_total = 0;
  esp_err_t err;

  if (!fs || !src_path || !dst_path) return ESP_ERR_INVALID_ARG;
  if (cmd_fs_path_is_root(src_path)) return ESP_ERR_INVALID_ARG;
  if (cmd_fileops_same_location(src_device, src_path, dst_device, dst_path)) return ESP_ERR_INVALID_STATE;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  err = cmd_fileops_copy_path_to_slot(workspace->src_paths[0], CMD_PATH_MAX, src_path);
  if (err != ESP_OK) return err;

  err = cmd_fileops_copy_path_to_slot(workspace->dst_paths[0], CMD_PATH_MAX, dst_path);
  if (err != ESP_OK) return err;

  src_root = workspace->src_paths[0];
  dst_root = workspace->dst_paths[0];

  err = cmd_fileops_ensure_device(fs, src_device);
  if (err != ESP_OK) return err;

  err = cmd_fileops_ensure_device(fs, dst_device);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_stat(fs, src_device, src_root, &src_entry);
  if (err != ESP_OK) return err;

  if (src_entry.type == CMD_ENTRY_DIR && src_device == dst_device &&
    cmd_fileops_path_contains(src_root, dst_root)) return ESP_ERR_INVALID_STATE;

  err = cmd_fileops_check_dst_available(fs, dst_device, dst_root, options, NULL);
  if (err != ESP_OK) return err;

  if (src_entry.type == CMD_ENTRY_FILE)
  {
    bytes_total = src_entry.size;
    files_total = 1;
  }
  else if (src_entry.type == CMD_ENTRY_DIR)
  {
    if (!cmd_fileops_should_recurse(options)) return ESP_ERR_NOT_SUPPORTED;
    err = cmd_fileops_scan_tree(fs, src_device, src_root, 0, &bytes_total, &files_total);
    if (err != ESP_OK) return err;
  }
  else
  {
    return ESP_ERR_NOT_SUPPORTED;
  }

  filename = cmd_fs_filename_ptr(src_root);
  err = cmd_fileops_prepare_tree_context(&ctx, options, CMD_FILEOPS_OP_MOVE, bytes_total, files_total);
  if (err != ESP_OK) return err;

  driver = cmd_fs_manager_get_driver(fs, src_device);
  if (src_device == dst_device && driver && driver->rename)
  {
    if (!cmd_fileops_tree_progress(&ctx, filename, true)) return CMD_ERR_CANCELLED;

    err = cmd_fs_manager_rename(fs, src_device, src_root, dst_root);
    if (err == ESP_OK)
    {
      ctx.bytes_done = ctx.bytes_total;
      ctx.files_done = ctx.files_total;
      if (!cmd_fileops_tree_progress(&ctx, filename, false)) return CMD_ERR_CANCELLED;
      return ESP_OK;
    }

    if (src_entry.type != CMD_ENTRY_DIR) return err;
  }

  err = cmd_fileops_copy_path_as_op(fs,
    src_device,
    src_root,
    dst_device,
    dst_root,
    options,
    CMD_FILEOPS_OP_MOVE);
  if (err != ESP_OK) return err;

  if (options)
    delete_options = *options;
  delete_options.flags |= CMD_FILEOPS_FLAG_RECURSIVE;
  delete_options.progress = NULL;
  delete_options.progress_ctx = NULL;

  return cmd_fileops_delete_path(fs, src_device, src_root, &delete_options);
}

esp_err_t cmd_fileops_delete_path(CmdFsManager *fs,
  cmd_device_id_t device_id,
  const char *path,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  cmd_file_entry_t entry;
  const char *path_root;
  const char *filename;
  uint64_t bytes_done = 0;
  uint64_t bytes_total = 0;
  size_t files_done = 0;
  size_t files_total = 0;
  esp_err_t err;

  if (!fs || !path) return ESP_ERR_INVALID_ARG;
  if (cmd_fs_path_is_root(path)) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;

  err = cmd_fileops_copy_path_to_slot(workspace->src_paths[0], CMD_PATH_MAX, path);
  if (err != ESP_OK) return err;

  path_root = workspace->src_paths[0];

  err = cmd_fileops_ensure_device(fs, device_id);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_stat(fs, device_id, path_root, &entry);
  if (err != ESP_OK) return err;

  if (entry.type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;
  if (entry.type == CMD_ENTRY_UNKNOWN) return ESP_ERR_NOT_SUPPORTED;

  filename = cmd_fs_filename_ptr(path_root);

  if (entry.type == CMD_ENTRY_FILE)
  {
    bytes_total = entry.size;
    files_total = 1;
    return cmd_fileops_delete_file_with_progress(fs,
      device_id,
      path_root,
      &entry,
      options,
      &bytes_done,
      bytes_total,
      &files_done,
      files_total,
      true);
  }

  if (entry.type != CMD_ENTRY_DIR) return ESP_ERR_NOT_SUPPORTED;

  if (!cmd_fileops_should_recurse(options))
  {
    if (!cmd_fileops_progress_call_ex(options,
      CMD_FILEOPS_OP_DELETE,
      filename,
      0,
      0,
      0,
      0,
      true)) return CMD_ERR_CANCELLED;

    err = cmd_fs_manager_remove(fs, device_id, path_root);
    if (err != ESP_OK) return err;

    if (!cmd_fileops_progress_call_ex(options,
      CMD_FILEOPS_OP_DELETE,
      filename,
      0,
      0,
      0,
      0,
      false)) return CMD_ERR_CANCELLED;

    return ESP_OK;
  }

  err = cmd_fileops_scan_tree(fs, device_id, path_root, 0, &bytes_total, &files_total);
  if (err != ESP_OK) return err;

  err = cmd_fileops_delete_dir_recursive(fs,
    device_id,
    path_root,
    options,
    &bytes_done,
    bytes_total,
    &files_done,
    files_total,
    0,
    true);
  if (err != ESP_OK) return err;

  if (!cmd_fileops_progress_call_ex(options,
    CMD_FILEOPS_OP_DELETE,
    filename,
    bytes_done,
    bytes_total,
    files_done,
    files_total,
    false)) return CMD_ERR_CANCELLED;

  return ESP_OK;
}

typedef struct
{
  const CmdFileopsOptions *options;
  cmd_fileops_op_t op;
  uint64_t bytes_done_base;
  uint64_t item_bytes_done_max;
  uint64_t bytes_total;
  size_t files_done_base;
  size_t files_total;
} CmdFileopsBatchCtx;

bool cmd_fileops_batch_progress_cb(const CmdFileopsProgress *progress, void *ctx)
{
  CmdFileopsBatchCtx *batch = (CmdFileopsBatchCtx *)ctx;
  uint64_t bytes_done;
  size_t files_done;

  if (!batch || !progress) return true;

  if (progress->bytes_done > batch->item_bytes_done_max)
    batch->item_bytes_done_max = progress->bytes_done;

  bytes_done = batch->bytes_done_base + batch->item_bytes_done_max;
  if (bytes_done > batch->bytes_total) bytes_done = batch->bytes_total;

  files_done = batch->files_done_base;
  if (!progress->can_cancel && progress->bytes_done >= progress->bytes_total)
    files_done++;

  if (files_done > batch->files_total) files_done = batch->files_total;

  return cmd_fileops_progress_call_ex(batch->options,
    batch->op,
    progress->filename,
    bytes_done,
    batch->bytes_total,
    files_done,
    batch->files_total,
    progress->can_cancel);
}

bool cmd_fileops_entry_is_marked(const cmd_file_entry_t *entry)
{
  if (!entry) return false;
  if (!(entry->flags & CMD_ENTRY_FLAG_SELECTED)) return false;
  if (entry->type == CMD_ENTRY_PARENT) return false;
  return true;
}

esp_err_t cmd_fileops_validate_selected_group(const CmdPanel *active,
  const CmdPanel *passive,
  cmd_fileops_op_t op)
{
  if (!active) return ESP_ERR_INVALID_ARG;
  if ((op == CMD_FILEOPS_OP_COPY || op == CMD_FILEOPS_OP_MOVE) && !passive) return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < active->count; i++)
  {
    const cmd_file_entry_t *entry = &active->entries[i];

    if (!cmd_fileops_entry_is_marked(entry)) continue;
    if (entry->type == CMD_ENTRY_FILE || entry->type == CMD_ENTRY_DIR) continue;
    return ESP_ERR_NOT_SUPPORTED;
  }

  return ESP_OK;
}

void cmd_fileops_batch_options(CmdFileopsBatchCtx *batch,
  const CmdFileopsOptions *options,
  CmdFileopsOptions *out_options)
{
  if (!out_options) return;

  if (options)
    *out_options = *options;
  else
    memset(out_options, 0, sizeof(*out_options));

  out_options->progress = cmd_fileops_batch_progress_cb;
  out_options->progress_ctx = batch;
}

esp_err_t cmd_fileops_make_selected_paths(const CmdPanel *active,
  const CmdPanel *passive,
  const cmd_file_entry_t *entry,
  char *src_path,
  size_t src_path_size,
  char *dst_path,
  size_t dst_path_size)
{
  esp_err_t err;

  if (!active || !passive || !entry || !src_path || !dst_path) return ESP_ERR_INVALID_ARG;

  err = cmd_fileops_join_selected_path(active, entry, src_path, src_path_size);
  if (err != ESP_OK) return err;

  return cmd_fs_join_path(passive->current_path, entry->name, dst_path, dst_path_size);
}

esp_err_t cmd_fileops_copy_selected_group(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  CmdFileopsBatchCtx batch = {};
  CmdFileopsOptions item_options = {};
  char *src_path;
  char *dst_path;
  esp_err_t err;

  if (!active || !passive) return ESP_ERR_INVALID_ARG;
  if (active->fs != passive->fs) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  src_path = workspace->scratch_src_path;
  dst_path = workspace->scratch_dst_path;

  cmd_panel_update_selection_stats(active);
  if (!cmd_panel_has_selection(active)) return ESP_ERR_NOT_FOUND;

  err = cmd_fileops_validate_selected_group(active, passive, CMD_FILEOPS_OP_COPY);
  if (err != ESP_OK) return err;

  batch.options = options;
  batch.op = CMD_FILEOPS_OP_COPY;
  err = cmd_fileops_count_selected_tree_bytes(active, &batch.bytes_total);
  if (err != ESP_OK) return err;
  batch.files_total = active->selected_count;
  cmd_fileops_batch_options(&batch, options, &item_options);

  for (size_t i = 0; i < active->count; i++)
  {
    const cmd_file_entry_t *entry = &active->entries[i];

    if (!cmd_fileops_entry_is_marked(entry)) continue;

    err = cmd_fileops_make_selected_paths(active,
      passive,
      entry,
      src_path,
      CMD_PATH_MAX,
      dst_path,
      CMD_PATH_MAX);
    if (err != ESP_OK) return err;

    batch.item_bytes_done_max = 0;
    err = cmd_fileops_copy_path(active->fs,
      active->device_id,
      src_path,
      passive->device_id,
      dst_path,
      &item_options);
    if (err != ESP_OK) return err;

    batch.files_done_base++;
    batch.bytes_done_base += batch.item_bytes_done_max;
  }

  return cmd_panel_reload(passive);
}

esp_err_t cmd_fileops_move_selected_group(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  CmdFileopsBatchCtx batch = {};
  CmdFileopsOptions item_options = {};
  char *src_path;
  char *dst_path;
  esp_err_t err;

  if (!active || !passive) return ESP_ERR_INVALID_ARG;
  if (active->fs != passive->fs) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  src_path = workspace->scratch_src_path;
  dst_path = workspace->scratch_dst_path;

  cmd_panel_update_selection_stats(active);
  if (!cmd_panel_has_selection(active)) return ESP_ERR_NOT_FOUND;

  err = cmd_fileops_validate_selected_group(active, passive, CMD_FILEOPS_OP_MOVE);
  if (err != ESP_OK) return err;

  batch.options = options;
  batch.op = CMD_FILEOPS_OP_MOVE;
  err = cmd_fileops_count_selected_tree_bytes(active, &batch.bytes_total);
  if (err != ESP_OK) return err;
  batch.files_total = active->selected_count;
  cmd_fileops_batch_options(&batch, options, &item_options);

  for (size_t i = 0; i < active->count; i++)
  {
    const cmd_file_entry_t *entry = &active->entries[i];

    if (!cmd_fileops_entry_is_marked(entry)) continue;

    err = cmd_fileops_make_selected_paths(active,
      passive,
      entry,
      src_path,
      CMD_PATH_MAX,
      dst_path,
      CMD_PATH_MAX);
    if (err != ESP_OK) return err;

    batch.item_bytes_done_max = 0;
    err = cmd_fileops_move_path(active->fs,
      active->device_id,
      src_path,
      passive->device_id,
      dst_path,
      &item_options);
    if (err != ESP_OK) return err;

    batch.files_done_base++;
    batch.bytes_done_base += batch.item_bytes_done_max;
  }

  err = cmd_panel_reload(active);
  if (err != ESP_OK) return err;

  if (passive != active)
    err = cmd_panel_reload(passive);

  return err;
}

esp_err_t cmd_fileops_delete_selected_group(CmdPanel *panel,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  CmdFileopsBatchCtx batch = {};
  CmdFileopsOptions item_options = {};
  char *path;
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  path = workspace->scratch_path;

  cmd_panel_update_selection_stats(panel);
  if (!cmd_panel_has_selection(panel)) return ESP_ERR_NOT_FOUND;

  err = cmd_fileops_validate_selected_group(panel, NULL, CMD_FILEOPS_OP_DELETE);
  if (err != ESP_OK) return err;

  batch.options = options;
  batch.op = CMD_FILEOPS_OP_DELETE;
  err = cmd_fileops_count_selected_tree_bytes(panel, &batch.bytes_total);
  if (err != ESP_OK) return err;
  batch.files_total = panel->selected_count;
  cmd_fileops_batch_options(&batch, options, &item_options);

  for (size_t i = 0; i < panel->count; i++)
  {
    const cmd_file_entry_t *entry = &panel->entries[i];

    if (!cmd_fileops_entry_is_marked(entry)) continue;

    err = cmd_fileops_join_selected_path(panel, entry, path, CMD_PATH_MAX);
    if (err != ESP_OK) return err;

    batch.item_bytes_done_max = 0;
    err = cmd_fileops_delete_path(panel->fs, panel->device_id, path, &item_options);
    if (err != ESP_OK) return err;

    batch.files_done_base++;
    batch.bytes_done_base += batch.item_bytes_done_max;
  }

  return cmd_panel_reload(panel);
}

esp_err_t cmd_fileops_copy_selected(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  const cmd_file_entry_t *entry;
  char *src_path;
  char *dst_path;
  esp_err_t err;

  if (!active || !passive) return ESP_ERR_INVALID_ARG;
  if (active->fs != passive->fs) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  src_path = workspace->scratch_src_path;
  dst_path = workspace->scratch_dst_path;

  cmd_panel_update_selection_stats(active);
  if (cmd_panel_has_selection(active))
    return cmd_fileops_copy_selected_group(active, passive, options);

  entry = cmd_panel_get_selected_entry(active);
  if (!entry) return ESP_ERR_NOT_FOUND;
  if (entry->type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;
  if (entry->type != CMD_ENTRY_FILE && entry->type != CMD_ENTRY_DIR) return ESP_ERR_NOT_SUPPORTED;

  err = cmd_fileops_join_selected_path(active, entry, src_path, CMD_PATH_MAX);
  if (err != ESP_OK) return err;

  err = cmd_fs_join_path(passive->current_path, entry->name, dst_path, CMD_PATH_MAX);
  if (err != ESP_OK) return err;

  err = cmd_fileops_copy_path(active->fs,
    active->device_id,
    src_path,
    passive->device_id,
    dst_path,
    options);
  if (err != ESP_OK) return err;

  return cmd_panel_reload(passive);
}

esp_err_t cmd_fileops_move_selected(CmdPanel *active,
  CmdPanel *passive,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  const cmd_file_entry_t *entry;
  char *src_path;
  char *dst_path;
  esp_err_t err;

  if (!active || !passive) return ESP_ERR_INVALID_ARG;
  if (active->fs != passive->fs) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  src_path = workspace->scratch_src_path;
  dst_path = workspace->scratch_dst_path;

  cmd_panel_update_selection_stats(active);
  if (cmd_panel_has_selection(active))
    return cmd_fileops_move_selected_group(active, passive, options);

  entry = cmd_panel_get_selected_entry(active);
  if (!entry) return ESP_ERR_NOT_FOUND;
  if (entry->type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;

  err = cmd_fileops_join_selected_path(active, entry, src_path, CMD_PATH_MAX);
  if (err != ESP_OK) return err;

  err = cmd_fs_join_path(passive->current_path, entry->name, dst_path, CMD_PATH_MAX);
  if (err != ESP_OK) return err;

  err = cmd_fileops_move_path(active->fs,
    active->device_id,
    src_path,
    passive->device_id,
    dst_path,
    options);
  if (err != ESP_OK) return err;

  err = cmd_panel_reload(active);
  if (err != ESP_OK) return err;

  if (passive != active)
    err = cmd_panel_reload(passive);

  return err;
}

esp_err_t cmd_fileops_delete_selected(CmdPanel *panel,
  const CmdFileopsOptions *options)
{
  CmdFileopsWorkspace *workspace;
  const cmd_file_entry_t *entry;
  char *path;
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;

  workspace = cmd_fileops_workspace();
  if (!workspace) return ESP_ERR_NO_MEM;
  path = workspace->scratch_path;

  cmd_panel_update_selection_stats(panel);
  if (cmd_panel_has_selection(panel))
    return cmd_fileops_delete_selected_group(panel, options);

  entry = cmd_panel_get_selected_entry(panel);
  if (!entry) return ESP_ERR_NOT_FOUND;
  if (entry->type == CMD_ENTRY_PARENT) return ESP_ERR_INVALID_ARG;

  err = cmd_fileops_join_selected_path(panel, entry, path, CMD_PATH_MAX);
  if (err != ESP_OK) return err;

  err = cmd_fileops_delete_path(panel->fs, panel->device_id, path, options);
  if (err != ESP_OK) return err;

  return cmd_panel_reload(panel);
}

// --------------- cmd_fs.cpp ---------------

esp_err_t cmd_fs_errno_to_err(int err)
{
  if (err == 0) return ESP_OK;
  if (err == ENOENT) return ESP_ERR_NOT_FOUND;
  if (err == ENOMEM) return ESP_ERR_NO_MEM;
  if (err == EINVAL) return ESP_ERR_INVALID_ARG;
  if (err == ENAMETOOLONG) return CMD_ERR_PATH_TOO_LONG;
  if (err == ENOTDIR) return ESP_ERR_NOT_FOUND;
  if (err == EEXIST) return ESP_ERR_INVALID_STATE;
  if (err == ENOSPC) return ESP_ERR_NO_MEM;
  return ESP_FAIL;
}


const char *cmd_device_name(cmd_device_id_t id)
{
  switch (id)
  {
    case CMD_DEVICE_SD: return "SD";
    case CMD_DEVICE_FAT: return "FAT";
    case CMD_DEVICE_TSF: return "TSF";
    default: return "?";
  }
}

const char *cmd_entry_type_name(cmd_entry_type_t type)
{
  switch (type)
  {
    case CMD_ENTRY_FILE: return "file";
    case CMD_ENTRY_DIR: return "dir";
    case CMD_ENTRY_PARENT: return "parent";
    case CMD_ENTRY_UNKNOWN: return "unknown";
    default: return "unknown";
  }
}

bool cmd_fs_is_valid_device_id(cmd_device_id_t device_id)
{
  if (device_id < 0) return false;
  if (device_id >= CMD_DEVICE_MAX) return false;
  return true;
}

esp_err_t cmd_fs_copy_path(const char *src, char *out, size_t out_size)
{
  const char *s = src;
  size_t len;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (!s || !s[0]) s = "/";

  len = strlen(s);
  if (len >= out_size) return CMD_ERR_PATH_TOO_LONG;

  memcpy(out, s, len);
  out[len] = 0;
  return ESP_OK;
}

bool cmd_fs_path_is_root(const char *path)
{
  if (!path || !path[0]) return true;
  if (strcmp(path, "/") == 0) return true;
  return false;
}

esp_err_t cmd_fs_join_path(const char *left, const char *right, char *out, size_t out_size)
{
  const char *a = left;
  const char *b = right;
  size_t a_len;
  size_t b_len;
  size_t pos = 0;
  bool need_slash;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (!a || !a[0]) a = "/";
  if (!b) b = "";

  a_len = strlen(a);
  while (a_len > 1 && a[a_len - 1] == '/')
    a_len--;

  while (*b == '/')
    b++;

  b_len = strlen(b);

  if (b_len == 0)
  {
    if (a_len >= out_size) return CMD_ERR_PATH_TOO_LONG;
    memcpy(out, a, a_len);
    out[a_len] = 0;
    return ESP_OK;
  }

  need_slash = !(a_len == 1 && a[0] == '/');

  if (a_len + (need_slash ? 1 : 0) + b_len >= out_size)
    return CMD_ERR_PATH_TOO_LONG;

  if (a_len == 1 && a[0] == '/')
  {
    out[pos++] = '/';
  }
  else
  {
    memcpy(out + pos, a, a_len);
    pos += a_len;
    out[pos++] = '/';
  }

  memcpy(out + pos, b, b_len);
  pos += b_len;
  out[pos] = 0;
  return ESP_OK;
}

esp_err_t cmd_fs_join3_path(const char *left, const char *middle, const char *right, char *out, size_t out_size)
{
  char tmp[CMD_PATH_MAX];
  esp_err_t err;

  err = cmd_fs_join_path(left, middle, tmp, sizeof(tmp));
  if (err != ESP_OK) return err;

  return cmd_fs_join_path(tmp, right, out, out_size);
}

esp_err_t cmd_fs_parent_path(const char *path, char *out, size_t out_size)
{
  const char *src = path;
  size_t len;
  size_t i;
  size_t parent_len;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (!src || !src[0]) src = "/";

  len = strlen(src);
  while (len > 1 && src[len - 1] == '/')
    len--;

  if (len == 1 && src[0] == '/')
    return cmd_fs_copy_path("/", out, out_size);

  i = len;
  while (i > 0 && src[i - 1] != '/')
    i--;

  if (i == 0 || i == 1)
    return cmd_fs_copy_path("/", out, out_size);

  parent_len = i - 1;
  if (parent_len >= out_size) return CMD_ERR_PATH_TOO_LONG;

  memcpy(out, src, parent_len);
  out[parent_len] = 0;
  return ESP_OK;
}

const char *cmd_fs_filename_ptr(const char *path)
{
  const char *p = path;
  const char *name = path;

  if (!path) return "";
  if (!path[0]) return "";

  while (*p)
  {
    if (*p == '/' && p[1])
      name = p + 1;
    p++;
  }

  return name;
}

esp_err_t cmd_fs_extract_filename(const char *path, char *out, size_t out_size)
{
  const char *src = path;
  size_t len;
  size_t end;
  size_t start;
  size_t name_len;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;
  if (!src || !src[0])
  {
    out[0] = 0;
    return ESP_OK;
  }

  len = strlen(src);
  end = len;
  while (end > 0 && src[end - 1] == '/')
    end--;

  if (end == 0)
  {
    out[0] = 0;
    return ESP_OK;
  }

  start = end;
  while (start > 0 && src[start - 1] != '/')
    start--;

  name_len = end - start;
  if (name_len >= out_size) return CMD_ERR_ENTRY_TOO_LONG;

  memcpy(out, src + start, name_len);
  out[name_len] = 0;
  return ESP_OK;
}

bool cmd_fs_path_has_root_prefix(const char *root_path, const char *path)
{
  size_t root_len;

  if (!root_path || !root_path[0]) return false;
  if (!path || !path[0]) return false;

  root_len = strlen(root_path);
  if (root_len == 1 && root_path[0] == '/') return path[0] == '/';
  if (strncmp(path, root_path, root_len) != 0) return false;

  return path[root_len] == 0 || path[root_len] == '/';
}

esp_err_t cmd_fs_driver_build_abs_path(const CmdFsDriver *driver, const char *path, char *out, size_t out_size)
{
  if (!driver || !driver->root_path || !driver->root_path[0]) return ESP_ERR_INVALID_ARG;
  if (cmd_fs_path_has_root_prefix(driver->root_path, path))
    return cmd_fs_copy_path(path, out, out_size);

  return cmd_fs_join_path(driver->root_path, path, out, out_size);
}

esp_err_t cmd_fs_check_root_path(const CmdFsDriver *driver)
{
  DIR *dir;
  struct stat st = {};

  if (!driver || !driver->root_path || !driver->root_path[0]) return ESP_ERR_INVALID_ARG;

  errno = 0;
  dir = opendir(driver->root_path);
  if (dir)
  {
    closedir(dir);
    return ESP_OK;
  }

  errno = 0;
  if (stat(driver->root_path, &st) != 0) return cmd_fs_errno_to_err(errno);
  if (!S_ISDIR(st.st_mode)) return ESP_ERR_INVALID_STATE;

  return ESP_OK;
}

esp_err_t cmd_fs_driver_space_info(const CmdFsDriver *driver, CmdFsSpaceInfo *info)
{
  uint64_t total = 0;
  uint64_t free = 0;
  esp_err_t err;

  if (!driver || !driver->root_path || !driver->root_path[0]) return ESP_ERR_INVALID_ARG;
  if (!info) return ESP_ERR_INVALID_ARG;

  memset(info, 0, sizeof(*info));

  if (driver->device_id == CMD_DEVICE_SD)
  {
    esp_log_level_t old_sd_host_level = sd_host_log_suppress_begin();

    err = sd_fs_mount_quiet(driver->root_path, NULL);
    if (err == ESP_OK)
      err = esp_vfs_fat_info(driver->root_path, &total, &free);

    sd_host_log_suppress_end(old_sd_host_level);
    if (err != ESP_OK) return err;

    info->total_bytes = total;
    info->free_bytes = free;
    return ESP_OK;
  }

  if (driver->device_id == CMD_DEVICE_FAT)
  {
    err = esp_vfs_fat_info(driver->root_path, &total, &free);
    if (err != ESP_OK) return err;

    info->total_bytes = total;
    info->free_bytes = free;
    return ESP_OK;
  }

  if (driver->device_id == CMD_DEVICE_TSF)
    return tsf_storage_space_info(&info->total_bytes, &info->free_bytes);

  return CMD_ERR_INVALID_DEVICE;
}

cmd_entry_type_t cmd_fs_entry_type_from_mode(mode_t mode)
{
  if (S_ISDIR(mode)) return CMD_ENTRY_DIR;
  if (S_ISREG(mode)) return CMD_ENTRY_FILE;
  return CMD_ENTRY_UNKNOWN;
}

esp_err_t cmd_fs_fill_entry_from_stat(const char *name, const struct stat *st, cmd_file_entry_t *entry)
{
  if (!name || !st || !entry) return ESP_ERR_INVALID_ARG;

  memset(entry, 0, sizeof(*entry));
  size_t name_len = strlen(name);
  if (name_len >= sizeof(entry->name)) return CMD_ERR_ENTRY_TOO_LONG;
  memcpy(entry->name, name, name_len);
  entry->name[name_len] = 0;

  entry->type = cmd_fs_entry_type_from_mode(st->st_mode);
  entry->size = (uint64_t)st->st_size;
  entry->mtime = (uint64_t)st->st_mtime;
  entry->flags = CMD_ENTRY_FLAG_NONE;
  return ESP_OK;
}

esp_err_t cmd_fs_fill_parent_entry(cmd_file_entry_t *entry)
{
  if (!entry) return ESP_ERR_INVALID_ARG;

  memset(entry, 0, sizeof(*entry));
  entry->name[0] = '.';
  entry->name[1] = '.';
  entry->name[2] = 0;

  entry->type = CMD_ENTRY_PARENT;
  entry->size = 0;
  entry->mtime = 0;
  entry->flags = CMD_ENTRY_FLAG_NONE;
  return ESP_OK;
}

esp_err_t cmd_fs_posix_list_begin(CmdFsDriver *driver, const char *path, CmdFsList *list)
{
  DIR *dir;
  esp_err_t err;

  if (!driver || !list) return ESP_ERR_INVALID_ARG;

  memset(list, 0, sizeof(*list));
  list->device_id = driver->device_id;

  err = cmd_fs_copy_path(path, list->path, sizeof(list->path));
  if (err != ESP_OK) return err;

  err = cmd_fs_driver_build_abs_path(driver, list->path, list->abs_path, sizeof(list->abs_path));
  if (err != ESP_OK) return err;

  dir = opendir(list->abs_path);
  if (!dir) return cmd_fs_errno_to_err(errno);

  list->dir = dir;
  list->parent_pending = !cmd_fs_path_is_root(list->path);
  list->index = 0;
  list->flags = 0;
  return ESP_OK;
}

esp_err_t cmd_fs_posix_list_next(CmdFsDriver *driver, CmdFsList *list, cmd_file_entry_t *entry)
{
  DIR *dir;
  struct dirent *de;
  char item_path[CMD_PATH_MAX];
  struct stat st = {};
  esp_err_t err;

  if (!driver || !list || !entry) return ESP_ERR_INVALID_ARG;
  if (list->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;

  if (list->parent_pending)
  {
    list->parent_pending = false;
    list->index++;
    return cmd_fs_fill_parent_entry(entry);
  }

  dir = (DIR *)list->dir;
  if (!dir) return ESP_ERR_INVALID_STATE;

  for (;;)
  {
    errno = 0;
    de = readdir(dir);
    if (!de)
    {
      if (errno != 0) return cmd_fs_errno_to_err(errno);
      return ESP_ERR_NOT_FOUND;
    }

    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
      continue;

    err = cmd_fs_join_path(list->abs_path, de->d_name, item_path, sizeof(item_path));
    if (err != ESP_OK) return err;

    if (stat(item_path, &st) != 0)
    {
      memset(entry, 0, sizeof(*entry));
      err = cmd_fs_copy_path(de->d_name, entry->name, sizeof(entry->name));
      if (err != ESP_OK) return err;
      entry->type = CMD_ENTRY_UNKNOWN;
      entry->flags = CMD_ENTRY_FLAG_ERROR;
    }
    else
    {
      err = cmd_fs_fill_entry_from_stat(de->d_name, &st, entry);
      if (err != ESP_OK) return err;
    }

    list->index++;
    return ESP_OK;
  }
}

void cmd_fs_posix_list_close(CmdFsDriver *driver, CmdFsList *list)
{
  if (!driver || !list) return;
  if (list->device_id != driver->device_id) return;

  if (list->dir)
  {
    closedir((DIR *)list->dir);
    list->dir = NULL;
  }

  memset(list, 0, sizeof(*list));
}

esp_err_t cmd_fs_posix_stat(CmdFsDriver *driver, const char *path, cmd_file_entry_t *entry)
{
  char abs[CMD_PATH_MAX];
  struct stat st = {};
  esp_err_t err;

  if (!driver || !entry) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_driver_build_abs_path(driver, path, abs, sizeof(abs));
  if (err != ESP_OK) return err;

  if (stat(abs, &st) != 0) return cmd_fs_errno_to_err(errno);

  return cmd_fs_fill_entry_from_stat(cmd_fs_filename_ptr(abs), &st, entry);
}

esp_err_t cmd_fs_posix_mkdir(CmdFsDriver *driver, const char *path)
{
  char abs[CMD_PATH_MAX];
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_driver_build_abs_path(driver, path, abs, sizeof(abs));
  if (err != ESP_OK) return err;

  if (mkdir(abs, 0775) != 0) return cmd_fs_errno_to_err(errno);
  return ESP_OK;
}

esp_err_t cmd_fs_posix_remove(CmdFsDriver *driver, const char *path)
{
  char abs[CMD_PATH_MAX];
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_driver_build_abs_path(driver, path, abs, sizeof(abs));
  if (err != ESP_OK) return err;

  if (remove(abs) != 0) return cmd_fs_errno_to_err(errno);
  return ESP_OK;
}

esp_err_t cmd_fs_posix_rename(CmdFsDriver *driver, const char *old_path, const char *new_path)
{
  char old_abs[CMD_PATH_MAX];
  char new_abs[CMD_PATH_MAX];
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_driver_build_abs_path(driver, old_path, old_abs, sizeof(old_abs));
  if (err != ESP_OK) return err;

  err = cmd_fs_driver_build_abs_path(driver, new_path, new_abs, sizeof(new_abs));
  if (err != ESP_OK) return err;

  if (rename(old_abs, new_abs) != 0) return cmd_fs_errno_to_err(errno);
  return ESP_OK;
}

esp_err_t cmd_fs_posix_open(CmdFsDriver *driver, const char *path, const char *mode, CmdFsFile *file)
{
  char abs[CMD_PATH_MAX];
  esp_err_t err;

  if (!driver || !mode || !file) return ESP_ERR_INVALID_ARG;

  memset(file, 0, sizeof(*file));

  err = cmd_fs_driver_build_abs_path(driver, path, abs, sizeof(abs));
  if (err != ESP_OK) return err;

  file->fp = fopen(abs, mode);
  if (!file->fp) return cmd_fs_errno_to_err(errno);

  file->device_id = driver->device_id;
  return ESP_OK;
}

esp_err_t cmd_fs_posix_open_read(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  return cmd_fs_posix_open(driver, path, "rb", file);
}

esp_err_t cmd_fs_posix_open_write(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  return cmd_fs_posix_open(driver, path, "wb", file);
}

esp_err_t cmd_fs_posix_read(CmdFsDriver *driver, CmdFsFile *file, void *dst, size_t size, size_t *out_size)
{
  size_t got;

  if (!driver || !file || !dst) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->fp) return ESP_ERR_INVALID_STATE;

  got = fread(dst, 1, size, file->fp);
  if (out_size) *out_size = got;
  if (got < size && ferror(file->fp)) return ESP_FAIL;

  return ESP_OK;
}

esp_err_t cmd_fs_posix_write(CmdFsDriver *driver, CmdFsFile *file, const void *src, size_t size, size_t *out_size)
{
  size_t done;

  if (!driver || !file || (!src && size)) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->fp) return ESP_ERR_INVALID_STATE;

  done = fwrite(src, 1, size, file->fp);
  if (out_size) *out_size = done;
  if (done != size) return ESP_FAIL;

  return ESP_OK;
}

esp_err_t cmd_fs_posix_seek(CmdFsDriver *driver, CmdFsFile *file, int64_t offset, int whence)
{
  if (!driver || !file) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->fp) return ESP_ERR_INVALID_STATE;
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) return ESP_ERR_INVALID_ARG;
  if (offset < (int64_t)LONG_MIN || offset > (int64_t)LONG_MAX) return ESP_ERR_INVALID_ARG;

  if (fseek(file->fp, (long)offset, whence) != 0) return cmd_fs_errno_to_err(errno);
  return ESP_OK;
}

esp_err_t cmd_fs_posix_tell(CmdFsDriver *driver, CmdFsFile *file, uint64_t *out_pos)
{
  long pos;

  if (!driver || !file || !out_pos) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->fp) return ESP_ERR_INVALID_STATE;

  pos = ftell(file->fp);
  if (pos < 0) return cmd_fs_errno_to_err(errno);

  *out_pos = (uint64_t)pos;
  return ESP_OK;
}

esp_err_t cmd_fs_posix_close(CmdFsDriver *driver, CmdFsFile *file)
{
  int rc;

  if (!driver || !file) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->fp) return ESP_OK;

  rc = fclose(file->fp);
  file->fp = NULL;
  file->device_id = CMD_DEVICE_INVALID;
  if (rc != 0) return cmd_fs_errno_to_err(errno);

  return ESP_OK;
}


esp_err_t cmd_fs_tsf_require_volume(TSF_VOLUME **out_vol)
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

esp_err_t cmd_fs_tsf_fill_file_entry(const char *name, uint32_t size, cmd_file_entry_t *entry)
{
  esp_err_t err;

  if (!name || !entry) return ESP_ERR_INVALID_ARG;

  memset(entry, 0, sizeof(*entry));
  err = cmd_fs_copy_path(name, entry->name, sizeof(entry->name));
  if (err != ESP_OK) return err;

  entry->type = CMD_ENTRY_FILE;
  entry->size = size;
  entry->mtime = 0;
  entry->flags = CMD_ENTRY_FLAG_NONE;
  return ESP_OK;
}

esp_err_t cmd_fs_tsf_list_begin(CmdFsDriver *driver, const char *path, CmdFsList *list)
{
  TSF_VOLUME *vol;
  esp_err_t err;
  TSF_RESULT res;

  if (!driver || !list) return ESP_ERR_INVALID_ARG;
  if (!tsf_storage_path_is_root(path)) return ESP_ERR_NOT_SUPPORTED;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  memset(list, 0, sizeof(*list));
  list->device_id = driver->device_id;
  err = cmd_fs_copy_path(path, list->path, sizeof(list->path));
  if (err != ESP_OK) return err;
  list->parent_pending = false;
  list->index = 0;

  res = tsf_list(vol, TSF_LIST_START);
  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_list_next(CmdFsDriver *driver, CmdFsList *list, cmd_file_entry_t *entry)
{
  TSF_VOLUME *vol;
  TSF_CONFIG *cfg;
  TSF_FILE_STAT st = {};
  TSF_RESULT res;
  esp_err_t err;

  if (!driver || !list || !entry) return ESP_ERR_INVALID_ARG;
  if (list->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  cfg = vol->cfg;
  if (!cfg || !cfg->buf) return ESP_ERR_INVALID_STATE;

  res = tsf_list(vol, TSF_LIST_NEXT);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  res = tsf_stat(vol, &st, (const char *)cfg->buf);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  list->index++;
  return cmd_fs_tsf_fill_file_entry((const char *)cfg->buf, st.size, entry);
}

void cmd_fs_tsf_list_close(CmdFsDriver *driver, CmdFsList *list)
{
  if (!driver || !list) return;
  if (list->device_id != driver->device_id) return;
  memset(list, 0, sizeof(*list));
}

esp_err_t cmd_fs_tsf_stat(CmdFsDriver *driver, const char *path, cmd_file_entry_t *entry)
{
  char name[CMD_FILE_NAME_MAX];
  TSF_VOLUME *vol;
  TSF_FILE_STAT st = {};
  TSF_RESULT res;
  esp_err_t err;

  if (!driver || !entry) return ESP_ERR_INVALID_ARG;

  if (tsf_storage_path_is_root(path))
  {
    memset(entry, 0, sizeof(*entry));
    err = cmd_fs_copy_path("/", entry->name, sizeof(entry->name));
    if (err != ESP_OK) return err;
    entry->type = CMD_ENTRY_DIR;
    return ESP_OK;
  }

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return err;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  res = tsf_stat(vol, &st, name);
  if (res != TSF_RES_OK) return tsf_storage_result_to_err(res);

  return cmd_fs_tsf_fill_file_entry(name, st.size, entry);
}

esp_err_t cmd_fs_tsf_mkdir(CmdFsDriver *driver, const char *path)
{
  if (!driver || !path) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t cmd_fs_tsf_remove(CmdFsDriver *driver, const char *path)
{
  char name[CMD_FILE_NAME_MAX];
  TSF_VOLUME *vol;
  TSF_RESULT res;
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return err;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  res = tsf_delete(vol, name);
  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_rename(CmdFsDriver *driver, const char *old_path, const char *new_path)
{
  if (!driver || !old_path || !new_path) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t cmd_fs_tsf_open_common(CmdFsDriver *driver, const char *path, u8 mode, CmdFsFile *file)
{
  char name[CMD_FILE_NAME_MAX];
  TSF_VOLUME *vol;
  TSF_FILE *tsf_file;
  TSF_RESULT res;
  esp_err_t err;

  if (!driver || !file) return ESP_ERR_INVALID_ARG;

  err = tsf_storage_name_from_path(path, name, sizeof(name));
  if (err != ESP_OK) return err;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  memset(file, 0, sizeof(*file));
  tsf_file = (TSF_FILE *)calloc(1, sizeof(TSF_FILE));
  if (!tsf_file) return ESP_ERR_NO_MEM;

  if (mode == TSF_MODE_CREATE_WRITE)
  {
    res = tsf_delete(vol, name);
    if (res != TSF_RES_OK && res != TSF_RES_FILE_NOT_FOUND)
    {
      free(tsf_file);
      return tsf_storage_result_to_err(res);
    }
  }

  res = tsf_open(vol, tsf_file, name, mode);
  if (res != TSF_RES_OK)
  {
    free(tsf_file);
    return tsf_storage_result_to_err(res);
  }

  file->device_id = driver->device_id;
  file->ctx = tsf_file;
  file->flags = mode;
  err = cmd_fs_copy_path(name, file->name, sizeof(file->name));
  if (err != ESP_OK)
  {
    tsf_close(tsf_file);
    free(tsf_file);
    memset(file, 0, sizeof(*file));
    return err;
  }

  return ESP_OK;
}

esp_err_t cmd_fs_tsf_open_read(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  return cmd_fs_tsf_open_common(driver, path, TSF_MODE_READ, file);
}

esp_err_t cmd_fs_tsf_open_write(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  return cmd_fs_tsf_open_common(driver, path, TSF_MODE_CREATE_WRITE, file);
}

esp_err_t cmd_fs_tsf_read(CmdFsDriver *driver, CmdFsFile *file, void *dst, size_t size, size_t *out_size)
{
  TSF_FILE *tsf_file;
  TSF_RESULT res;
  u32 before;
  u32 todo;

  if (!driver || !file || !dst) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->ctx) return ESP_ERR_INVALID_STATE;
  if (size > UINT32_MAX) return ESP_ERR_INVALID_SIZE;

  tsf_file = (TSF_FILE *)file->ctx;
  before = tsf_file->seek;
  todo = (u32)size;

  res = tsf_read(tsf_file, dst, todo);
  if (out_size) *out_size = (size_t)(tsf_file->seek - before);

  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_write(CmdFsDriver *driver, CmdFsFile *file, const void *src, size_t size, size_t *out_size)
{
  TSF_FILE *tsf_file;
  TSF_RESULT res;
  u32 before;
  u32 todo;

  if (!driver || !file || (!src && size)) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->ctx) return ESP_ERR_INVALID_STATE;
  if (size > UINT32_MAX) return ESP_ERR_INVALID_SIZE;

  tsf_file = (TSF_FILE *)file->ctx;
  before = tsf_file->seek;
  todo = (u32)size;

  res = tsf_write(tsf_file, src, todo);
  if (out_size) *out_size = (size_t)(tsf_file->seek - before);

  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_reopen_read(CmdFsDriver *driver, CmdFsFile *file)
{
  TSF_VOLUME *vol;
  TSF_FILE *tsf_file;
  TSF_RESULT res;
  esp_err_t err;

  if (!driver || !file || !file->ctx) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_tsf_require_volume(&vol);
  if (err != ESP_OK) return err;

  tsf_file = (TSF_FILE *)file->ctx;
  memset(tsf_file, 0, sizeof(*tsf_file));
  res = tsf_open(vol, tsf_file, file->name, TSF_MODE_READ);
  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_seek(CmdFsDriver *driver, CmdFsFile *file, int64_t offset, int whence)
{
  TSF_FILE *tsf_file;
  int64_t base;
  int64_t target;
  TSF_RESULT res;
  esp_err_t err;

  if (!driver || !file) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->ctx) return ESP_ERR_INVALID_STATE;
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) return ESP_ERR_INVALID_ARG;

  tsf_file = (TSF_FILE *)file->ctx;
  if ((file->flags & TSF_MODE_WRITE) == TSF_MODE_WRITE) return ESP_ERR_NOT_SUPPORTED;

  if (whence == SEEK_SET)
    base = 0;
  else if (whence == SEEK_CUR)
    base = (int64_t)tsf_file->seek;
  else
    base = (int64_t)tsf_file->size;

  target = base + offset;
  if (target < 0 || target > (int64_t)tsf_file->size) return ESP_ERR_INVALID_ARG;

  if ((u32)target < tsf_file->seek)
  {
    err = cmd_fs_tsf_reopen_read(driver, file);
    if (err != ESP_OK) return err;
    tsf_file = (TSF_FILE *)file->ctx;
  }

  res = tsf_seek(tsf_file, (u32)target - tsf_file->seek);
  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_tsf_tell(CmdFsDriver *driver, CmdFsFile *file, uint64_t *out_pos)
{
  TSF_FILE *tsf_file;

  if (!driver || !file || !out_pos) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->ctx) return ESP_ERR_INVALID_STATE;

  tsf_file = (TSF_FILE *)file->ctx;
  *out_pos = tsf_file->seek;
  return ESP_OK;
}

esp_err_t cmd_fs_tsf_close(CmdFsDriver *driver, CmdFsFile *file)
{
  TSF_FILE *tsf_file;
  TSF_RESULT res;

  if (!driver || !file) return ESP_ERR_INVALID_ARG;
  if (file->device_id != driver->device_id) return CMD_ERR_INVALID_DEVICE;
  if (!file->ctx) return ESP_OK;

  tsf_file = (TSF_FILE *)file->ctx;
  res = tsf_close(tsf_file);
  free(tsf_file);
  memset(file, 0, sizeof(*file));
  file->device_id = CMD_DEVICE_INVALID;

  return tsf_storage_result_to_err(res);
}

esp_err_t cmd_fs_sd_sense(CmdFsDriver *driver)
{
  if (!driver) return ESP_ERR_INVALID_ARG;
  return sd_fs_mount_quiet(driver->root_path, NULL);
}

esp_err_t cmd_fs_sd_mount_once()
{
  return sd_fs_mount_quiet(CMD_FS_SD_ROOT_PATH, NULL);
}

esp_err_t cmd_fs_sd_mount(CmdFsDriver *driver)
{
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_sd_mount_once();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t cmd_fs_fat_mount(CmdFsDriver *driver)
{
  esp_err_t err;

  if (!driver) return ESP_ERR_INVALID_ARG;

  err = fs_ensure_ready_quiet();
  if (err != ESP_OK) return err;

  return cmd_fs_check_root_path(driver);
}

esp_err_t cmd_fs_tsf_mount(CmdFsDriver *driver)
{
  if (!driver) return ESP_ERR_INVALID_ARG;
  return tsf_storage_ensure_ready_quiet();
}

esp_err_t cmd_fs_sd_list_begin(CmdFsDriver *driver, const char *path, CmdFsList *list)
{
  esp_err_t err;

  if (!driver || !list) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;

  return cmd_fs_posix_list_begin(driver, path, list);
}

esp_err_t cmd_fs_sd_list_next(CmdFsDriver *driver, CmdFsList *list, cmd_file_entry_t *entry)
{
  esp_err_t err;

  err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;

  return cmd_fs_posix_list_next(driver, list, entry);
}

esp_err_t cmd_fs_sd_stat(CmdFsDriver *driver, const char *path, cmd_file_entry_t *entry)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_stat(driver, path, entry);
}

esp_err_t cmd_fs_sd_mkdir(CmdFsDriver *driver, const char *path)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_mkdir(driver, path);
}

esp_err_t cmd_fs_sd_remove(CmdFsDriver *driver, const char *path)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_remove(driver, path);
}

esp_err_t cmd_fs_sd_rename(CmdFsDriver *driver, const char *old_path, const char *new_path)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_rename(driver, old_path, new_path);
}

esp_err_t cmd_fs_sd_open_read(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_open_read(driver, path, file);
}

esp_err_t cmd_fs_sd_open_write(CmdFsDriver *driver, const char *path, CmdFsFile *file)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_open_write(driver, path, file);
}

esp_err_t cmd_fs_sd_read(CmdFsDriver *driver, CmdFsFile *file, void *dst, size_t size, size_t *out_size)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_read(driver, file, dst, size, out_size);
}

esp_err_t cmd_fs_sd_write(CmdFsDriver *driver, CmdFsFile *file, const void *src, size_t size, size_t *out_size)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_write(driver, file, src, size, out_size);
}

esp_err_t cmd_fs_sd_seek(CmdFsDriver *driver, CmdFsFile *file, int64_t offset, int whence)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_seek(driver, file, offset, whence);
}

esp_err_t cmd_fs_sd_tell(CmdFsDriver *driver, CmdFsFile *file, uint64_t *out_pos)
{
  esp_err_t err = cmd_fs_sd_sense(driver);
  if (err != ESP_OK) return err;
  return cmd_fs_posix_tell(driver, file, out_pos);
}

esp_err_t cmd_fs_sd_unmount(CmdFsDriver *driver)
{
  if (!driver) return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}

esp_err_t cmd_fs_noop_unmount(CmdFsDriver *driver)
{
  if (!driver) return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}

CmdFsDriver cmd_fs_sd_driver =
{
  .device_id = CMD_DEVICE_SD,
  .name = "sd",
  .root_path = CMD_FS_SD_ROOT_PATH,
  .ctx = NULL,
  .mount = cmd_fs_sd_mount,
  .unmount = cmd_fs_sd_unmount,
  .list_begin = cmd_fs_sd_list_begin,
  .list_next = cmd_fs_sd_list_next,
  .list_close = cmd_fs_posix_list_close,
  .stat = cmd_fs_sd_stat,
  .mkdir = cmd_fs_sd_mkdir,
  .remove = cmd_fs_sd_remove,
  .rename = cmd_fs_sd_rename,
  .open_read = cmd_fs_sd_open_read,
  .open_write = cmd_fs_sd_open_write,
  .read = cmd_fs_sd_read,
  .write = cmd_fs_sd_write,
  .seek = cmd_fs_sd_seek,
  .tell = cmd_fs_sd_tell,
  .close = cmd_fs_posix_close
};

CmdFsDriver cmd_fs_fat_driver =
{
  .device_id = CMD_DEVICE_FAT,
  .name = "fat",
  .root_path = FS_BASE_PATH,
  .ctx = NULL,
  .mount = cmd_fs_fat_mount,
  .unmount = cmd_fs_noop_unmount,
  .list_begin = cmd_fs_posix_list_begin,
  .list_next = cmd_fs_posix_list_next,
  .list_close = cmd_fs_posix_list_close,
  .stat = cmd_fs_posix_stat,
  .mkdir = cmd_fs_posix_mkdir,
  .remove = cmd_fs_posix_remove,
  .rename = cmd_fs_posix_rename,
  .open_read = cmd_fs_posix_open_read,
  .open_write = cmd_fs_posix_open_write,
  .read = cmd_fs_posix_read,
  .write = cmd_fs_posix_write,
  .seek = cmd_fs_posix_seek,
  .tell = cmd_fs_posix_tell,
  .close = cmd_fs_posix_close
};

CmdFsDriver cmd_fs_tsf_driver =
{
  .device_id = CMD_DEVICE_TSF,
  .name = "tsf",
  .root_path = TSF_BASE_PATH,
  .ctx = NULL,
  .mount = cmd_fs_tsf_mount,
  .unmount = cmd_fs_noop_unmount,
  .list_begin = cmd_fs_tsf_list_begin,
  .list_next = cmd_fs_tsf_list_next,
  .list_close = cmd_fs_tsf_list_close,
  .stat = cmd_fs_tsf_stat,
  .mkdir = cmd_fs_tsf_mkdir,
  .remove = cmd_fs_tsf_remove,
  .rename = cmd_fs_tsf_rename,
  .open_read = cmd_fs_tsf_open_read,
  .open_write = cmd_fs_tsf_open_write,
  .read = cmd_fs_tsf_read,
  .write = cmd_fs_tsf_write,
  .seek = cmd_fs_tsf_seek,
  .tell = cmd_fs_tsf_tell,
  .close = cmd_fs_tsf_close
};

void cmd_fs_manager_init(CmdFsManager *manager)
{
  if (!manager) return;

  memset(manager, 0, sizeof(*manager));
}

esp_err_t cmd_fs_manager_register_driver(CmdFsManager *manager, CmdFsDriver *driver)
{
  uint32_t bit;

  if (!manager || !driver) return ESP_ERR_INVALID_ARG;
  if (!cmd_fs_is_valid_device_id(driver->device_id)) return CMD_ERR_INVALID_DEVICE;
  if (!driver->root_path || !driver->root_path[0]) return ESP_ERR_INVALID_ARG;

  bit = 1u << (uint32_t)driver->device_id;
  if (manager->registered_mask & bit) return ESP_ERR_INVALID_STATE;

  manager->drivers[driver->device_id] = driver;
  manager->registered_mask |= bit;
  return ESP_OK;
}

esp_err_t cmd_fs_manager_unregister_driver(CmdFsManager *manager, cmd_device_id_t device_id)
{
  uint32_t bit;

  if (!manager) return ESP_ERR_INVALID_ARG;
  if (!cmd_fs_is_valid_device_id(device_id)) return CMD_ERR_INVALID_DEVICE;

  bit = 1u << (uint32_t)device_id;
  manager->drivers[device_id] = NULL;
  manager->registered_mask &= ~bit;
  return ESP_OK;
}

CmdFsDriver *cmd_fs_manager_get_driver(CmdFsManager *manager, cmd_device_id_t device_id)
{
  if (!manager) return NULL;
  if (!cmd_fs_is_valid_device_id(device_id)) return NULL;

  return manager->drivers[device_id];
}

const CmdFsDriver *cmd_fs_manager_get_driver_const(const CmdFsManager *manager, cmd_device_id_t device_id)
{
  if (!manager) return NULL;
  if (!cmd_fs_is_valid_device_id(device_id)) return NULL;

  return manager->drivers[device_id];
}

esp_err_t cmd_fs_manager_register_default_drivers(CmdFsManager *manager)
{
  esp_err_t err;

  if (!manager) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_manager_register_driver(manager, &cmd_fs_sd_driver);
  if (err != ESP_OK) return err;

  err = cmd_fs_manager_register_driver(manager, &cmd_fs_fat_driver);
  if (err != ESP_OK) return err;

  return cmd_fs_manager_register_driver(manager, &cmd_fs_tsf_driver);
}

esp_err_t cmd_fs_manager_build_abs_path(const CmdFsManager *manager, cmd_device_id_t device_id, const char *current_path, const char *filename, char *out, size_t out_size)
{
  const CmdFsDriver *driver;
  char tmp[CMD_PATH_MAX];
  esp_err_t err;

  if (!out || out_size == 0) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver_const(manager, device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;

  err = cmd_fs_driver_build_abs_path(driver, current_path, tmp, sizeof(tmp));
  if (err != ESP_OK) return err;

  if (!filename || !filename[0])
    return cmd_fs_copy_path(tmp, out, out_size);

  return cmd_fs_join_path(tmp, filename, out, out_size);
}

esp_err_t cmd_fs_manager_mount(CmdFsManager *manager, cmd_device_id_t device_id)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);
  uint32_t bit;
  esp_err_t err;

  if (!manager) return ESP_ERR_INVALID_ARG;
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->mount) return ESP_ERR_NOT_SUPPORTED;

  bit = 1u << (uint32_t)device_id;
  if (manager->mounted_mask & bit) return ESP_OK;

  if (device_id != CMD_DEVICE_SD)
  {
    err = cmd_fs_check_root_path(driver);
    if (err == ESP_OK)
    {
      manager->mounted_mask |= bit;
      return ESP_OK;
    }
  }

  err = driver->mount(driver);
  if (err == ESP_OK)
    manager->mounted_mask |= bit;

  return err;
}

esp_err_t cmd_fs_manager_unmount(CmdFsManager *manager, cmd_device_id_t device_id)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);
  uint32_t bit;
  esp_err_t err = ESP_OK;

  if (!manager) return ESP_ERR_INVALID_ARG;
  if (!driver) return CMD_ERR_INVALID_DEVICE;

  bit = 1u << (uint32_t)device_id;
  if (!(manager->mounted_mask & bit)) return ESP_OK;

  if (driver->unmount)
    err = driver->unmount(driver);

  if (err == ESP_OK)
    manager->mounted_mask &= ~bit;

  return err;
}

esp_err_t cmd_fs_manager_unmount_all(CmdFsManager *manager)
{
  esp_err_t first_err = ESP_OK;

  if (!manager) return ESP_ERR_INVALID_ARG;

  for (int i = 0; i < CMD_DEVICE_MAX; i++)
  {
    uint32_t bit = 1u << (uint32_t)i;
    esp_err_t err;

    if (!(manager->mounted_mask & bit))
      continue;

    err = cmd_fs_manager_unmount(manager, (cmd_device_id_t)i);
    if (err != ESP_OK && first_err == ESP_OK)
      first_err = err;
  }

  return first_err;
}

esp_err_t cmd_fs_manager_list_begin(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsList *list)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->list_begin) return ESP_ERR_NOT_SUPPORTED;
  return driver->list_begin(driver, path, list);
}

esp_err_t cmd_fs_manager_list_next(CmdFsManager *manager, CmdFsList *list, cmd_file_entry_t *entry)
{
  CmdFsDriver *driver;

  if (!list) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, list->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->list_next) return ESP_ERR_NOT_SUPPORTED;
  return driver->list_next(driver, list, entry);
}

void cmd_fs_manager_list_close(CmdFsManager *manager, CmdFsList *list)
{
  CmdFsDriver *driver;

  if (!manager || !list) return;

  driver = cmd_fs_manager_get_driver(manager, list->device_id);
  if (!driver || !driver->list_close) return;
  driver->list_close(driver, list);
}

esp_err_t cmd_fs_manager_stat(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, cmd_file_entry_t *entry)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->stat) return ESP_ERR_NOT_SUPPORTED;
  return driver->stat(driver, path, entry);
}

esp_err_t cmd_fs_manager_mkdir(CmdFsManager *manager, cmd_device_id_t device_id, const char *path)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->mkdir) return ESP_ERR_NOT_SUPPORTED;
  return driver->mkdir(driver, path);
}

esp_err_t cmd_fs_manager_space_info(CmdFsManager *manager, cmd_device_id_t device_id, CmdFsSpaceInfo *info)
{
  CmdFsDriver *driver;
  esp_err_t err;

  if (!manager || !info) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;

  err = cmd_fs_manager_mount(manager, device_id);
  if (err != ESP_OK) return err;

  return cmd_fs_driver_space_info(driver, info);
}

esp_err_t cmd_fs_manager_remove(CmdFsManager *manager, cmd_device_id_t device_id, const char *path)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->remove) return ESP_ERR_NOT_SUPPORTED;
  return driver->remove(driver, path);
}

esp_err_t cmd_fs_manager_rename(CmdFsManager *manager, cmd_device_id_t device_id, const char *old_path, const char *new_path)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->rename) return ESP_ERR_NOT_SUPPORTED;
  return driver->rename(driver, old_path, new_path);
}

esp_err_t cmd_fs_manager_open_read(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsFile *file)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->open_read) return ESP_ERR_NOT_SUPPORTED;
  return driver->open_read(driver, path, file);
}

esp_err_t cmd_fs_manager_open_write(CmdFsManager *manager, cmd_device_id_t device_id, const char *path, CmdFsFile *file)
{
  CmdFsDriver *driver = cmd_fs_manager_get_driver(manager, device_id);

  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->open_write) return ESP_ERR_NOT_SUPPORTED;
  return driver->open_write(driver, path, file);
}

esp_err_t cmd_fs_manager_read(CmdFsManager *manager, CmdFsFile *file, void *dst, size_t size, size_t *out_size)
{
  CmdFsDriver *driver;

  if (!file) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, file->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->read) return ESP_ERR_NOT_SUPPORTED;
  return driver->read(driver, file, dst, size, out_size);
}

esp_err_t cmd_fs_manager_write(CmdFsManager *manager, CmdFsFile *file, const void *src, size_t size, size_t *out_size)
{
  CmdFsDriver *driver;

  if (!file) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, file->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->write) return ESP_ERR_NOT_SUPPORTED;
  return driver->write(driver, file, src, size, out_size);
}

esp_err_t cmd_fs_manager_seek(CmdFsManager *manager, CmdFsFile *file, int64_t offset, int whence)
{
  CmdFsDriver *driver;

  if (!file) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, file->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->seek) return ESP_ERR_NOT_SUPPORTED;
  return driver->seek(driver, file, offset, whence);
}

esp_err_t cmd_fs_manager_tell(CmdFsManager *manager, CmdFsFile *file, uint64_t *out_pos)
{
  CmdFsDriver *driver;

  if (!file || !out_pos) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, file->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->tell) return ESP_ERR_NOT_SUPPORTED;
  return driver->tell(driver, file, out_pos);
}

esp_err_t cmd_fs_manager_close(CmdFsManager *manager, CmdFsFile *file)
{
  CmdFsDriver *driver;

  if (!file) return ESP_ERR_INVALID_ARG;

  driver = cmd_fs_manager_get_driver(manager, file->device_id);
  if (!driver) return CMD_ERR_INVALID_DEVICE;
  if (!driver->close) return ESP_ERR_NOT_SUPPORTED;
  return driver->close(driver, file);
}

// --------------- cmd_input.cpp ---------------

#ifndef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM UART_NUM_0
#endif

esp_err_t cmd_input_console_begin(CmdInputBackend *backend);
void cmd_input_console_end(CmdInputBackend *backend);
esp_err_t cmd_input_console_poll_event(CmdInputBackend *backend, cmd_event_t *event, uint32_t timeout_ms);

CmdInputBackend g_cmd_input_console_backend =
{
  .name = "console-uart",
  .ctx = NULL,
  .begin = cmd_input_console_begin,
  .end = cmd_input_console_end,
  .poll_event = cmd_input_console_poll_event
};

void cmd_input_event_clear(cmd_event_t *event)
{
  if (!event) return;

  event->type = CMD_EVENT_NONE;
  event->key = CMD_KEY_NONE;
  event->ch = 0;
  event->mods = CMD_KEY_MOD_NONE;
}

void cmd_input_event_key(cmd_event_t *event, cmd_key_t key, uint32_t mods)
{
  if (!event) return;

  event->type = CMD_EVENT_KEY;
  event->key = key;
  event->ch = 0;
  event->mods = mods;
}

void cmd_input_event_char(cmd_event_t *event, uint32_t ch, uint32_t mods)
{
  if (!event) return;

  event->type = CMD_EVENT_KEY;
  event->key = CMD_KEY_CHAR;
  event->ch = ch;
  event->mods = mods;
}

void cmd_input_event_quit(cmd_event_t *event)
{
  if (!event) return;

  event->type = CMD_EVENT_QUIT;
  event->key = CMD_KEY_NONE;
  event->ch = 0;
  event->mods = CMD_KEY_MOD_NONE;
}

esp_err_t cmd_input_begin(CmdInputBackend *backend)
{
  if (!backend || !backend->begin) return ESP_ERR_INVALID_ARG;
  return backend->begin(backend);
}

void cmd_input_end(CmdInputBackend *backend)
{
  if (!backend || !backend->end) return;
  backend->end(backend);
}

esp_err_t cmd_input_poll_event(CmdInputBackend *backend, cmd_event_t *event, uint32_t timeout_ms)
{
  if (!backend || !backend->poll_event || !event) return ESP_ERR_INVALID_ARG;
  return backend->poll_event(backend, event, timeout_ms);
}

CmdInputBackend *cmd_input_console_backend()
{
  return &g_cmd_input_console_backend;
}

CmdInputBackend *cmd_input_default_backend()
{
  return cmd_input_console_backend();
}

TickType_t cmd_input_timeout_to_ticks(uint32_t timeout_ms)
{
  if (timeout_ms == CMD_INPUT_WAIT_FOREVER) return portMAX_DELAY;
  return pdMS_TO_TICKS(timeout_ms);
}

int cmd_input_uart_read_byte(uint8_t *out, uint32_t timeout_ms)
{
  if (!out) return 0;

  return uart_read_bytes(
    (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
    out,
    1,
    cmd_input_timeout_to_ticks(timeout_ms));
}

uint32_t cmd_input_xterm_mods(int value)
{
  switch (value)
  {
    case 2: return CMD_KEY_MOD_SHIFT;
    case 3: return CMD_KEY_MOD_ALT;
    case 4: return CMD_KEY_MOD_SHIFT | CMD_KEY_MOD_ALT;
    case 5: return CMD_KEY_MOD_CTRL;
    case 6: return CMD_KEY_MOD_SHIFT | CMD_KEY_MOD_CTRL;
    case 7: return CMD_KEY_MOD_ALT | CMD_KEY_MOD_CTRL;
    case 8: return CMD_KEY_MOD_SHIFT | CMD_KEY_MOD_ALT | CMD_KEY_MOD_CTRL;
    default: return CMD_KEY_MOD_NONE;
  }
}

uint32_t cmd_input_csi_mods(const char *seq)
{
  int value = 0;
  int have_semicolon = 0;

  if (!seq) return CMD_KEY_MOD_NONE;

  for (size_t i = 0; seq[i]; i++)
  {
    if (seq[i] == ';')
    {
      have_semicolon = 1;
      value = 0;
      continue;
    }

    if (have_semicolon && seq[i] >= '0' && seq[i] <= '9')
      value = value * 10 + (seq[i] - '0');
  }

  if (!have_semicolon) return CMD_KEY_MOD_NONE;
  return cmd_input_xterm_mods(value);
}

int cmd_input_csi_first_number(const char *seq)
{
  int value = 0;

  if (!seq) return 0;

  for (size_t i = 0; seq[i]; i++)
  {
    if (seq[i] < '0' || seq[i] > '9') break;
    value = value * 10 + (seq[i] - '0');
  }

  return value;
}

cmd_key_t cmd_input_csi_tilde_key(int code)
{
  switch (code)
  {
    case 1: return CMD_KEY_HOME;
    case 2: return CMD_KEY_INSERT;
    case 3: return CMD_KEY_DELETE;
    case 4: return CMD_KEY_END;
    case 5: return CMD_KEY_PAGE_UP;
    case 6: return CMD_KEY_PAGE_DOWN;
    case 7: return CMD_KEY_HOME;
    case 8: return CMD_KEY_END;
    case 11: return CMD_KEY_F1;
    case 12: return CMD_KEY_F2;
    case 13: return CMD_KEY_F3;
    case 14: return CMD_KEY_F4;
    case 15: return CMD_KEY_F5;
    case 17: return CMD_KEY_F6;
    case 18: return CMD_KEY_F7;
    case 19: return CMD_KEY_F8;
    case 20: return CMD_KEY_F9;
    case 21: return CMD_KEY_F10;
    case 23: return CMD_KEY_F11;
    case 24: return CMD_KEY_F12;
    default: return CMD_KEY_UNKNOWN;
  }
}

cmd_key_t cmd_input_csi_final_key(char final)
{
  switch (final)
  {
    case 'P': return CMD_KEY_F1;
    case 'Q': return CMD_KEY_F2;
    case 'R': return CMD_KEY_F3;
    case 'S': return CMD_KEY_F4;
    case 'A': return CMD_KEY_UP;
    case 'B': return CMD_KEY_DOWN;
    case 'C': return CMD_KEY_RIGHT;
    case 'D': return CMD_KEY_LEFT;
    case 'H': return CMD_KEY_HOME;
    case 'F': return CMD_KEY_END;
    case 'Z': return CMD_KEY_TAB;
    default: return CMD_KEY_UNKNOWN;
  }
}

cmd_key_t cmd_input_ss3_key(uint8_t c)
{
  switch (c)
  {
    case 'P': return CMD_KEY_F1;
    case 'Q': return CMD_KEY_F2;
    case 'R': return CMD_KEY_F3;
    case 'S': return CMD_KEY_F4;
    case 'A': return CMD_KEY_UP;
    case 'B': return CMD_KEY_DOWN;
    case 'C': return CMD_KEY_RIGHT;
    case 'D': return CMD_KEY_LEFT;
    case 'H': return CMD_KEY_HOME;
    case 'F': return CMD_KEY_END;
    default: return CMD_KEY_UNKNOWN;
  }
}

esp_err_t cmd_input_console_poll_csi(cmd_event_t *event)
{
  char seq[16];
  size_t len = 0;

  memset(seq, 0, sizeof(seq));

  while (len + 1 < sizeof(seq))
  {
    uint8_t c = 0;
    if (cmd_input_uart_read_byte(&c, CMD_INPUT_ESC_TIMEOUT_MS) <= 0)
    {
      cmd_input_event_key(event, CMD_KEY_UNKNOWN, CMD_KEY_MOD_NONE);
      return ESP_OK;
    }

    seq[len++] = (char)c;
    seq[len] = 0;

    if (c >= '@' && c <= '~')
      break;
  }

  char final = seq[len ? len - 1 : 0];
  uint32_t mods = cmd_input_csi_mods(seq);
  cmd_key_t key;

  if (final == '~')
    key = cmd_input_csi_tilde_key(cmd_input_csi_first_number(seq));
  else
    key = cmd_input_csi_final_key(final);

  if (final == 'Z')
    mods |= CMD_KEY_MOD_SHIFT;

  cmd_input_event_key(event, key, mods);
  return ESP_OK;
}

esp_err_t cmd_input_console_poll_ss3(cmd_event_t *event)
{
  uint8_t c = 0;

  if (cmd_input_uart_read_byte(&c, CMD_INPUT_ESC_TIMEOUT_MS) <= 0)
  {
    cmd_input_event_key(event, CMD_KEY_ESC, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  cmd_input_event_key(event, cmd_input_ss3_key(c), CMD_KEY_MOD_NONE);
  return ESP_OK;
}

esp_err_t cmd_input_console_poll_escape(cmd_event_t *event)
{
  uint8_t c = 0;
  esp_err_t err;

  if (cmd_input_uart_read_byte(&c, CMD_INPUT_ESC_TIMEOUT_MS) <= 0)
  {
    cmd_input_event_key(event, CMD_KEY_ESC, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  if (c == 0x1b)
  {
    err = cmd_input_console_poll_escape(event);
    if (err == ESP_OK && event->type == CMD_EVENT_KEY)
      event->mods |= CMD_KEY_MOD_ALT;
    return err;
  }

  if (c == '[')
    return cmd_input_console_poll_csi(event);

  if (c == 'O')
    return cmd_input_console_poll_ss3(event);

  if (c >= 0x20)
  {
    cmd_input_event_char(event, c, CMD_KEY_MOD_ALT);
    return ESP_OK;
  }

  cmd_input_event_key(event, CMD_KEY_ESC, CMD_KEY_MOD_ALT);
  return ESP_OK;
}

esp_err_t cmd_input_console_byte_to_event(uint8_t c, cmd_event_t *event)
{
  if (c == 0x1b)
    return cmd_input_console_poll_escape(event);

  if (c == '\r' || c == '\n')
  {
    cmd_input_event_key(event, CMD_KEY_ENTER, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  if (c == '\t')
  {
    cmd_input_event_key(event, CMD_KEY_TAB, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  if (c == 0x08 || c == 0x7f)
  {
    cmd_input_event_key(event, CMD_KEY_BACKSPACE, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  if (c >= 0x20)
  {
    cmd_input_event_char(event, c, CMD_KEY_MOD_NONE);
    return ESP_OK;
  }

  cmd_input_event_key(event, CMD_KEY_UNKNOWN, CMD_KEY_MOD_NONE);
  return ESP_OK;
}

esp_err_t cmd_input_console_begin(CmdInputBackend *backend)
{
  if (!backend) return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}

void cmd_input_console_end(CmdInputBackend *backend)
{
  if (!backend) return;
}

esp_err_t cmd_input_console_poll_event(CmdInputBackend *backend, cmd_event_t *event, uint32_t timeout_ms)
{
  uint8_t c = 0;

  if (!backend || !event) return ESP_ERR_INVALID_ARG;

  cmd_input_event_clear(event);

  if (cmd_input_uart_read_byte(&c, timeout_ms) <= 0)
    return ESP_ERR_TIMEOUT;

  return cmd_input_console_byte_to_event(c, event);
}

const char *cmd_input_key_name(cmd_key_t key)
{
  switch (key)
  {
    case CMD_KEY_NONE: return "none";
    case CMD_KEY_CHAR: return "char";
    case CMD_KEY_ESC: return "esc";
    case CMD_KEY_ENTER: return "enter";
    case CMD_KEY_BACKSPACE: return "backspace";
    case CMD_KEY_TAB: return "tab";
    case CMD_KEY_UP: return "up";
    case CMD_KEY_DOWN: return "down";
    case CMD_KEY_LEFT: return "left";
    case CMD_KEY_RIGHT: return "right";
    case CMD_KEY_HOME: return "home";
    case CMD_KEY_END: return "end";
    case CMD_KEY_PAGE_UP: return "page_up";
    case CMD_KEY_PAGE_DOWN: return "page_down";
    case CMD_KEY_INSERT: return "insert";
    case CMD_KEY_DELETE: return "delete";
    case CMD_KEY_F1: return "f1";
    case CMD_KEY_F2: return "f2";
    case CMD_KEY_F3: return "f3";
    case CMD_KEY_F4: return "f4";
    case CMD_KEY_F5: return "f5";
    case CMD_KEY_F6: return "f6";
    case CMD_KEY_F7: return "f7";
    case CMD_KEY_F8: return "f8";
    case CMD_KEY_F9: return "f9";
    case CMD_KEY_F10: return "f10";
    case CMD_KEY_F11: return "f11";
    case CMD_KEY_F12: return "f12";
    case CMD_KEY_UNKNOWN: return "unknown";
    default: return "?";
  }
}

// --------------- cmd_panel.cpp ---------------

CmdFsList *g_cmd_panel_reload_list;

esp_err_t cmd_panel_alloc_global_buffers()
{
  if (g_cmd_panel_reload_list) return ESP_OK;

  g_cmd_panel_reload_list = (CmdFsList *)heap_caps_calloc(
    1,
    sizeof(CmdFsList),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!g_cmd_panel_reload_list) return ESP_ERR_NO_MEM;
  return ESP_OK;
}

void cmd_panel_free_global_buffers()
{
  if (!g_cmd_panel_reload_list) return;

  heap_caps_free(g_cmd_panel_reload_list);
  g_cmd_panel_reload_list = NULL;
}

esp_err_t cmd_panel_alloc_entries(CmdPanel *panel, size_t capacity)
{
  if (!panel || capacity == 0) return ESP_ERR_INVALID_ARG;
  if (panel->entries && panel->capacity >= capacity) return ESP_OK;

  cmd_panel_free_entries(panel);

  panel->entries = (cmd_file_entry_t *)heap_caps_calloc(
    capacity,
    sizeof(cmd_file_entry_t),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!panel->entries)
  {
    panel->capacity = 0;
    return ESP_ERR_NO_MEM;
  }

  panel->capacity = capacity;
  return ESP_OK;
}

void cmd_panel_free_entries(CmdPanel *panel)
{
  if (!panel) return;

  if (panel->entries)
    heap_caps_free(panel->entries);

  panel->entries = NULL;
  panel->capacity = 0;
  panel->count = 0;
  panel->selected_count = 0;
  panel->selected_size = 0;
  panel->cursor = 0;
  panel->scroll = 0;
}

size_t cmd_panel_text_len(const char *s)
{
  size_t len = 0;

  if (!s) return 0;

  while (s[len])
    len++;

  return len;
}

void cmd_panel_append_limited(char *out, size_t out_size, const char *text, size_t max_chars)
{
  size_t len;
  size_t pos = 0;

  if (!out || out_size == 0 || !text) return;

  len = cmd_panel_text_len(out);

  while (text[pos] && pos < max_chars && len + 1 < out_size)
  {
    out[len] = text[pos];
    len++;
    pos++;
  }

  out[len] = 0;
}

void cmd_panel_append_text(char *out, size_t out_size, const char *text)
{
  if (!text) return;

  cmd_panel_append_limited(out, out_size, text, cmd_panel_text_len(text));
}

void cmd_panel_make_title(char *out, size_t out_size, const char *device, const char *path)
{
  size_t used;
  size_t reserve = 2;
  size_t path_limit = 0;

  if (!out || out_size == 0) return;

  out[0] = 0;
  cmd_panel_append_text(out, out_size, " ");
  cmd_panel_append_text(out, out_size, device ? device : "?");
  cmd_panel_append_text(out, out_size, ":");

  used = cmd_panel_text_len(out);
  if (out_size > used + reserve)
    path_limit = out_size - used - reserve;

  cmd_panel_append_limited(out, out_size, path ? path : "/", path_limit);
  cmd_panel_append_text(out, out_size, " ");
}

void cmd_panel_make_entry_line(const cmd_file_entry_t *entry, char *out, size_t out_size)
{
  size_t name_limit;

  if (!out || out_size == 0) return;

  out[0] = 0;
  if (!entry) return;

  if (entry->type == CMD_ENTRY_PARENT)
  {
    cmd_panel_append_text(out, out_size, "..");
    return;
  }

  if (entry->type == CMD_ENTRY_DIR || entry->type == CMD_ENTRY_FILE)
  {
    name_limit = out_size > 1 ? out_size - 1 : 0;
    cmd_panel_append_limited(out, out_size, entry->name, name_limit);
    return;
  }

  cmd_panel_append_text(out, out_size, "? ");
  name_limit = out_size > 3 ? out_size - 3 : 0;
  cmd_panel_append_limited(out, out_size, entry->name, name_limit);
}

int cmd_panel_entry_rank(cmd_entry_type_t type)
{
  if (type == CMD_ENTRY_PARENT) return -1;
  if (type == CMD_ENTRY_DIR) return 0;
  if (type == CMD_ENTRY_FILE) return 1;
  return 2;
}

int cmd_panel_entry_compare(const cmd_file_entry_t *a, const cmd_file_entry_t *b)
{
  int ar;
  int br;

  if (!a || !b) return 0;

  ar = cmd_panel_entry_rank(a->type);
  br = cmd_panel_entry_rank(b->type);

  if (ar < br) return -1;
  if (ar > br) return 1;
  return strcmp(a->name, b->name);
}

void cmd_panel_sort_entries(CmdPanel *panel)
{
  size_t first = 0;

  if (!panel || !panel->entries || panel->count < 2) return;

  if (panel->entries[0].type == CMD_ENTRY_PARENT)
    first = 1;

  if (first + 1 >= panel->count) return;

  std::sort(panel->entries + first, panel->entries + panel->count,
    [](const cmd_file_entry_t &a, const cmd_file_entry_t &b)
    {
      return cmd_panel_entry_compare(&a, &b) < 0;
    });
}

bool cmd_panel_find_entry(CmdPanel *panel, const char *name, size_t *out_index)
{
  if (!panel || !name) return false;

  for (size_t i = 0; i < panel->count; i++)
  {
    if (strcmp(panel->entries[i].name, name) == 0)
    {
      if (out_index) *out_index = i;
      return true;
    }
  }

  return false;
}

bool cmd_panel_entry_can_select(const cmd_file_entry_t *entry)
{
  if (!entry) return false;
  if (entry->type == CMD_ENTRY_PARENT) return false;
  if (entry->type != CMD_ENTRY_FILE && entry->type != CMD_ENTRY_DIR) return false;
  return true;
}

void cmd_panel_update_selection_stats(CmdPanel *panel)
{
  if (!panel) return;

  panel->selected_count = 0;
  panel->selected_size = 0;

  if (!panel->entries) return;

  for (size_t i = 0; i < panel->count; i++)
  {
    cmd_file_entry_t *entry = &panel->entries[i];

    if (!cmd_panel_entry_can_select(entry))
    {
      entry->flags &= ~CMD_ENTRY_FLAG_SELECTED;
      continue;
    }

    if (!(entry->flags & CMD_ENTRY_FLAG_SELECTED)) continue;

    panel->selected_count++;
    if (entry->type == CMD_ENTRY_FILE)
      panel->selected_size += entry->size;
  }
}

void cmd_panel_clear_selection(CmdPanel *panel)
{
  if (!panel) return;

  if (panel->entries)
  {
    for (size_t i = 0; i < panel->count; i++)
      panel->entries[i].flags &= ~CMD_ENTRY_FLAG_SELECTED;
  }

  panel->selected_count = 0;
  panel->selected_size = 0;
}

bool cmd_panel_has_selection(const CmdPanel *panel)
{
  if (!panel) return false;
  return panel->selected_count > 0;
}

void cmd_panel_toggle_selection(CmdPanel *panel)
{
  cmd_file_entry_t *entry;

  if (!panel) return;

  if (panel->cursor < panel->count)
  {
    entry = &panel->entries[panel->cursor];
    if (cmd_panel_entry_can_select(entry))
    {
      entry->flags ^= CMD_ENTRY_FLAG_SELECTED;
      cmd_panel_update_selection_stats(panel);
    }
  }

  if (panel->cursor + 1 < panel->count)
    panel->cursor++;

  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_invert_selection(CmdPanel *panel)
{
  if (!panel || !panel->entries) return;

  for (size_t i = 0; i < panel->count; i++)
  {
    cmd_file_entry_t *entry = &panel->entries[i];

    if (!cmd_panel_entry_can_select(entry))
    {
      entry->flags &= ~CMD_ENTRY_FLAG_SELECTED;
      continue;
    }

    entry->flags ^= CMD_ENTRY_FLAG_SELECTED;
  }

  cmd_panel_update_selection_stats(panel);
}

void cmd_panel_clear_entries(CmdPanel *panel)
{
  if (!panel) return;

  panel->count = 0;
  panel->selected_count = 0;
  panel->selected_size = 0;
  panel->flags &= ~CMD_PANEL_FLAG_TRUNCATED;
  panel->cursor = 0;
  panel->scroll = 0;
}

void cmd_panel_init(CmdPanel *panel, CmdFsManager *fs, cmd_device_id_t device_id)
{
  if (!panel) return;

  memset(panel, 0, sizeof(*panel));
  panel->fs = fs;
  panel->device_id = device_id;
  panel->view_rows = CMD_PANEL_DEFAULT_PAGE_STEP;
  panel->active = false;
  panel->flags = CMD_PANEL_FLAG_NONE;
  panel->last_error = ESP_OK;
  cmd_fs_copy_path("/", panel->current_path, sizeof(panel->current_path));
}

void cmd_panel_reset(CmdPanel *panel)
{
  CmdFsManager *fs;
  cmd_device_id_t device_id;
  cmd_file_entry_t *entries;
  size_t capacity;

  if (!panel) return;

  fs = panel->fs;
  device_id = panel->device_id;
  entries = panel->entries;
  capacity = panel->capacity;

  memset(panel, 0, sizeof(*panel));
  panel->entries = entries;
  panel->capacity = capacity;
  panel->fs = fs;
  panel->device_id = device_id;
  panel->view_rows = CMD_PANEL_DEFAULT_PAGE_STEP;
  panel->active = false;
  panel->flags = CMD_PANEL_FLAG_NONE;
  panel->last_error = ESP_OK;
  cmd_fs_copy_path("/", panel->current_path, sizeof(panel->current_path));
}

void cmd_panel_set_active(CmdPanel *panel, bool active)
{
  if (!panel) return;

  panel->active = active;
  if (active)
    panel->flags |= CMD_PANEL_FLAG_ACTIVE;
  else
    panel->flags &= ~CMD_PANEL_FLAG_ACTIVE;
}

bool cmd_panel_is_active(const CmdPanel *panel)
{
  if (!panel) return false;
  return panel->active;
}

esp_err_t cmd_panel_set_path(CmdPanel *panel, const char *path)
{
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;

  err = cmd_fs_copy_path(path, panel->current_path, sizeof(panel->current_path));
  if (err != ESP_OK) return err;

  cmd_panel_clear_selection(panel);
  panel->cursor = 0;
  panel->scroll = 0;
  return ESP_OK;
}

void cmd_panel_clamp_cursor(CmdPanel *panel)
{
  if (!panel) return;

  if (panel->count == 0)
  {
    panel->cursor = 0;
    panel->scroll = 0;
    return;
  }

  if (panel->cursor >= panel->count)
    panel->cursor = panel->count - 1;

  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_ensure_cursor_visible(CmdPanel *panel)
{
  size_t rows;

  if (!panel) return;

  rows = panel->view_rows ? panel->view_rows : CMD_PANEL_DEFAULT_PAGE_STEP;
  if (rows == 0) rows = 1;

  if (panel->cursor < panel->scroll)
    panel->scroll = panel->cursor;

  if (panel->cursor >= panel->scroll + rows)
    panel->scroll = panel->cursor - rows + 1;
}

esp_err_t cmd_panel_reload(CmdPanel *panel)
{
  CmdFsList *list = g_cmd_panel_reload_list;
  cmd_file_entry_t entry;
  size_t old_cursor;
  bool truncated = false;
  esp_err_t err;

  if (!panel || !panel->fs) return ESP_ERR_INVALID_ARG;
  if (!panel->entries || panel->capacity == 0 || !list) return ESP_ERR_NO_MEM;

  memset(list, 0, sizeof(*list));
  memset(&panel->space_info, 0, sizeof(panel->space_info));
  panel->space_info_valid = false;
  panel->space_info_error = ESP_ERR_INVALID_STATE;

  old_cursor = panel->cursor;
  cmd_panel_clear_entries(panel);

  err = cmd_fs_manager_mount(panel->fs, panel->device_id);
  if (err != ESP_OK)
  {
    panel->space_info_valid = true;
    panel->space_info_error = err;
    panel->last_error = err;
    panel->flags |= CMD_PANEL_FLAG_ERROR;
    return err;
  }

  err = cmd_fs_manager_list_begin(panel->fs, panel->device_id, panel->current_path, list);
  if (err != ESP_OK)
  {
    panel->space_info_valid = true;
    panel->space_info_error = err;
    panel->last_error = err;
    panel->flags |= CMD_PANEL_FLAG_ERROR;
    return err;
  }

  for (;;)
  {
    err = cmd_fs_manager_list_next(panel->fs, list, &entry);
    if (err == ESP_ERR_NOT_FOUND) break;
    if (err != ESP_OK)
    {
      cmd_fs_manager_list_close(panel->fs, list);
      panel->space_info_valid = true;
      panel->space_info_error = err;
      panel->last_error = err;
      panel->flags |= CMD_PANEL_FLAG_ERROR;
      return err;
    }

    if (panel->count >= panel->capacity)
    {
      truncated = true;
      continue;
    }

    panel->entries[panel->count] = entry;
    panel->count++;
  }

  cmd_fs_manager_list_close(panel->fs, list);
  cmd_panel_sort_entries(panel);
  cmd_panel_update_selection_stats(panel);

  panel->space_info_error = cmd_fs_manager_space_info(panel->fs, panel->device_id, &panel->space_info);
  panel->space_info_valid = true;

  panel->cursor = old_cursor;
  panel->last_error = truncated ? CMD_ERR_DIR_FULL : ESP_OK;
  panel->flags &= ~CMD_PANEL_FLAG_ERROR;

  if (truncated)
    panel->flags |= CMD_PANEL_FLAG_TRUNCATED;
  else
    panel->flags &= ~CMD_PANEL_FLAG_TRUNCATED;

  cmd_panel_clamp_cursor(panel);
  return ESP_OK;
}

void cmd_panel_cursor_up(CmdPanel *panel)
{
  if (!panel) return;
  if (panel->cursor > 0) panel->cursor--;
  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_cursor_down(CmdPanel *panel)
{
  if (!panel) return;
  if (panel->cursor + 1 < panel->count) panel->cursor++;
  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_cursor_page_up(CmdPanel *panel)
{
  size_t step;

  if (!panel) return;

  step = panel->view_rows ? panel->view_rows : CMD_PANEL_DEFAULT_PAGE_STEP;
  if (step == 0) step = 1;

  if (panel->cursor > step)
    panel->cursor -= step;
  else
    panel->cursor = 0;

  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_cursor_page_down(CmdPanel *panel)
{
  size_t step;

  if (!panel) return;

  step = panel->view_rows ? panel->view_rows : CMD_PANEL_DEFAULT_PAGE_STEP;
  if (step == 0) step = 1;

  panel->cursor += step;
  cmd_panel_clamp_cursor(panel);
}

void cmd_panel_cursor_home(CmdPanel *panel)
{
  if (!panel) return;

  panel->cursor = 0;
  cmd_panel_ensure_cursor_visible(panel);
}

void cmd_panel_cursor_end(CmdPanel *panel)
{
  if (!panel) return;

  if (panel->count > 0)
    panel->cursor = panel->count - 1;
  else
    panel->cursor = 0;

  cmd_panel_ensure_cursor_visible(panel);
}

const cmd_file_entry_t *cmd_panel_get_selected_entry(const CmdPanel *panel)
{
  if (!panel) return NULL;
  if (panel->cursor >= panel->count) return NULL;
  return &panel->entries[panel->cursor];
}

esp_err_t cmd_panel_change_device(CmdPanel *panel, cmd_device_id_t device_id)
{
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;
  if (!cmd_fs_is_valid_device_id(device_id)) return CMD_ERR_INVALID_DEVICE;

  panel->device_id = device_id;
  err = cmd_panel_set_path(panel, "/");
  if (err != ESP_OK) return err;

  return cmd_panel_reload(panel);
}

esp_err_t cmd_panel_go_parent(CmdPanel *panel)
{
  char parent[CMD_PATH_MAX];
  char child_name[CMD_FILE_NAME_MAX];
  char old_path[CMD_PATH_MAX];
  size_t old_cursor;
  size_t old_scroll;
  size_t index;
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;
  if (cmd_fs_path_is_root(panel->current_path)) return ESP_OK;

  err = cmd_fs_copy_path(panel->current_path, old_path, sizeof(old_path));
  if (err != ESP_OK) return err;

  old_cursor = panel->cursor;
  old_scroll = panel->scroll;

  err = cmd_fs_extract_filename(panel->current_path, child_name, sizeof(child_name));
  if (err != ESP_OK) return err;

  err = cmd_fs_parent_path(panel->current_path, parent, sizeof(parent));
  if (err != ESP_OK) return err;

  err = cmd_panel_set_path(panel, parent);
  if (err != ESP_OK) return err;

  err = cmd_panel_reload(panel);
  if (err != ESP_OK)
  {
    cmd_fs_copy_path(old_path, panel->current_path, sizeof(panel->current_path));
    panel->cursor = old_cursor;
    panel->scroll = old_scroll;
    cmd_panel_reload(panel);
    return err;
  }

  if (cmd_panel_find_entry(panel, child_name, &index))
  {
    panel->cursor = index;
    cmd_panel_ensure_cursor_visible(panel);
  }

  return ESP_OK;
}

esp_err_t cmd_panel_enter_selected_dir(CmdPanel *panel)
{
  const cmd_file_entry_t *entry;
  char path[CMD_PATH_MAX];
  char old_path[CMD_PATH_MAX];
  size_t old_cursor;
  size_t old_scroll;
  esp_err_t err;

  if (!panel) return ESP_ERR_INVALID_ARG;

  entry = cmd_panel_get_selected_entry(panel);
  if (!entry) return ESP_ERR_NOT_FOUND;

  if (entry->type == CMD_ENTRY_PARENT)
    return cmd_panel_go_parent(panel);

  if (entry->type != CMD_ENTRY_DIR) return ESP_ERR_NOT_SUPPORTED;

  err = cmd_fs_copy_path(panel->current_path, old_path, sizeof(old_path));
  if (err != ESP_OK) return err;

  old_cursor = panel->cursor;
  old_scroll = panel->scroll;

  err = cmd_fs_join_path(panel->current_path, entry->name, path, sizeof(path));
  if (err != ESP_OK) return err;

  err = cmd_panel_set_path(panel, path);
  if (err != ESP_OK) return err;

  err = cmd_panel_reload(panel);
  if (err != ESP_OK)
  {
    cmd_fs_copy_path(old_path, panel->current_path, sizeof(panel->current_path));
    panel->cursor = old_cursor;
    panel->scroll = old_scroll;
    cmd_panel_reload(panel);
    return err;
  }

  return ESP_OK;
}

void cmd_panel_draw_text_line(CmdDisplayBuffer *buffer, const cmd_rect_t *line_rect, const char *text, cmd_display_attr_t attr)
{
  if (!buffer || !line_rect) return;

  cmd_display_buffer_fill_rect(buffer, line_rect, ' ', attr);
  if (text)
    cmd_display_buffer_write_text(buffer, line_rect->x, line_rect->y, text, attr);
}

void cmd_panel_draw_title(CmdPanel *panel, CmdDisplayBuffer *buffer, const cmd_rect_t *rect, cmd_display_attr_t attr)
{
  char title[CMD_DISPLAY_COLS + 1];
  char selected[48];
  const char *device;

  if (!panel || !buffer || !rect) return;

  device = cmd_device_name(panel->device_id);
  cmd_panel_make_title(title, sizeof(title), device, panel->current_path);

  if (panel->selected_count > 0)
  {
    snprintf(selected, sizeof(selected), " [%u/%llu] ",
      (unsigned)panel->selected_count,
      (unsigned long long)panel->selected_size);
    cmd_panel_append_text(title, sizeof(title), selected);
  }

  cmd_display_buffer_write_text(buffer, rect->x + 2, rect->y, title, attr);
}

void cmd_panel_draw_error(CmdPanel *panel, CmdDisplayBuffer *buffer, const cmd_rect_t *rect)
{
  char msg[CMD_DISPLAY_COLS + 1];
  cmd_rect_t line_rect;

  if (!panel || !buffer || !rect) return;
  if (rect->w <= 2 || rect->h <= 2) return;

  line_rect.x = rect->x + 1;
  line_rect.y = rect->y + 1;
  line_rect.w = rect->w - 2;
  line_rect.h = 1;

  snprintf(msg, sizeof(msg), "error: 0x%x", (unsigned)panel->last_error);
  cmd_panel_draw_text_line(buffer,
    &line_rect,
    msg,
    cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_RED, CMD_DISPLAY_COLOR_BLUE, CMD_DISPLAY_ATTR_FLAG_BOLD));
}

esp_err_t cmd_panel_draw(CmdPanel *panel, CmdDisplayBuffer *buffer, const cmd_rect_t *rect)
{
  cmd_display_attr_t normal_attr;
  cmd_display_attr_t active_attr;
  cmd_display_attr_t selected_attr;
  cmd_display_attr_t marked_attr;
  cmd_display_attr_t folder_attr;
  cmd_display_attr_t frame_attr;
  cmd_rect_t inner;
  size_t rows;

  if (!panel || !buffer || !rect) return ESP_ERR_INVALID_ARG;
  if (rect->w <= 0 || rect->h <= 0) return ESP_OK;

  normal_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_WHITE, CMD_DISPLAY_COLOR_BLUE, CMD_DISPLAY_ATTR_FLAG_NONE);
  active_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_WHITE, CMD_DISPLAY_COLOR_BLUE, CMD_DISPLAY_ATTR_FLAG_NONE);
  selected_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BLACK, CMD_DISPLAY_COLOR_BRIGHT_YELLOW,
    CMD_DISPLAY_ATTR_FLAG_BOLD | CMD_DISPLAY_ATTR_FLAG_SELECTED);
  marked_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_WHITE, CMD_DISPLAY_COLOR_CYAN, CMD_DISPLAY_ATTR_FLAG_BOLD);
  folder_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_WHITE, CMD_DISPLAY_COLOR_BLUE, CMD_DISPLAY_ATTR_FLAG_BOLD);
  frame_attr = cmd_display_make_attr(CMD_DISPLAY_COLOR_BRIGHT_WHITE, CMD_DISPLAY_COLOR_BLUE, CMD_DISPLAY_ATTR_FLAG_BOLD);

  cmd_display_buffer_fill_rect(buffer, rect, ' ', panel->active ? active_attr : normal_attr);
  cmd_display_buffer_draw_box(buffer, rect, frame_attr);
  cmd_panel_draw_title(panel, buffer, rect, frame_attr);

  if (rect->w <= 2 || rect->h <= 2) return ESP_OK;

  inner.x = rect->x + 1;
  inner.y = rect->y + 1;
  inner.w = rect->w - 2;
  inner.h = rect->h - 2;
  rows = (size_t)inner.h;
  panel->view_rows = rows;
  cmd_panel_ensure_cursor_visible(panel);

  if (panel->flags & CMD_PANEL_FLAG_ERROR)
  {
    cmd_panel_draw_error(panel, buffer, rect);
    return ESP_OK;
  }

  for (size_t row = 0; row < rows; row++)
  {
    size_t idx = panel->scroll + row;
    cmd_rect_t line_rect;
    cmd_display_attr_t attr;
    char line[CMD_DISPLAY_COLS + 1];

    line_rect.x = inner.x;
    line_rect.y = inner.y + (int)row;
    line_rect.w = inner.w;
    line_rect.h = 1;
    if (panel->active && idx == panel->cursor)
      attr = selected_attr;
    else if (idx < panel->count && (panel->entries[idx].flags & CMD_ENTRY_FLAG_SELECTED))
      attr = marked_attr;
    else if (idx < panel->count && (panel->entries[idx].type == CMD_ENTRY_DIR || panel->entries[idx].type == CMD_ENTRY_PARENT))
      attr = folder_attr;
    else
      attr = panel->active ? active_attr : normal_attr;

    if (idx < panel->count)
    {
      size_t line_size = sizeof(line);
      if (line_rect.w > 2 && (size_t)(line_rect.w - 1) < line_size)
        line_size = (size_t)(line_rect.w - 1);
      cmd_panel_make_entry_line(&panel->entries[idx], line, line_size);
    }
    else
      line[0] = 0;

    cmd_display_buffer_fill_rect(buffer, &line_rect, ' ', attr);

    if (line_rect.w > 1)
      cmd_display_buffer_write_text(buffer, line_rect.x + 1, line_rect.y, line, attr);
    else
      cmd_display_buffer_write_text(buffer, line_rect.x, line_rect.y, line, attr);
  }

  return ESP_OK;
}
