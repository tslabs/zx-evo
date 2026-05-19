#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "mytypes.h"
#include "main.h"
#include "ps2.h"
#include "zx.h"
#include "setup.h"
#include "spiflash.h"
#include "tsspiffs.h"
#include "ff.h"
#include "diskio.h"
#include "pins.h"
#include "spi.h"
#include "depacker_dirty.h"
#include "config.h"

#ifdef SETUP_CONF
#define SETUP_SCREEN_COLS 80
#define SETUP_PANEL_W 40
#define SETUP_PANEL_H 28
#define SETUP_PANEL_LEFT_X 0
#define SETUP_PANEL_RIGHT_X 40
#define SETUP_PANEL_INNER_W 38
#define SETUP_PANEL_VISIBLE_ROWS 26
#define SETUP_PANEL_INFO_ROW 28
#define SETUP_PANEL_HINT_ROW 29
#define SETUP_NAME_MAX 64
#define SETUP_PATH_MAX 127
#define SETUP_TSF_BLOCK_SIZE 4096UL
#define SETUP_TSF_BULK_START 0UL
#define SETUP_COLOR_BLACK 0
#define SETUP_COLOR_BLUE 1
#define SETUP_COLOR_BRIGHT_BLUE 9
#define SETUP_COLOR_BRIGHT_CYAN 13
#define SETUP_COLOR_BRIGHT_RED 10
#define SETUP_COLOR_BRIGHT_GREEN 12
#define SETUP_COLOR_BRIGHT_YELLOW 14
#define SETUP_COLOR_BRIGHT_WHITE 15
#define SETUP_ATTR(ink, paper) ((((paper) & 0x0F) << 4) | ((ink) & 0x0F))
#define SETUP_ATTR_PANEL SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLUE)
#define SETUP_ATTR_FRAME SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLUE)
#define SETUP_ATTR_TITLE SETUP_ATTR(SETUP_COLOR_BRIGHT_YELLOW, SETUP_COLOR_BLUE)
#define SETUP_ATTR_INFO SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLACK)
#define SETUP_ATTR_HINT SETUP_ATTR(SETUP_COLOR_BRIGHT_YELLOW, SETUP_COLOR_BRIGHT_BLUE)
#define SETUP_ATTR_HELP SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLACK)
#define SETUP_ATTR_DIALOG SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLACK)
#define SETUP_ATTR_DIALOG_FRAME SETUP_ATTR(SETUP_COLOR_BRIGHT_WHITE, SETUP_COLOR_BLACK)
#define SETUP_ATTR_DIALOG_NAME SETUP_ATTR(SETUP_COLOR_BRIGHT_CYAN, SETUP_COLOR_BLACK)
#define SETUP_ATTR_INFO_ALERT SETUP_ATTR(SETUP_COLOR_BRIGHT_RED, SETUP_COLOR_BLACK)
#define SETUP_ATTR_DIALOG_ACTIVE SETUP_ATTR(SETUP_COLOR_BLACK, SETUP_COLOR_BRIGHT_YELLOW)
#define SETUP_ATTR_CURSOR_ACTIVE SETUP_ATTR(SETUP_COLOR_BLACK, SETUP_COLOR_BRIGHT_YELLOW)
#define SETUP_ATTR_CURSOR_INACTIVE SETUP_ATTR_PANEL
#define SETUP_BOX_TL 0xC9
#define SETUP_BOX_TR 0xBB
#define SETUP_BOX_BL 0xC8
#define SETUP_BOX_BR 0xBC
#define SETUP_BOX_H 0xCD
#define SETUP_BOX_V 0xBA
#define SETUP_ATTR_BUF_OFS SETUP_SCREEN_COLS
#define SETUP_TEXT_BYTE_ADDR(row, attr, col) \
  (((((u32)SETUP_TEXT_DEFAULT_VPAGE & 1UL) << 13) | ((((u32)(row)) & 0x3FUL) << 7) | (((attr) ? 1UL : 0UL) << 6) | (((u32)(col)) & 0x3FUL)) << 1)
#define SETUP_KEY_EXT 0xE0
#define SETUP_KEY_RELEASE 0xF0
#define SETUP_KEY_TAB 0x0D
#define SETUP_KEY_ESC 0x76
#define SETUP_KEY_ENTER 0x5A
#define SETUP_KEY_SPACE 0x29
#define SETUP_KEY_F1 0x05
#define SETUP_KEY_F2 0x06
#define SETUP_KEY_F5 0x03
#define SETUP_KEY_F6 0x0B
#define SETUP_KEY_F7 0x83
#define SETUP_KEY_F8 0x0A
#define SETUP_KEY_F9 0x01
#define SETUP_KEY_F10 0x09
#define SETUP_KEY_F12 0x07
#define SETUP_KEY_ALT 0x11
#define SETUP_KEY_LSHIFT 0x12
#define SETUP_KEY_RSHIFT 0x59
#define SETUP_KEY_BACKSPACE 0x66
#define SETUP_KEY_UP 0x75
#define SETUP_KEY_DOWN 0x72
#define SETUP_KEY_LEFT 0x6B
#define SETUP_KEY_RIGHT 0x74
#define SETUP_KEY_HOME 0x6C
#define SETUP_KEY_END 0x69
#define SETUP_KEY_PGUP 0x7D
#define SETUP_KEY_PGDN 0x7A
#define SETUP_HELP_X 12
#define SETUP_HELP_Y 1
#define SETUP_HELP_W 56
#define SETUP_HELP_H 27
#define SETUP_FORMAT_X 13
#define SETUP_FORMAT_Y 6
#define SETUP_FORMAT_W 54
#define SETUP_FORMAT_H 17
#define SETUP_FORMAT_BAR_X (SETUP_FORMAT_X + 5)
#define SETUP_FORMAT_BAR_W 44
#define SETUP_FORMAT_FAST 0
#define SETUP_FORMAT_NORMAL 1
#define SETUP_FORMAT_SLOW 2
#define SETUP_FORMAT_STAGE_IDLE 0
#define SETUP_FORMAT_STAGE_ERASE 1
#define SETUP_FORMAT_STAGE_CHECK 2
#define SETUP_FORMAT_STAGE_FORMAT 3
#define SETUP_CHK_STAGE_IDLE 0
#define SETUP_CHK_STAGE_SCAN 1
#define SETUP_CHK_STAGE_CHECK 2
#define SETUP_CHK_STAGE_BLANK 3
#define SETUP_CHK_STAGE_ERASE 4
#define SETUP_CHK_STAGE_VERIFY 5
#define SETUP_CHK_STAGE_FORMAT 6
#define SETUP_CHK_STAGE_REPAIR 7
#define SETUP_CHK_SCAN 0
#define SETUP_CHK_SCAN_FIX 1
#define SETUP_CHK_RETEST 2
#define SETUP_FPGA_X 13
#define SETUP_FPGA_Y 8
#define SETUP_FPGA_W 54
#define SETUP_FPGA_H 11
#define SETUP_FPGA_SET_CURRENT 0
#define SETUP_FPGA_LOAD_NOW 1
#define SETUP_COPY_X 13
#define SETUP_COPY_Y 8
#define SETUP_COPY_W 54
#define SETUP_COPY_H 11
#define SETUP_COPY_BAR_X (SETUP_COPY_X + 5)
#define SETUP_COPY_BAR_W 44
#define SETUP_COPY_ERROR_NONE 0
#define SETUP_COPY_ERROR_EXISTS 1
#define SETUP_COPY_ERROR_NO_SPACE 2
#define SETUP_ROM_X 13
#define SETUP_ROM_Y 4
#define SETUP_ROM_W 54
#define SETUP_ROM_H 19
#define SETUP_ROM_BAR_X (SETUP_ROM_X + 5)
#define SETUP_ROM_BAR_W 44
#define SETUP_ROM_BLOCK_SIZE 0x10000UL
#define SETUP_ROM_ERASE_WAIT_LOOPS 700000UL
#define SETUP_ROM_PROGRAM_WAIT_LOOPS 20000UL
#define SETUP_ROM_ERASE_STABLE_READS 64
#define SETUP_ROM_ERROR_NONE 0
#define SETUP_ROM_ERROR_OPEN 1
#define SETUP_ROM_ERROR_READ 2
#define SETUP_ROM_ERROR_SIZE 3
#define SETUP_ROM_ERROR_ERASE 4
#define SETUP_ROM_ERROR_PROGRAM 5
#define SETUP_ROM_ERROR_VERIFY 6
#define SETUP_ROM_ERROR_CANCEL 7
#define SETUP_ROM_STAGE_CONFIRM 0
#define SETUP_ROM_STAGE_ERASE 1
#define SETUP_ROM_STAGE_PROGRAM 2
#define SETUP_ROM_STAGE_VERIFY 3
#define SETUP_ROM_STAGE_DONE 4
#define SETUP_DELETE_X 13
#define SETUP_DELETE_Y 9
#define SETUP_DELETE_W 54
#define SETUP_DELETE_H 10
#define SETUP_DELETE_BAR_X (SETUP_DELETE_X + 5)
#define SETUP_DELETE_BAR_W 44
#define SETUP_DIALOG_BACKUP_ADDR 0x3FF000UL
#define SETUP_DIALOG_BACKUP_SIZE 4096UL
#define SETUP_HOST_SD_DIR_ADDR 0x3E0000UL
#define SETUP_HOST_TSF_DIR_ADDR 0x3D0000UL
#define SETUP_HOST_TSF_MAP_ADDR 0x3C0000UL
#define SETUP_HOST_TSF_MAP_ENTRY_SIZE 8
#define SETUP_HOST_TSF_MAP_ENTRY_COUNT ((SETUP_HOST_TSF_DIR_ADDR - SETUP_HOST_TSF_MAP_ADDR) / SETUP_HOST_TSF_MAP_ENTRY_SIZE)
#define SETUP_HOST_TSF_MAP_FLAG_VALID 0x01
#define SETUP_HOST_DIR_ENTRY_SIZE 80
#define SETUP_HOST_DIR_ENTRY_COUNT 255
#define SETUP_HOST_DIR_FLAG_VALID 0x01
#define SETUP_HOST_DIR_FLAG_DIR 0x02
#define SETUP_HOST_DIR_FLAG_AUX 0x04
#define SETUP_HOST_DIR_SIZE_OFS 4
#define SETUP_HOST_DIR_NAME_OFS 8
#define SETUP_HOST_DIR_AUX_OFS 76
#define SETUP_EEPROM_X 12
#define SETUP_EEPROM_Y 5
#define SETUP_EEPROM_W 56
#define SETUP_EEPROM_H 18
#define SETUP_MKDIR_ERROR_NONE 0
#define SETUP_MKDIR_ERROR_EMPTY 1
#define SETUP_MKDIR_ERROR_EXISTS 2
#define SETUP_MKDIR_ERROR_CREATE 3
#define SETUP_FPGA_TYPE_NONE 0
#define SETUP_FPGA_TYPE_RBF 1
#define SETUP_FPGA_TYPE_MLZ 2
#define SETUP_COPY_BUF_SIZE 512
#define SETUP_TEXT_BUF_SIZE 20
#define SETUP_STACK_MAGIC 0xA5

typedef struct
{
  u16 free_blocks;
  u16 used_blocks;
  u16 invalid_blocks;
  u16 files;
  u16 broken_files;
  u16 wrong_size_files;
  u16 lost_blocks;
  u16 fixed_blocks;
  u16 failed_blocks;
} SetupTsfScanStats;

SetupPanel setup_panels[2];
u8 setup_active_panel;
u8 setup_key_ext;
u8 setup_key_release;
u8 setup_key_alt;
u8 setup_key_shift;
u8 setup_help_visible;
u8 setup_eeprom_visible;
u8 setup_format_visible;
u8 setup_format_option;
u8 setup_format_check;
u8 setup_format_check_active;
u8 setup_format_progress;
u8 setup_format_running;
u8 setup_format_cancel;
u8 setup_format_stage;
u16 setup_format_free_blocks;
u16 setup_format_invalid_blocks;
u8 setup_chkdsk_visible;
u8 setup_chkdsk_option;
u8 setup_chkdsk_progress;
u8 setup_chkdsk_stage;
u8 setup_chkdsk_running;
u8 setup_chkdsk_cancel;
u8 setup_chkdsk_has_stats;
u8 setup_fpga_visible;
u8 setup_fpga_option;
u8 setup_fpga_type;
u32 setup_fpga_size;
u8 setup_fpga_error;
u8 setup_fpga_halt_enabled;
u8 setup_fpga_run_now_pending;
char setup_fpga_run_now_name[SETUP_NAME_MAX + 1];
u8 setup_copy_visible;
u8 setup_copy_progress;
u8 setup_copy_error;
u8 setup_copy_confirm_visible;
u8 setup_rom_visible;
u8 setup_rom_running;
u8 setup_rom_progress;
u8 setup_rom_error;
u8 setup_rom_stage;
u8 setup_rom_cancel;
u8 setup_rom_panel;
u8 setup_rom_start_block;
u32 setup_rom_size;
u8 setup_rom_id_mfr;
u8 setup_rom_id_dev0;
u8 setup_rom_id_dev1;
u8 setup_rom_id_dev2;
u8 setup_mkdir_visible;
u8 setup_mkdir_error;
u8 setup_delete_visible;
u8 setup_delete_running;
u8 setup_delete_progress;
u8 setup_dialog_backup_valid;
u8 setup_dialog_backup_x;
u8 setup_dialog_backup_y;
u8 setup_dialog_backup_w;
u8 setup_dialog_backup_h;
u16 setup_stack_mark_end;
u16 setup_stack_min_free;
u16 setup_stack_total_free;
SetupTsfScanStats setup_chkdsk_stats;
u8 setup_flash_detected;
u8 setup_flash_jedec_mfr;
u8 setup_flash_jedec_type;
u8 setup_flash_jedec_capacity;
u32 setup_flash_capacity;
u32 setup_tsf_bulk_size;
u16 setup_tsf_total_blocks;
u16 setup_tsf_valid_blocks;
TSF_CONFIG setup_fpga_tsf_cfg;
TSF_VOLUME setup_fpga_tsf_vol;
TSF_FILE setup_fpga_tsf_file;
u8 setup_tsf_mounted;
u8 setup_tsf_error;
FRESULT setup_sd_error;
u8 setup_sd_mounted;
u8 setup_sd_count;
u32 setup_sd_free;
u8 setup_sd_dir_cache_valid;
u8 setup_tsf_dir_cache_valid;
u8 setup_tsf_map_valid;

#define SETUP_WS_ALIGN2(v) (((v) + 1) & ~1)
enum
{
  SETUP_WS_BASE_OFS = SETUP_ATTR_BUF_OFS + SETUP_SCREEN_COLS,
  SETUP_WS_TSF_BUF_OFS = SETUP_WS_BASE_OFS,
  SETUP_WS_TSF_NAME_OFS = SETUP_WS_ALIGN2(SETUP_WS_TSF_BUF_OFS + SETUP_NAME_MAX + 1),
  SETUP_WS_SD_FS_OFS = SETUP_WS_ALIGN2(SETUP_WS_TSF_NAME_OFS + SETUP_NAME_MAX + 1),
  SETUP_WS_SD_DIR_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_FS_OFS + sizeof(FATFS)),
  SETUP_WS_SD_FNO_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_DIR_OFS + sizeof(DIR)),
  SETUP_WS_SD_PATH_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_FNO_OFS + sizeof(FILINFO)),
  SETUP_WS_SD_NAME_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_PATH_OFS + SETUP_PATH_MAX + 1),
  SETUP_WS_OP_PATH_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_NAME_OFS + SETUP_NAME_MAX + 1),
  SETUP_WS_COPY_BUF_OFS = SETUP_WS_ALIGN2(SETUP_WS_OP_PATH_OFS + SETUP_PATH_MAX + 1),
  SETUP_WS_SD_FILE_OFS = SETUP_WS_ALIGN2(SETUP_WS_COPY_BUF_OFS + SETUP_COPY_BUF_SIZE),
  SETUP_WS_TSF_FILE_OFS = SETUP_WS_ALIGN2(SETUP_WS_SD_FILE_OFS + sizeof(FIL)),
  SETUP_WS_TSF_CFG_OFS = SETUP_WS_ALIGN2(SETUP_WS_TSF_FILE_OFS + sizeof(TSF_FILE)),
  SETUP_WS_TSF_VOL_OFS = SETUP_WS_ALIGN2(SETUP_WS_TSF_CFG_OFS + sizeof(TSF_CONFIG)),
  SETUP_WS_DELETE_NAME_OFS = SETUP_WS_ALIGN2(SETUP_WS_TSF_VOL_OFS + sizeof(TSF_VOLUME)),
  SETUP_WS_TEXT_BUF_OFS = SETUP_WS_ALIGN2(SETUP_WS_DELETE_NAME_OFS + SETUP_NAME_MAX + 1),
  SETUP_WS_DELETE_PANEL_OFS = SETUP_WS_ALIGN2(SETUP_WS_TEXT_BUF_OFS + SETUP_TEXT_BUF_SIZE),
  SETUP_WS_DELETE_IS_DIR_OFS = SETUP_WS_ALIGN2(SETUP_WS_DELETE_PANEL_OFS + 1),
  SETUP_WS_END_OFS = SETUP_WS_ALIGN2(SETUP_WS_DELETE_IS_DIR_OFS + 1)
};

typedef char SetupWorkspaceSizeCheck[(SETUP_WS_END_OFS <= 2048) ? 1 : -1];
typedef char SetupDialogBackupSizeCheck[((SETUP_HELP_W * SETUP_HELP_H * 2UL) <= SETUP_DIALOG_BACKUP_SIZE) ? 1 : -1];

#define setup_tsf_buf ((u8*)(void*)(dbuf + SETUP_WS_TSF_BUF_OFS))
#define setup_tsf_name_buf ((char*)(void*)(dbuf + SETUP_WS_TSF_NAME_OFS))
#define setup_sd_fs (*(FATFS*)(void*)(dbuf + SETUP_WS_SD_FS_OFS))
#define setup_sd_dir (*(DIR*)(void*)(dbuf + SETUP_WS_SD_DIR_OFS))
#define setup_sd_fno (*(FILINFO*)(void*)(dbuf + SETUP_WS_SD_FNO_OFS))
#define setup_sd_path ((char*)(void*)(dbuf + SETUP_WS_SD_PATH_OFS))
#define setup_sd_name_buf ((char*)(void*)(dbuf + SETUP_WS_SD_NAME_OFS))
#define setup_op_path ((char*)(void*)(dbuf + SETUP_WS_OP_PATH_OFS))
#define setup_copy_buf ((u8*)(void*)(dbuf + SETUP_WS_COPY_BUF_OFS))
#define setup_sd_file (*(FIL*)(void*)(dbuf + SETUP_WS_SD_FILE_OFS))
#define setup_tsf_file (*(TSF_FILE*)(void*)(dbuf + SETUP_WS_TSF_FILE_OFS))
#define setup_tsf_cfg (*(TSF_CONFIG*)(void*)(dbuf + SETUP_WS_TSF_CFG_OFS))
#define setup_tsf_vol (*(TSF_VOLUME*)(void*)(dbuf + SETUP_WS_TSF_VOL_OFS))
#define setup_delete_name_buf ((char*)(void*)(dbuf + SETUP_WS_DELETE_NAME_OFS))
#define setup_text_buf ((char*)(void*)(dbuf + SETUP_WS_TEXT_BUF_OFS))
#define setup_delete_panel dbuf[SETUP_WS_DELETE_PANEL_OFS]
#define setup_delete_is_dir dbuf[SETUP_WS_DELETE_IS_DIR_OFS]

u8 setup_any_dialog_visible()
{
  if (setup_help_visible) return 1;
  if (setup_eeprom_visible) return 1;
  if (setup_format_visible) return 1;
  if (setup_chkdsk_visible) return 1;
  if (setup_fpga_visible) return 1;
  if (setup_mkdir_visible) return 1;
  if (setup_copy_confirm_visible) return 1;
  if (setup_copy_visible) return 1;
  if (setup_rom_visible) return 1;
  if (setup_delete_visible) return 1;
  return 0;
}

void setup_put_cell(u8 x, u8 ch, u8 attr)
{
  dbuf[x] = ch;
  dbuf[SETUP_ATTR_BUF_OFS + x] = attr;
}

void setup_clear_line(u8 ch, u8 attr)
{
  u8 i;

  for (i = 0; i < SETUP_SCREEN_COLS; i++)
    setup_put_cell(i, ch, attr);
}

void setup_put_text(u8 x, const char *text, u8 attr)
{
  while ((*text != 0) && (x < SETUP_SCREEN_COLS))
  {
    setup_put_cell(x, (u8)*text, attr);
    x++;
    text++;
  }
}

void setup_put_text_p(u8 x, const char *text, u8 attr)
{
  u8 ch;

  while (x < SETUP_SCREEN_COLS)
  {
    ch = pgm_read_byte(text);
    if (ch == 0) return;
    setup_put_cell(x, ch, attr);
    x++;
    text++;
  }
}

void setup_put_panel_frame(u8 x, u8 w, u8 row)
{
  u8 i;
  u8 right = x + w - 1;

  if (row >= SETUP_PANEL_H) return;

  if (row == 0)
  {
    setup_put_cell(x, SETUP_BOX_TL, SETUP_ATTR_FRAME);
    for (i = 1; i < (w - 1); i++)
      setup_put_cell(x + i, SETUP_BOX_H, SETUP_ATTR_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_FRAME);
    return;
  }

  if (row == (SETUP_PANEL_H - 1))
  {
    setup_put_cell(x, SETUP_BOX_BL, SETUP_ATTR_FRAME);
    for (i = 1; i < (w - 1); i++)
      setup_put_cell(x + i, SETUP_BOX_H, SETUP_ATTR_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_FRAME);
    return;
  }

  setup_put_cell(x, SETUP_BOX_V, SETUP_ATTR_FRAME);
  setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_FRAME);
}

void setup_put_panel_title_free(u8 panel, u8 x, u8 free_x, u32 free_size)
{
  if (panel == 0)
  {
    if (setup_sd_mounted == 0) return;
    free_size = setup_sd_free;
  }
  else
  {
    if (setup_flash_detected == 0) return;
    free_size = setup_tsf_vol.free;
  }

  setup_size_to_text(setup_text_buf, free_size);
  setup_put_text_p(x + free_x, PSTR("Free "), SETUP_ATTR_TITLE);
  setup_put_text(x + free_x + 5, setup_text_buf, SETUP_ATTR_TITLE);
}

void setup_put_titles()
{
  u8 x = 5;
  u8 i = 0;

  setup_put_text_p(1, PSTR(" SD:/"), SETUP_ATTR_TITLE);
  while ((setup_sd_path[i] != 0) && (x < 22))
  {
    setup_put_cell(x, (u8)setup_sd_path[i], SETUP_ATTR_TITLE);
    x++;
    i++;
  }
  setup_put_cell(x, ' ', SETUP_ATTR_TITLE);
  setup_put_panel_title_free(0, SETUP_PANEL_LEFT_X, 22, 0);

  setup_put_text_p(41, PSTR(" TSF:/ "), SETUP_ATTR_TITLE);
  setup_put_panel_title_free(1, SETUP_PANEL_RIGHT_X, 22, 0);
}

void setup_write_row(u8 row)
{
  setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 0, 0), dbuf, SETUP_SCREEN_COLS);
  setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 1, 0), dbuf + SETUP_ATTR_BUF_OFS, SETUP_SCREEN_COLS);
}

void setup_write_row_range(u8 row, u8 x, u8 w)
{
  setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 0, 0) + x, dbuf + x, w);
  setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 1, 0) + x, dbuf + SETUP_ATTR_BUF_OFS + x, w);
}

void setup_write_attr_row_range(u8 row, u8 x, u8 w)
{
  setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 1, 0) + x, dbuf + SETUP_ATTR_BUF_OFS + x, w);
}

void setup_clear_dialog_row(u8 x, u8 w)
{
  u8 i;

  for (i = 0; i < w; i++)
    setup_put_cell(x + i, ' ', SETUP_ATTR_DIALOG);
}

u8 setup_pstr_len(const char *text)
{
  u8 len = 0;

  while (pgm_read_byte(text + len))
    len++;

  return len;
}

void setup_put_dialog_title(u8 x, u8 w, const char *text)
{
  enum
  {
    dialog_title_attr = SETUP_ATTR(SETUP_COLOR_BRIGHT_GREEN, SETUP_COLOR_BLACK)
  };
  u8 len = setup_pstr_len(text);

  if (len >= w) setup_put_text_p(x + 1, text, dialog_title_attr);
  else setup_put_text_p(x + ((w - len) >> 1), text, dialog_title_attr);
}

void setup_draw_dialog_frame_row(u8 x, u8 y, u8 w, u8 h, u8 row)
{
  u8 i;
  u8 rel = row - y;
  u8 right = x + w - 1;

  setup_clear_dialog_row(x, w);

  if (rel == 0)
  {
    setup_put_cell(x, SETUP_BOX_TL, SETUP_ATTR_DIALOG_FRAME);
    for (i = 1; i < (w - 1); i++)
      setup_put_cell(x + i, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_DIALOG_FRAME);
    return;
  }

  if (rel == (h - 1))
  {
    setup_put_cell(x, SETUP_BOX_BL, SETUP_ATTR_DIALOG_FRAME);
    for (i = 1; i < (w - 1); i++)
      setup_put_cell(x + i, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_DIALOG_FRAME);
    return;
  }

  setup_put_cell(x, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
  setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
}

u8 setup_calc_progress(u32 done, u32 total, u8 bar_w)
{
  u32 step;

  if (total == 0) return 0;
  if (done >= total) return bar_w;

  step = (total + bar_w - 1) / bar_w;
  if (step == 0) step = 1;

  done /= step;
  if (done > bar_w) return bar_w;
  return (u8)done;
}

void setup_draw_progress_bar(u8 x, u8 w, u8 progress, u8 empty_attr)
{
  enum
  {
    bar_full = 0xDB,
    bar_empty = 0xB0
  };
  u8 i;

  setup_put_cell(x - 1, '[', SETUP_ATTR_DIALOG_FRAME);
  for (i = 0; i < w; i++)
  {
    if (i < progress)
      setup_put_cell(x + i, bar_full, SETUP_ATTR_DIALOG_FRAME);
    else
      setup_put_cell(x + i, bar_empty, empty_attr);
  }
  setup_put_cell(x + w, ']', SETUP_ATTR_DIALOG_FRAME);
}

void setup_redraw_progress_row(u8 row, u8 box_x, u8 box_w, u8 bar_x, u8 bar_w, u8 progress, u8 empty_attr)
{
  setup_clear_dialog_row(box_x, box_w);
  setup_put_cell(box_x, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
  setup_put_cell(box_x + box_w - 1, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
  setup_draw_progress_bar(bar_x, bar_w, progress, empty_attr);
  setup_write_row_range(row, box_x, box_w);
}

void setup_dialog_backup_save(u8 x, u8 y, u8 w, u8 h)
{
  u8 row;
  u32 addr = SETUP_DIALOG_BACKUP_ADDR;

  setup_dialog_backup_valid = 0;
  setup_dialog_backup_x = x;
  setup_dialog_backup_y = y;
  setup_dialog_backup_w = w;
  setup_dialog_backup_h = h;

  for (row = y; row < (y + h); row++)
  {
    setup_spi_dram_read_block(SETUP_TEXT_BYTE_ADDR(row, 0, 0) + x, dbuf, w);
    setup_spi_dram_write_block(addr, dbuf, w);
    addr += w;
    setup_spi_dram_read_block(SETUP_TEXT_BYTE_ADDR(row, 1, 0) + x, dbuf, w);
    setup_spi_dram_write_block(addr, dbuf, w);
    addr += w;
  }

  setup_dialog_backup_valid = 1;
}

void setup_dialog_backup_save_drawn_row(u8 row)
{
  u32 addr;

  if (setup_dialog_backup_w == 0) return;
  if (row < setup_dialog_backup_y) return;
  if (row >= (setup_dialog_backup_y + setup_dialog_backup_h)) return;

  addr = SETUP_DIALOG_BACKUP_ADDR + ((u32)(row - setup_dialog_backup_y) * (u32)setup_dialog_backup_w * 2UL);
  setup_spi_dram_write_block(addr, dbuf + setup_dialog_backup_x, setup_dialog_backup_w);
  addr += setup_dialog_backup_w;
  setup_spi_dram_write_block(addr, dbuf + SETUP_ATTR_BUF_OFS + setup_dialog_backup_x, setup_dialog_backup_w);
}

void setup_dialog_backup_restore()
{
  u8 row;
  u32 addr;

  if (setup_dialog_backup_valid == 0) return;

  addr = SETUP_DIALOG_BACKUP_ADDR;
  for (row = setup_dialog_backup_y; row < (setup_dialog_backup_y + setup_dialog_backup_h); row++)
  {
    setup_spi_dram_read_block(addr, dbuf, setup_dialog_backup_w);
    setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 0, 0) + setup_dialog_backup_x, dbuf, setup_dialog_backup_w);
    addr += setup_dialog_backup_w;
    setup_spi_dram_read_block(addr, dbuf, setup_dialog_backup_w);
    setup_spi_dram_write_block(SETUP_TEXT_BYTE_ADDR(row, 1, 0) + setup_dialog_backup_x, dbuf, setup_dialog_backup_w);
    addr += setup_dialog_backup_w;
  }

  setup_dialog_backup_valid = 0;
}

void setup_dialog_backup_restore_or_redraw(u8 x, u8 y, u8 w, u8 h)
{
  if (setup_dialog_backup_valid)
  {
    setup_dialog_backup_restore();
    return;
  }

  setup_draw_panels_area(x, y, w, h);
}

void setup_dialog_backup_save_if_needed(u8 x, u8 y, u8 w, u8 h)
{
  if (setup_dialog_backup_valid)
  {
    if ((setup_dialog_backup_x == x) && (setup_dialog_backup_y == y) &&
        (setup_dialog_backup_w == w) && (setup_dialog_backup_h == h)) return;
    setup_dialog_backup_restore();
  }

  setup_dialog_backup_save(x, y, w, h);
}

u8 setup_dialog_get_rect(u8 *x, u8 *y, u8 *w, u8 *h)
{
  if (setup_help_visible)
  {
    *x = SETUP_HELP_X;
    *y = SETUP_HELP_Y;
    *w = SETUP_HELP_W;
    *h = SETUP_HELP_H;
    return 1;
  }

  if (setup_eeprom_visible)
  {
    *x = SETUP_EEPROM_X;
    *y = SETUP_EEPROM_Y;
    *w = SETUP_EEPROM_W;
    *h = SETUP_EEPROM_H;
    return 1;
  }

  if (setup_format_visible || setup_chkdsk_visible)
  {
    *x = SETUP_FORMAT_X;
    *y = SETUP_FORMAT_Y;
    *w = SETUP_FORMAT_W;
    *h = SETUP_FORMAT_H;
    return 1;
  }

  if (setup_fpga_visible)
  {
    *x = SETUP_FPGA_X;
    *y = SETUP_FPGA_Y;
    *w = SETUP_FPGA_W;
    *h = SETUP_FPGA_H;
    return 1;
  }

  if (setup_mkdir_visible || setup_copy_confirm_visible || setup_delete_visible)
  {
    *x = SETUP_DELETE_X;
    *y = SETUP_DELETE_Y;
    *w = SETUP_DELETE_W;
    *h = SETUP_DELETE_H;
    return 1;
  }

  if (setup_copy_visible)
  {
    *x = SETUP_COPY_X;
    *y = SETUP_COPY_Y;
    *w = SETUP_COPY_W;
    *h = SETUP_COPY_H;
    return 1;
  }

  if (setup_rom_visible)
  {
    *x = SETUP_ROM_X;
    *y = SETUP_ROM_Y;
    *w = SETUP_ROM_W;
    *h = SETUP_ROM_H;
    return 1;
  }

  return 0;
}

void setup_workspace_init()
{
  u16 i;

  for (i = SETUP_WS_BASE_OFS; i < SETUP_WS_END_OFS; i++)
    dbuf[i] = 0;
}

extern "C" char __heap_start;

void setup_stack_debug_init()
{
  enum
  {
    stack_guard = 32
  };
  u8 *p;
  u16 start;
  u16 end;
  u16 sp;

  start = (u16)&__heap_start;
  sp = SPL;
  sp |= ((u16)SPH) << 8;
  setup_stack_mark_end = 0;
  setup_stack_min_free = 0;
  setup_stack_total_free = ((u16)RAMEND + 1) - start;

  if (sp <= (start + stack_guard)) return;

  end = sp - stack_guard;
  setup_stack_mark_end = end;
  setup_stack_min_free = end - start;

  for (p = (u8*)start; (u16)p < end; p++)
    *p = SETUP_STACK_MAGIC;
}

void setup_stack_debug_update()
{
  u8 *p;
  u16 start;
  u16 free;

  if (setup_stack_mark_end == 0) return;

  start = (u16)&__heap_start;
  p = (u8*)start;

  while (((u16)p < setup_stack_mark_end) && (*p == SETUP_STACK_MAGIC))
    p++;

  free = (u16)p - start;
  if (free < setup_stack_min_free) setup_stack_min_free = free;
}

void setup_draw_stack_debug()
{
  u16 used;
  u8 len;
  u8 used_len;

  setup_stack_debug_update();

  if (setup_stack_min_free >= setup_stack_total_free) used = 0;
  else used = setup_stack_total_free - setup_stack_min_free;

  len = setup_u32_to_dec(setup_text_buf, setup_stack_total_free);
  setup_text_buf[len++] = '/';
  used_len = setup_u32_to_dec(setup_text_buf + len, used);
  len += used_len;

  setup_put_text(SETUP_SCREEN_COLS - len, setup_text_buf, SETUP_ATTR_HINT);
}


u8 setup_tsf_map_addr_to_block(u32 addr, u16 *block, u32 *block_ofs)
{
  u32 ofs;

  if (block) *block = 0;
  if (block_ofs) *block_ofs = 0;
  if (setup_tsf_cfg.block_size == 0) return 0;
  if (addr < setup_tsf_cfg.bulk_start) return 0;

  ofs = addr - setup_tsf_cfg.bulk_start;
  if (ofs >= setup_tsf_cfg.bulk_size) return 0;

  if (block) *block = (u16)(ofs / setup_tsf_cfg.block_size);
  if (block_ofs) *block_ofs = ofs % setup_tsf_cfg.block_size;
  return 1;
}

void setup_tsf_map_chunk_to_entry(const TSF_CHUNK *chunk, u8 *entry)
{
  entry[0] = (u8)chunk->magic;
  entry[1] = (u8)(chunk->magic >> 8);
  entry[2] = (u8)(chunk->magic >> 16);
  entry[3] = (u8)(chunk->magic >> 24);
  entry[4] = (u8)chunk->next_chunk;
  entry[5] = (u8)(chunk->next_chunk >> 8);
  entry[6] = chunk->type;
  entry[7] = SETUP_HOST_TSF_MAP_FLAG_VALID;
}

void setup_tsf_map_entry_to_chunk(const u8 *entry, TSF_CHUNK *chunk)
{
  chunk->magic = (u32)entry[0] |
                 ((u32)entry[1] << 8) |
                 ((u32)entry[2] << 16) |
                 ((u32)entry[3] << 24);
  chunk->next_chunk = (u16)entry[4] | ((u16)entry[5] << 8);
  chunk->type = entry[6];
}

void setup_tsf_map_store_chunk(u16 block, const TSF_CHUNK *chunk)
{
  u8 entry[SETUP_HOST_TSF_MAP_ENTRY_SIZE];

  if (block >= SETUP_HOST_TSF_MAP_ENTRY_COUNT) return;
  setup_tsf_map_chunk_to_entry(chunk, entry);
  setup_spi_dram_write_block((SETUP_HOST_TSF_MAP_ADDR + ((u32)block * SETUP_HOST_TSF_MAP_ENTRY_SIZE)), entry, SETUP_HOST_TSF_MAP_ENTRY_SIZE);
}

u8 setup_tsf_map_load_chunk(u16 block, TSF_CHUNK *chunk)
{
  u8 entry[SETUP_HOST_TSF_MAP_ENTRY_SIZE];

  if (setup_tsf_map_valid == 0) return 0;
  if (block >= SETUP_HOST_TSF_MAP_ENTRY_COUNT) return 0;

  setup_spi_dram_read_block((SETUP_HOST_TSF_MAP_ADDR + ((u32)block * SETUP_HOST_TSF_MAP_ENTRY_SIZE)), entry, SETUP_HOST_TSF_MAP_ENTRY_SIZE);
  if ((entry[7] & SETUP_HOST_TSF_MAP_FLAG_VALID) == 0) return 0;

  setup_tsf_map_entry_to_chunk(entry, chunk);
  return 1;
}

void setup_tsf_map_mark_erased_block(u16 block)
{
  TSF_CHUNK chunk;

  chunk.magic = 0xFFFFFFFFUL;
  chunk.next_chunk = 0xFFFF;
  chunk.type = 0xFF;
  setup_tsf_map_store_chunk(block, &chunk);
}

void setup_tsf_map_mark_erased_range(u32 addr, u32 size)
{
  u16 block;
  u32 block_ofs;
  u32 count;

  if (setup_tsf_map_valid == 0) return;
  if (setup_tsf_map_addr_to_block(addr, &block, &block_ofs) == 0) return;
  if (block_ofs != 0) return;

  count = (size + setup_tsf_cfg.block_size - 1) / setup_tsf_cfg.block_size;
  while ((count > 0) && (block < SETUP_HOST_TSF_MAP_ENTRY_COUNT))
  {
    setup_tsf_map_mark_erased_block(block);
    block++;
    count--;
  }
}

void setup_tsf_map_update_write(u32 addr, const void *src, u32 size)
{
  u8 entry[SETUP_HOST_TSF_MAP_ENTRY_SIZE];
  u16 block;
  u32 block_ofs;
  u8 i;
  u8 update;
  const u8 *p = (const u8*)src;

  if (setup_tsf_map_valid == 0) return;
  if (size == 0) return;
  if (setup_tsf_map_addr_to_block(addr, &block, &block_ofs) == 0) return;
  if (block >= SETUP_HOST_TSF_MAP_ENTRY_COUNT) return;
  if (block_ofs >= sizeof(TSF_CHUNK)) return;

  update = (u8)(sizeof(TSF_CHUNK) - block_ofs);
  if (update > size) update = (u8)size;

  setup_spi_dram_read_block((SETUP_HOST_TSF_MAP_ADDR + ((u32)block * SETUP_HOST_TSF_MAP_ENTRY_SIZE)), entry, SETUP_HOST_TSF_MAP_ENTRY_SIZE);
  if ((entry[7] & SETUP_HOST_TSF_MAP_FLAG_VALID) == 0) return;

  for (i = 0; i < update; i++)
    entry[block_ofs + i] = p[i];

  setup_spi_dram_write_block((SETUP_HOST_TSF_MAP_ADDR + ((u32)block * SETUP_HOST_TSF_MAP_ENTRY_SIZE)), entry, SETUP_HOST_TSF_MAP_ENTRY_SIZE);
}

TSF_RESULT setup_tsf_flash_read_raw(u32 addr, void *dst, u32 size)
{
  u8 *p = (u8*)dst;

  setup_tsf_wait_ready();
  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_READ);

  while (size > 0)
  {
    *p++ = spi_flash_read(SPIFL_REG_DATA);
    size--;
  }

  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
  return TSF_RES_OK;
}

u8 setup_tsf_rebuild_block_map()
{
  TSF_CHUNK chunk;
  u32 off;
  u16 block = 0;

  setup_tsf_map_valid = 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_tsf_total_blocks == 0) return 0;
  if (setup_tsf_total_blocks > SETUP_HOST_TSF_MAP_ENTRY_COUNT) return 0;

  for (off = 0; off < setup_tsf_cfg.bulk_size; off += setup_tsf_cfg.block_size)
  {
    setup_tsf_flash_read_raw(setup_tsf_cfg.bulk_start + off, &chunk, sizeof(chunk));
    setup_tsf_map_store_chunk(block, &chunk);
    block++;
    if (block >= setup_tsf_total_blocks) break;
  }

  setup_tsf_map_valid = 1;
  return 1;
}

void setup_flash_detect()
{
  setup_flash_detected = 0;
  setup_flash_jedec_mfr = 0;
  setup_flash_jedec_type = 0;
  setup_flash_jedec_capacity = 0;
  setup_flash_capacity = 0;
  setup_tsf_bulk_size = 0;
  setup_tsf_total_blocks = 0;
  setup_tsf_valid_blocks = 0;
  setup_tsf_map_valid = 0;

  sfi_enable();
  setup_tsf_wait_ready();

  sfi_cs_on();
  sfi_send(SF_CMD_RDID2);
  setup_flash_jedec_mfr = sfi_recv();
  setup_flash_jedec_type = sfi_recv();
  setup_flash_jedec_capacity = sfi_recv();
  sfi_cs_off();

  if ((setup_flash_jedec_mfr == 0x00) || (setup_flash_jedec_mfr == 0xFF)) return;
  if ((setup_flash_jedec_type == 0x00) || (setup_flash_jedec_type == 0xFF)) return;
  if ((setup_flash_jedec_capacity < 12) || (setup_flash_jedec_capacity > 27)) return;

  setup_flash_capacity = 1UL << setup_flash_jedec_capacity;
  setup_tsf_bulk_size = setup_flash_capacity;
  setup_tsf_total_blocks = (u16)(setup_tsf_bulk_size / SETUP_TSF_BLOCK_SIZE);
  if (setup_tsf_total_blocks == 0) return;
  setup_flash_detected = 1;
}

void setup_tsf_set_flash_addr(u32 addr)
{
  spi_flash_write(SPIFL_REG_A0, (u8)addr);
  spi_flash_write(SPIFL_REG_A1, (u8)(addr >> 8));
  spi_flash_write(SPIFL_REG_A2, (u8)(addr >> 16));
}

void setup_tsf_wait_ready()
{
  while (spi_flash_read(SPIFL_REG_STAT) & SPIFL_STAT_BUSY)
  {
  }
}

TSF_RESULT setup_tsf_hal_read(u32 addr, void *dst, u32 size)
{
  u16 block;
  u32 block_ofs;

  if ((size == sizeof(TSF_CHUNK)) && setup_tsf_map_addr_to_block(addr, &block, &block_ofs) && (block_ofs == 0))
  {
    if (setup_tsf_map_load_chunk(block, (TSF_CHUNK*)dst)) return TSF_RES_OK;
  }

  return setup_tsf_flash_read_raw(addr, dst, size);
}

TSF_RESULT setup_tsf_hal_write(u32 addr, const void *src, u32 size)
{
  const u8 *p = (const u8*)src;
  u16 chunk;
  u16 i;

  while (size > 0)
  {
    u32 page_addr = addr;
    const u8 *page = p;

    chunk = 256 - (u8)addr;
    if (chunk > size) chunk = (u16)size;

    setup_tsf_wait_ready();
    setup_tsf_set_flash_addr(addr);
    spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_WRITE);

    for (i = 0; i < chunk; i++)
      spi_flash_write(SPIFL_REG_DATA, *p++);

    spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
    setup_tsf_wait_ready();
    setup_tsf_map_update_write(page_addr, page, chunk);

    addr += chunk;
    size -= chunk;
  }

  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_hal_erase_cmd(u32 addr, u8 cmd)
{
  setup_tsf_wait_ready();
  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, cmd);
  setup_tsf_wait_ready();
  setup_tsf_map_mark_erased_range(addr, SETUP_TSF_BLOCK_SIZE);
  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_hal_erase(u32 addr)
{
  return setup_tsf_hal_erase_cmd(addr, SPIFL_CMD_ERSSEC);
}

TSF_RESULT setup_tsf_hal_erase_range(u32 addr, u32 size)
{
  if ((addr == SETUP_TSF_BULK_START) && (size == setup_tsf_bulk_size))
  {
    setup_tsf_wait_ready();
    spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_ERSBLK);
    setup_tsf_wait_ready();
    setup_tsf_map_mark_erased_range(addr, size);
    return TSF_RES_OK;
  }

  while (size > 0)
  {
    if (setup_tsf_hal_erase(addr) != TSF_RES_OK) return TSF_RES_FS_ERROR;
    addr += SETUP_TSF_BLOCK_SIZE;
    if (size < SETUP_TSF_BLOCK_SIZE) break;
    size -= SETUP_TSF_BLOCK_SIZE;
  }

  return TSF_RES_OK;
}

void setup_tsf_config_init()
{
  setup_tsf_cfg.bulk_start = SETUP_TSF_BULK_START;
  setup_tsf_cfg.bulk_size = setup_tsf_bulk_size;
  setup_tsf_cfg.block_size = SETUP_TSF_BLOCK_SIZE;
  setup_tsf_cfg.last_written_chunk = 0;
  setup_tsf_cfg.hal_read_func = setup_tsf_hal_read;
  setup_tsf_cfg.hal_write_func = setup_tsf_hal_write;
  setup_tsf_cfg.hal_erase_func = setup_tsf_hal_erase;
  setup_tsf_cfg.hal_erase_range_func = setup_tsf_hal_erase_range;
  setup_tsf_cfg.poll_func = 0;
  setup_tsf_cfg.progress_func = 0;
  setup_tsf_cfg.buf = setup_tsf_buf;
  setup_tsf_cfg.buf_size = SETUP_NAME_MAX + 1;
}

void setup_tsf_mount_volume()
{
  TSF_RESULT rc;

  setup_tsf_mounted = 0;
  setup_tsf_dir_cache_valid = 0;
  setup_tsf_map_valid = 0;
  setup_tsf_error = 0;
  setup_tsf_valid_blocks = 0;
  setup_tsf_vol.free = 0;
  setup_tsf_vol.chunks_number = 0;
  setup_tsf_vol.files_number = 0;

  if (setup_flash_detected == 0)
  {
    setup_tsf_error = TSF_RES_FS_ERROR;
    return;
  }

  setup_tsf_config_init();
  sfi_enable();

  rc = tsf_mount(&setup_tsf_cfg, &setup_tsf_vol);
  if (rc != TSF_RES_OK)
  {
    setup_tsf_error = (u8)rc;
    return;
  }

  setup_tsf_valid_blocks = setup_tsf_vol.chunks_number;
  setup_tsf_mounted = 1;
  setup_tsf_rebuild_block_map();
}


void setup_copy_name(char *dst, const char *src, u8 max_len)
{
  u8 i;

  for (i = 0; i < max_len; i++)
  {
    dst[i] = src[i];
    if (src[i] == 0) return;
  }

  dst[max_len] = 0;
}

void setup_host_dir_clear_buf()
{
  u8 i;

  for (i = 0; i < SETUP_HOST_DIR_ENTRY_SIZE; i++)
    setup_copy_buf[i] = 0;
}

u32 setup_host_dir_get_u32(u8 ofs)
{
  return (u32)setup_copy_buf[ofs] |
         ((u32)setup_copy_buf[ofs + 1] << 8) |
         ((u32)setup_copy_buf[ofs + 2] << 16) |
         ((u32)setup_copy_buf[ofs + 3] << 24);
}

void setup_host_dir_put_u32(u8 ofs, u32 value)
{
  setup_copy_buf[ofs] = (u8)value;
  setup_copy_buf[ofs + 1] = (u8)(value >> 8);
  setup_copy_buf[ofs + 2] = (u8)(value >> 16);
  setup_copy_buf[ofs + 3] = (u8)(value >> 24);
}

void setup_host_dir_store_entry(u32 base, u8 index, const char *name, u32 size, u8 is_dir, u32 aux, u8 aux_valid)
{
  u8 i = 0;

  if (index >= SETUP_HOST_DIR_ENTRY_COUNT) return;

  setup_host_dir_clear_buf();
  setup_copy_buf[0] = SETUP_HOST_DIR_FLAG_VALID;
  if (is_dir) setup_copy_buf[0] |= SETUP_HOST_DIR_FLAG_DIR;
  if (aux_valid) setup_copy_buf[0] |= SETUP_HOST_DIR_FLAG_AUX;
  setup_host_dir_put_u32(SETUP_HOST_DIR_SIZE_OFS, size);
  setup_host_dir_put_u32(SETUP_HOST_DIR_AUX_OFS, aux);

  if (name)
  {
    while ((i < SETUP_NAME_MAX) && (name[i] != 0))
    {
      setup_copy_buf[SETUP_HOST_DIR_NAME_OFS + i] = (u8)name[i];
      i++;
    }
  }

  setup_copy_buf[SETUP_HOST_DIR_NAME_OFS + i] = 0;
  setup_spi_dram_write_block((base + ((u32)index * SETUP_HOST_DIR_ENTRY_SIZE)), setup_copy_buf, SETUP_HOST_DIR_ENTRY_SIZE);
}

void setup_host_dir_invalidate_entry(u32 base, u8 index)
{
  if (index >= SETUP_HOST_DIR_ENTRY_COUNT) return;

  setup_host_dir_clear_buf();
  setup_spi_dram_write_block((base + ((u32)index * SETUP_HOST_DIR_ENTRY_SIZE)), setup_copy_buf, SETUP_HOST_DIR_ENTRY_SIZE);
}

void setup_host_dir_mark_end(u32 base, u8 count)
{
  if (count < SETUP_HOST_DIR_ENTRY_COUNT) setup_host_dir_invalidate_entry(base, count);
}

u8 setup_host_dir_load_entry(u32 base, u8 index, char *name, u32 *size, u8 *is_dir, u32 *aux, u8 *aux_valid)
{
  u8 i;
  u8 flags;

  if (name) name[0] = 0;
  if (size) *size = 0;
  if (is_dir) *is_dir = 0;
  if (aux) *aux = 0;
  if (aux_valid) *aux_valid = 0;
  if (index >= SETUP_HOST_DIR_ENTRY_COUNT) return 0;

  setup_spi_dram_read_block((base + ((u32)index * SETUP_HOST_DIR_ENTRY_SIZE)), setup_copy_buf, SETUP_HOST_DIR_ENTRY_SIZE);
  flags = setup_copy_buf[0];
  if ((flags & SETUP_HOST_DIR_FLAG_VALID) == 0) return 0;

  if (size) *size = setup_host_dir_get_u32(SETUP_HOST_DIR_SIZE_OFS);
  if (is_dir) *is_dir = (flags & SETUP_HOST_DIR_FLAG_DIR) ? 1 : 0;
  if (aux) *aux = setup_host_dir_get_u32(SETUP_HOST_DIR_AUX_OFS);
  if (aux_valid) *aux_valid = (flags & SETUP_HOST_DIR_FLAG_AUX) ? 1 : 0;

  if (name)
  {
    for (i = 0; i < SETUP_NAME_MAX; i++)
    {
      name[i] = (char)setup_copy_buf[SETUP_HOST_DIR_NAME_OFS + i];
      if (name[i] == 0) return 1;
    }

    name[SETUP_NAME_MAX] = 0;
  }

  return 1;
}

u8 setup_tsf_rebuild_dir_cache()
{
  u32 off;
  u16 block = 0;
  u8 count = 0;

  setup_tsf_dir_cache_valid = 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;

  if (setup_tsf_map_valid)
  {
    while ((block < setup_tsf_total_blocks) && (count < SETUP_HOST_DIR_ENTRY_COUNT))
    {
      u8 entries = 8;
      u8 i;

      if ((u16)(block + entries) > setup_tsf_total_blocks) entries = (u8)(setup_tsf_total_blocks - block);
      setup_spi_dram_read_block((SETUP_HOST_TSF_MAP_ADDR + ((u32)block * SETUP_HOST_TSF_MAP_ENTRY_SIZE)), setup_tsf_buf, (u16)entries * SETUP_HOST_TSF_MAP_ENTRY_SIZE);

      for (i = 0; i < entries; i++)
      {
        u8 *entry = setup_tsf_buf + ((u16)i * SETUP_HOST_TSF_MAP_ENTRY_SIZE);
        u32 magic;
        u32 addr;
        TSF_HDR hdr;
        u8 len;

        if ((entry[7] & SETUP_HOST_TSF_MAP_FLAG_VALID) == 0) continue;
        if (entry[6] != (u8)TSF_CHUNK_HEAD) continue;

        magic = (u32)entry[0] |
                ((u32)entry[1] << 8) |
                ((u32)entry[2] << 16) |
                ((u32)entry[3] << 24);
        if (magic != TSF_MAGIC) continue;

        addr = setup_tsf_cfg.bulk_start + ((u32)(block + i) * setup_tsf_cfg.block_size);
        setup_tsf_hal_read(addr + sizeof(TSF_CHUNK), &hdr, sizeof(hdr));
        len = hdr.fnlen;
        if (len > SETUP_NAME_MAX) len = SETUP_NAME_MAX;
        setup_tsf_hal_read(addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), setup_tsf_name_buf, len);
        setup_tsf_name_buf[len] = 0;
        setup_host_dir_store_entry(SETUP_HOST_TSF_DIR_ADDR, count, setup_tsf_name_buf, hdr.size, 0, addr, 1);
        count++;
        if (count >= SETUP_HOST_DIR_ENTRY_COUNT) break;
      }

      block += entries;
    }
  }
  else
  {
    for (off = 0; off < setup_tsf_cfg.bulk_size; off += setup_tsf_cfg.block_size)
    {
      u32 addr = setup_tsf_cfg.bulk_start + off;
      TSF_CHUNK chunk;

      if (count >= SETUP_HOST_DIR_ENTRY_COUNT) break;

      setup_tsf_hal_read(addr, &chunk, sizeof(chunk));
      if ((chunk.magic != TSF_MAGIC) || (chunk.type != (u8)TSF_CHUNK_HEAD)) continue;

      {
        TSF_HDR hdr;
        u8 len;

        setup_tsf_hal_read(addr + sizeof(TSF_CHUNK), &hdr, sizeof(hdr));
        len = hdr.fnlen;
        if (len > SETUP_NAME_MAX) len = SETUP_NAME_MAX;
        setup_tsf_hal_read(addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), setup_tsf_name_buf, len);
        setup_tsf_name_buf[len] = 0;
        setup_host_dir_store_entry(SETUP_HOST_TSF_DIR_ADDR, count, setup_tsf_name_buf, hdr.size, 0, addr, 1);
        count++;
      }
    }
  }

  setup_host_dir_mark_end(SETUP_HOST_TSF_DIR_ADDR, count);
  setup_tsf_dir_cache_valid = 1;
  return count;
}

u8 setup_tsf_get_entry(u8 index, char *name, u32 *size)
{
  if (name) name[0] = 0;
  if (size) *size = 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;

  if (setup_tsf_dir_cache_valid == 0) return 0;
  return setup_host_dir_load_entry(SETUP_HOST_TSF_DIR_ADDR, index, name, size, 0, 0, 0);
}

u8 setup_tsf_get_entry_head(u8 index, char *name, u32 *size, u32 *head_addr, u8 *head_valid)
{
  if (name) name[0] = 0;
  if (size) *size = 0;
  if (head_addr) *head_addr = 0;
  if (head_valid) *head_valid = 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;

  if (setup_tsf_dir_cache_valid == 0) return 0;
  return setup_host_dir_load_entry(SETUP_HOST_TSF_DIR_ADDR, index, name, size, 0, head_addr, head_valid);
}

u8 setup_name_equal(const char *a, const char *b)
{
  u8 i;

  for (i = 0; i <= SETUP_NAME_MAX; i++)
  {
    if (a[i] != b[i]) return 0;
    if (a[i] == 0) return 1;
  }

  return 1;
}

u8 setup_tsf_cached_head_by_name(const char *name, u32 *size, u32 *head_addr, u8 *head_valid)
{
  u8 i;
  u32 entry_size;
  u32 entry_head;
  u8 entry_head_valid;
  char *entry_name = (char*)(void*)setup_tsf_buf;

  if (size) *size = 0;
  if (head_addr) *head_addr = 0;
  if (head_valid) *head_valid = 0;
  if (name == 0) return 0;
  if (setup_tsf_dir_cache_valid == 0) return 0;

  setup_copy_name(setup_delete_name_buf, name, SETUP_NAME_MAX);

  for (i = 0; i < setup_panel_count(1); i++)
  {
    if (setup_host_dir_load_entry(SETUP_HOST_TSF_DIR_ADDR, i, entry_name, &entry_size, 0, &entry_head, &entry_head_valid) == 0)
      return 0;

    if (setup_name_equal(setup_delete_name_buf, entry_name))
    {
      if (size) *size = entry_size;
      if (head_addr) *head_addr = entry_head;
      if (head_valid) *head_valid = entry_head_valid;
      return 1;
    }
  }

  return 0;
}

u8 setup_tsf_name_len(const char *name)
{
  u8 len = 0;

  while ((len < SETUP_NAME_MAX) && (name[len] != 0))
    len++;

  return len;
}

u8 setup_tsf_open_read_at(TSF_VOLUME *vol, TSF_FILE *file, const char *name, u32 head_addr)
{
  TSF_CHUNK chunk;
  TSF_HDR hdr;
  u8 name_len;

  if (vol == 0) return 0;
  if (file == 0) return 0;
  if (name == 0) return 0;
  if (vol->cfg == 0) return 0;
  if (vol->cfg->hal_read_func == 0) return 0;

  if (vol->cfg->hal_read_func(head_addr, &chunk, sizeof(chunk)) != TSF_RES_OK) return 0;
  if ((chunk.magic != TSF_MAGIC) || (chunk.type != (u8)TSF_CHUNK_HEAD)) return 0;
  if (vol->cfg->hal_read_func(head_addr + sizeof(TSF_CHUNK), &hdr, sizeof(hdr)) != TSF_RES_OK) return 0;

  name_len = setup_tsf_name_len(name);
  if (name_len != hdr.fnlen) return 0;

  if (name_len)
  {
    if (vol->cfg->hal_read_func(head_addr + sizeof(TSF_CHUNK) + sizeof(TSF_HDR), setup_tsf_buf, name_len) != TSF_RES_OK) return 0;
    setup_tsf_buf[name_len] = 0;
    if (setup_name_equal(name, (char*)(void*)setup_tsf_buf) == 0) return 0;
  }

  file->mode = TSF_MODE_READ;
  file->vol = vol;
  file->addr = head_addr;
  file->size = hdr.size;
  file->seek = 0;
  file->chunk_addr = head_addr;
  file->chunk_offset = sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + name_len;
  file->prev_chunk_addr = 0;
  file->next_chunk = chunk.next_chunk;
  return 1;
}

TSF_RESULT setup_tsf_open_read_cached(TSF_VOLUME *vol, TSF_FILE *file, const char *name, u32 head_addr, u8 head_valid)
{
  if (head_valid)
  {
    if (setup_tsf_open_read_at(vol, file, name, head_addr)) return TSF_RES_OK;
  }

  return tsf_open(vol, file, name, TSF_MODE_READ);
}

TSF_RESULT setup_tsf_open_read_cached_by_name(TSF_VOLUME *vol, TSF_FILE *file, const char *name)
{
  u32 size;
  u32 head_addr;
  u8 head_valid;

  if (setup_tsf_cached_head_by_name(name, &size, &head_addr, &head_valid))
    return setup_tsf_open_read_cached(vol, file, name, head_addr, head_valid);

  return tsf_open(vol, file, name, TSF_MODE_READ);
}

void setup_sd_update_free()
{
  FATFS *fs;
  DWORD free_clusters;
  FRESULT rc;
  u32 bytes_per_cluster;

  setup_sd_free = 0;
  if (setup_sd_mounted == 0) return;

  rc = f_getfree("", &free_clusters, &fs);
  if (rc != FR_OK)
  {
    setup_sd_error = rc;
    return;
  }

  if (fs == 0) return;
  bytes_per_cluster = (u32)fs->csize * 512UL;
  if (bytes_per_cluster == 0) return;
  if (free_clusters > (0xFFFFFFFFUL / bytes_per_cluster)) setup_sd_free = 0xFFFFFFFFUL;
  else setup_sd_free = (u32)free_clusters * bytes_per_cluster;
}

u8 setup_sd_count_entries()
{
  u8 count = 0;
  FRESULT rc;

  setup_sd_dir_cache_valid = 0;
  if (setup_sd_mounted == 0) return 0;

  if (setup_sd_path[0] != 0)
  {
    setup_host_dir_store_entry(SETUP_HOST_SD_DIR_ADDR, count, "..", 0, 1, 0, 0);
    count++;
  }

  rc = f_opendir(&setup_sd_dir, setup_sd_path);
  if (rc != FR_OK)
  {
    setup_sd_error = rc;
    setup_host_dir_mark_end(SETUP_HOST_SD_DIR_ADDR, count);
    if (count) setup_sd_dir_cache_valid = 1;
    return count;
  }

  while (count < SETUP_HOST_DIR_ENTRY_COUNT)
  {
    rc = f_readdir(&setup_sd_dir, &setup_sd_fno);
    if (rc != FR_OK)
    {
      setup_sd_error = rc;
      break;
    }

    if (setup_sd_fno.fname[0] == 0) break;

    setup_host_dir_store_entry(SETUP_HOST_SD_DIR_ADDR, count, setup_sd_fno.fname,
                               (u32)setup_sd_fno.fsize,
                               (setup_sd_fno.fattrib & AM_DIR) ? 1 : 0, 0, 0);
    count++;
  }

  f_closedir(&setup_sd_dir);
  setup_host_dir_mark_end(SETUP_HOST_SD_DIR_ADDR, count);
  setup_sd_dir_cache_valid = 1;
  return count;
}

void setup_sd_mount()
{
  FRESULT rc;

  setup_sd_mounted = 0;
  setup_sd_error = FR_OK;
  setup_sd_count = 0;
  setup_sd_free = 0;
  setup_sd_dir_cache_valid = 0;
  setup_sd_path[0] = 0;

  rc = f_mount(&setup_sd_fs, "", 1);
  if (rc != FR_OK)
  {
    setup_sd_error = rc;
    return;
  }

  setup_sd_mounted = 1;
  setup_sd_update_free();
  setup_sd_count = setup_sd_count_entries();
}


u8 setup_sd_get_entry(u8 index, char *name, u32 *size, u8 *is_dir)
{
  if (name) name[0] = 0;
  if (size) *size = 0;
  if (is_dir) *is_dir = 0;
  if (setup_sd_mounted == 0) return 0;

  if (setup_sd_dir_cache_valid == 0) return 0;
  return setup_host_dir_load_entry(SETUP_HOST_SD_DIR_ADDR, index, name, size, is_dir, 0, 0);
}

u8 setup_sd_path_len()
{
  u8 len = 0;

  while ((len < SETUP_PATH_MAX) && (setup_sd_path[len] != 0))
    len++;

  return len;
}

void setup_sd_path_parent()
{
  u8 len;

  len = setup_sd_path_len();
  if (len == 0) return;

  while (len > 0)
  {
    len--;
    if (setup_sd_path[len] == '/')
    {
      setup_sd_path[len] = 0;
      return;
    }
  }

  setup_sd_path[0] = 0;
}

u8 setup_sd_path_append(const char *name)
{
  u8 len;
  u8 name_len = 0;
  u8 add_slash;
  u8 i = 0;

  len = setup_sd_path_len();
  add_slash = len ? 1 : 0;

  while (name[name_len] != 0)
    name_len++;

  if ((u16)len + add_slash + name_len > SETUP_PATH_MAX) return 0;

  if (add_slash)
  {
    setup_sd_path[len] = '/';
    len++;
  }

  while (name[i] != 0)
  {
    setup_sd_path[len] = name[i];
    len++;
    i++;
  }

  setup_sd_path[len] = 0;
  return 1;
}

void setup_sd_reload_panel()
{
  setup_sd_update_free();
  setup_sd_count = setup_sd_count_entries();
  setup_panels[0].cursor = 0;
  setup_panels[0].scroll = 0;
  setup_panels[0].count = setup_panel_count(0);
  setup_panel_fix_cursor(&setup_panels[0]);
}


void setup_sd_refresh_panel()
{
  setup_sd_update_free();
  setup_sd_count = setup_sd_count_entries();
  setup_panels[0].count = setup_panel_count(0);
  setup_panel_fix_cursor(&setup_panels[0]);
}

void setup_sd_path_last_name(char *name)
{
  u8 len;
  u8 start;
  u8 i = 0;

  name[0] = 0;
  len = setup_sd_path_len();
  if (len == 0) return;

  start = len;
  while (start > 0)
  {
    if (setup_sd_path[start - 1] == '/') break;
    start--;
  }

  while ((start < len) && (i < SETUP_NAME_MAX))
  {
    name[i] = setup_sd_path[start];
    i++;
    start++;
  }

  name[i] = 0;
}

void setup_panel_select_index(SetupPanel *p, u8 index)
{
  if (p->count == 0)
  {
    p->cursor = 0;
    p->scroll = 0;
    return;
  }

  if (index >= p->count) index = p->count - 1;

  if (index < p->scroll)
  {
    p->scroll = index;
    p->cursor = 0;
    return;
  }

  if (index >= (p->scroll + SETUP_PANEL_VISIBLE_ROWS))
  {
    p->scroll = index - SETUP_PANEL_VISIBLE_ROWS + 1;
    p->cursor = SETUP_PANEL_VISIBLE_ROWS - 1;
    return;
  }

  p->cursor = index - p->scroll;
}

u8 setup_sd_select_name(const char *name)
{
  u8 index = 0;

  if (name[0] == 0) return 0;
  if (setup_sd_mounted == 0) return 0;

  if (setup_sd_path[0] != 0) index = 1;

  while (index < setup_panels[0].count)
  {
    u8 i = 0;
    u8 equal = 1;

    if (setup_sd_get_entry(index, setup_sd_name_buf, 0, 0) == 0) return 0;

    while (i <= SETUP_NAME_MAX)
    {
      if (setup_sd_name_buf[i] != name[i])
      {
        equal = 0;
        break;
      }
      if (setup_sd_name_buf[i] == 0) break;
      i++;
    }

    if (equal)
    {
      setup_panel_select_index(&setup_panels[0], index);
      return 1;
    }

    index++;
  }

  return 0;
}

void setup_tsf_refresh_panel_light()
{
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0))
  {
    setup_tsf_dir_cache_valid = 0;
    setup_panels[1].count = 0;
    setup_panel_fix_cursor(&setup_panels[1]);
    return;
  }

  setup_tsf_rebuild_dir_cache();
  setup_panels[1].count = setup_panel_count(1);
  setup_panel_fix_cursor(&setup_panels[1]);
}


u8 setup_name_extension_type(const char *name)
{
  u8 dot = 0xFF;
  u8 len = 0;
  u8 e0;
  u8 e1;
  u8 e2;

  while (name[len])
  {
    if (name[len] == '.') dot = len;
    len++;
  }

  if (dot == 0xFF) return SETUP_FPGA_TYPE_NONE;
  if ((len - dot) != 4) return SETUP_FPGA_TYPE_NONE;

  e0 = (u8)name[dot + 1];
  e1 = (u8)name[dot + 2];
  e2 = (u8)name[dot + 3];
  if ((e0 >= 'a') && (e0 <= 'z')) e0 -= 32;
  if ((e1 >= 'a') && (e1 <= 'z')) e1 -= 32;
  if ((e2 >= 'a') && (e2 <= 'z')) e2 -= 32;

  if ((e0 == 'R') && (e1 == 'B') && (e2 == 'F')) return SETUP_FPGA_TYPE_RBF;
  if ((e0 == 'M') && (e1 == 'L') && (e2 == 'Z')) return SETUP_FPGA_TYPE_MLZ;

  return SETUP_FPGA_TYPE_NONE;
}

u8 setup_fpga_set_current_config(const char *name, u8 type)
{
  if ((type != SETUP_FPGA_TYPE_RBF) && (type != SETUP_FPGA_TYPE_MLZ))
  {
    setup_fpga_error = 1;
    return 0;
  }

  if (cfg_set_boot_bitstream(name) == 0)
  {
    setup_fpga_error = 1;
    return 0;
  }

  setup_fpga_error = 0;
  return 1;
}

u8 setup_fpga_config_start()
{
  u32 guard = 0x0000FFFFUL;

  cli();
  spi_set_lsb();

  DDRF |= (1 << nCONFIG);
  _delay_ms(50);
  DDRF &= ~(1 << nCONFIG);

  while (!(PINF & (1 << nSTATUS)))
  {
    if (guard == 0) return 0;
    guard--;
  }

  return 1;
}

void setup_fpga_release_runtime_pins()
{
  sfi_cs_off();
  sfi_disable();

  DDRF &= (u8)~(_BV(nCONFIG) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_ASDO) | _BV(SFI_BIT_DCLK) | _BV(SFI_BIT_DATA0));
  PORTF |= _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_DATA0) | _BV(SFI_BIT_DCLK);
  spi_set_lsb();
}

void setup_fpga_prepare_tsf()
{
  setup_fpga_tsf_cfg = setup_tsf_cfg;
  setup_fpga_tsf_vol = setup_tsf_vol;
  setup_fpga_tsf_vol.cfg = &setup_fpga_tsf_cfg;
}

void setup_fpga_halt_after_load()
{
  if (setup_fpga_halt_enabled == 0) return;

  setup_fpga_release_runtime_pins();

  while (1)
  {
  }
}

void setup_fpga_save_run_now_name(const char *name)
{
  setup_copy_name(setup_fpga_run_now_name, name, SETUP_NAME_MAX);
  setup_fpga_run_now_pending = 1;
}

void setup_fpga_request_run_now(const char *name)
{
  setup_fpga_save_run_now_name(name);
  flags_register |= FLAG_HARD_RESET;
}

u8 setup_fpga_load_rbf_now(const char *name, u32 size)
{
  TSF_RESULT tr;
  u16 chunk;
  u16 i;

  setup_fpga_prepare_tsf();
  tr = setup_tsf_open_read_cached_by_name(&setup_fpga_tsf_vol, &setup_fpga_tsf_file, name);
  if (tr != TSF_RES_OK)
  {
    setup_tsf_error = (u8)tr;
    setup_fpga_error = 2;
    return 0;
  }

  if (setup_fpga_config_start() == 0)
  {
    tsf_close(&setup_fpga_tsf_file);
    setup_fpga_error = 3;
    return 0;
  }

  while (size > 0)
  {
    chunk = (size > SETUP_COPY_BUF_SIZE) ? SETUP_COPY_BUF_SIZE : (u16)size;
    tr = tsf_read(&setup_fpga_tsf_file, setup_copy_buf, chunk);
    if (tr != TSF_RES_OK)
    {
      tsf_close(&setup_fpga_tsf_file);
      setup_tsf_error = (u8)tr;
      setup_fpga_error = 2;
      return 0;
    }

    for (i = 0; i < chunk; i++)
      spi_send(setup_copy_buf[i]);

    size -= chunk;
  }

  tsf_close(&setup_fpga_tsf_file);
  setup_fpga_halt_after_load();
  return 1;
}

u16 setup_mlz_dbpos;
u8 setup_mlz_bitstream;
u8 setup_mlz_bitcount;
u8 setup_mlz_error;

u8 setup_mlz_next_byte()
{
  u8 b = 0;
  TSF_RESULT tr;

  if (setup_mlz_error) return 0;

  tr = tsf_read(&setup_fpga_tsf_file, &b, 1);
  if (tr != TSF_RES_OK)
  {
    setup_tsf_error = (u8)tr;
    setup_mlz_error = 1;
    setup_fpga_error = 2;
    return 0;
  }

  return b;
}

void setup_mlz_put_byte(u8 byte)
{
  dbuf[setup_mlz_dbpos] = byte;
  setup_mlz_dbpos = DBMASK & (setup_mlz_dbpos + 1);

  if (setup_mlz_dbpos == 0)
    put_buffer(DBSIZE);
}

u8 setup_mlz_get_bits(u8 numbits)
{
  u8 bits = 0;

  do
  {
    if (!(setup_mlz_bitcount--))
    {
      setup_mlz_bitcount = 7;
      setup_mlz_bitstream = setup_mlz_next_byte();
    }

    bits = (bits << 1) | (setup_mlz_bitstream >> 7);
    setup_mlz_bitstream <<= 1;
  } while (--numbits);

  return bits;
}

s16 setup_mlz_get_bigdisp()
{
  u8 bits;

  if (setup_mlz_get_bits(1))
  {
    bits = setup_mlz_get_bits(4);
    return (((0xF0 | bits) - 1) << 8) | setup_mlz_next_byte();
  }

  return 0xFF00 | setup_mlz_next_byte();
}

void setup_mlz_repeat(s16 disp, u8 len)
{
  u8 i;

  for (i = 0; i < len; i++)
    setup_mlz_put_byte(dbuf[DBMASK & (setup_mlz_dbpos + disp)]);
}

u8 setup_fpga_load_mlz_now(const char *name)
{
  u8 j;
  u8 bits;
  s16 disp;
  TSF_RESULT tr;

  setup_fpga_prepare_tsf();
  tr = setup_tsf_open_read_cached_by_name(&setup_fpga_tsf_vol, &setup_fpga_tsf_file, name);
  if (tr != TSF_RES_OK)
  {
    setup_tsf_error = (u8)tr;
    setup_fpga_error = 2;
    return 0;
  }

  if (setup_fpga_config_start() == 0)
  {
    tsf_close(&setup_fpga_tsf_file);
    setup_fpga_error = 3;
    return 0;
  }

  setup_mlz_error = 0;
  setup_mlz_dbpos = 0;
  setup_mlz_put_byte(setup_mlz_next_byte());
  setup_mlz_bitstream = setup_mlz_next_byte();
  setup_mlz_bitcount = 8;

  do
  {
    j = 0;

    if (setup_mlz_get_bits(1))
      setup_mlz_put_byte(setup_mlz_next_byte());
    else
    {
      switch (setup_mlz_get_bits(2))
      {
        case 0:
          setup_mlz_repeat(0xFFF8 | setup_mlz_get_bits(3), 1);
        break;

        case 1:
          setup_mlz_repeat(0xFF00 | setup_mlz_next_byte(), 2);
        break;

        case 2:
          setup_mlz_repeat(setup_mlz_get_bigdisp(), 3);
        break;

        case 3:
          do j++; while (!setup_mlz_get_bits(1));
          if (j < 8)
          {
            bits = setup_mlz_get_bits(j);
            disp = setup_mlz_get_bigdisp();
            setup_mlz_repeat(disp, 2 + (1 << j) + bits);
          }
        break;
      }
    }
  } while ((j < 8) && (setup_mlz_error == 0));

  if ((DBMASK & setup_mlz_dbpos) && (setup_mlz_error == 0))
    put_buffer(DBMASK & setup_mlz_dbpos);

  tsf_close(&setup_fpga_tsf_file);

  if (setup_mlz_error) return 0;

  setup_fpga_halt_after_load();
  return 1;
}

u8 setup_fpga_load_now(const char *name, u8 type, u32 size)
{
  if (type == SETUP_FPGA_TYPE_RBF) return setup_fpga_load_rbf_now(name, size);
  if (type == SETUP_FPGA_TYPE_MLZ) return setup_fpga_load_mlz_now(name);
  return 0;
}

u8 setup_try_boot_config()
{
  char *name = setup_tsf_name_buf;
  u8 type;
  TSF_FILE_STAT stat;
  u8 ok = 0;

  setup_workspace_init();
  setup_fpga_halt_enabled = 0;

  if (setup_fpga_run_now_pending)
  {
    setup_fpga_run_now_pending = 0;
    setup_copy_name(name, setup_fpga_run_now_name, SETUP_NAME_MAX);
  }
  else
  {
    if (cfg_get_boot_bitstream(name, SETUP_NAME_MAX + 1) == 0) goto exit;
  }

  if (name[0] == 0) goto exit;

  setup_flash_detect();
  if (setup_flash_detected == 0) goto exit;

  setup_tsf_mount_volume();
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) goto exit;

  type = setup_name_extension_type(name);
  if (type == SETUP_FPGA_TYPE_NONE) goto exit;

  if (tsf_stat(&setup_tsf_vol, &stat, name) != TSF_RES_OK) goto exit;

  ok = setup_fpga_load_now(name, type, stat.size);
  if (ok) setup_fpga_release_runtime_pins();

exit:
  setup_fpga_halt_enabled = 1;
  return ok;
}

void setup_tsf_enter_active()
{
  u32 size;

  if (setup_active_panel != 1) return;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return;
  if (setup_panels[1].count == 0) return;
  if (setup_tsf_get_entry(setup_active_entry(1), setup_tsf_name_buf, &size) == 0) return;

  setup_fpga_type = setup_name_extension_type(setup_tsf_name_buf);
  if (setup_fpga_type == SETUP_FPGA_TYPE_NONE) return;

  setup_fpga_size = size;
  setup_help_visible = 0;
  setup_eeprom_visible = 0;
  setup_format_visible = 0;
  setup_chkdsk_visible = 0;
  setup_mkdir_visible = 0;
  setup_fpga_visible = 1;
  setup_fpga_option = SETUP_FPGA_SET_CURRENT;
  setup_fpga_error = 0;
  setup_draw_fpga_window();
}

void setup_fpga_action()
{
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return;
  if (setup_fpga_type == SETUP_FPGA_TYPE_NONE) return;

  if (setup_fpga_option == SETUP_FPGA_LOAD_NOW)
  {
    setup_fpga_request_run_now(setup_tsf_name_buf);
    return;
  }

  if (setup_fpga_set_current_config(setup_tsf_name_buf, setup_fpga_type))
  {
    setup_fpga_visible = 0;
    setup_dialog_backup_restore_or_redraw(SETUP_FPGA_X, SETUP_FPGA_Y, SETUP_FPGA_W, SETUP_FPGA_H);
    return;
  }

  setup_draw_fpga_window();
}

u8 setup_sd_build_path(const char *name)
{
  u8 len;
  u8 name_len = 0;
  u8 add_slash;
  u8 i = 0;

  setup_op_path[0] = 0;
  len = setup_sd_path_len();
  add_slash = len ? 1 : 0;

  while (name[name_len] != 0)
    name_len++;

  if ((u16)len + add_slash + name_len > SETUP_PATH_MAX) return 0;

  for (i = 0; i < len; i++)
    setup_op_path[i] = setup_sd_path[i];

  if (add_slash)
  {
    setup_op_path[len] = '/';
    len++;
  }

  i = 0;
  while (name[i] != 0)
  {
    setup_op_path[len] = name[i];
    len++;
    i++;
  }

  setup_op_path[len] = 0;
  return 1;
}


void setup_mkdir_show_dialog()
{
  if (setup_active_panel != 0) return;
  if (setup_sd_mounted == 0) return;

  setup_help_visible = 0;
  setup_mkdir_visible = 1;
  setup_mkdir_error = SETUP_MKDIR_ERROR_NONE;
  setup_delete_name_buf[0] = 0;
  setup_draw_mkdir_window();
}

u8 setup_scancode_to_char(u8 code)
{
  if (code == 0x1C) return setup_key_shift ? 'A' : 'a';
  if (code == 0x32) return setup_key_shift ? 'B' : 'b';
  if (code == 0x21) return setup_key_shift ? 'C' : 'c';
  if (code == 0x23) return setup_key_shift ? 'D' : 'd';
  if (code == 0x24) return setup_key_shift ? 'E' : 'e';
  if (code == 0x2B) return setup_key_shift ? 'F' : 'f';
  if (code == 0x34) return setup_key_shift ? 'G' : 'g';
  if (code == 0x33) return setup_key_shift ? 'H' : 'h';
  if (code == 0x43) return setup_key_shift ? 'I' : 'i';
  if (code == 0x3B) return setup_key_shift ? 'J' : 'j';
  if (code == 0x42) return setup_key_shift ? 'K' : 'k';
  if (code == 0x4B) return setup_key_shift ? 'L' : 'l';
  if (code == 0x3A) return setup_key_shift ? 'M' : 'm';
  if (code == 0x31) return setup_key_shift ? 'N' : 'n';
  if (code == 0x44) return setup_key_shift ? 'O' : 'o';
  if (code == 0x4D) return setup_key_shift ? 'P' : 'p';
  if (code == 0x15) return setup_key_shift ? 'Q' : 'q';
  if (code == 0x2D) return setup_key_shift ? 'R' : 'r';
  if (code == 0x1B) return setup_key_shift ? 'S' : 's';
  if (code == 0x2C) return setup_key_shift ? 'T' : 't';
  if (code == 0x3C) return setup_key_shift ? 'U' : 'u';
  if (code == 0x2A) return setup_key_shift ? 'V' : 'v';
  if (code == 0x1D) return setup_key_shift ? 'W' : 'w';
  if (code == 0x22) return setup_key_shift ? 'X' : 'x';
  if (code == 0x35) return setup_key_shift ? 'Y' : 'y';
  if (code == 0x1A) return setup_key_shift ? 'Z' : 'z';

  if (code == 0x45) return setup_key_shift ? ')' : '0';
  if (code == 0x16) return setup_key_shift ? '!' : '1';
  if (code == 0x1E) return setup_key_shift ? '@' : '2';
  if (code == 0x26) return setup_key_shift ? '#' : '3';
  if (code == 0x25) return setup_key_shift ? '$' : '4';
  if (code == 0x2E) return setup_key_shift ? '%' : '5';
  if (code == 0x36) return setup_key_shift ? '^' : '6';
  if (code == 0x3D) return setup_key_shift ? '&' : '7';
  if (code == 0x3E) return setup_key_shift ? '*' : '8';
  if (code == 0x46) return setup_key_shift ? '(' : '9';

  if (code == 0x29) return ' ';
  if (code == 0x4E) return setup_key_shift ? '_' : '-';
  if (code == 0x55) return setup_key_shift ? '+' : '=';
  if (code == 0x49) return setup_key_shift ? '>' : '.';
  if (code == 0x41) return setup_key_shift ? '<' : ',';

  return 0;
}

void setup_mkdir_add_char(u8 ch)
{
  u8 len;

  if (ch == 0) return;
  if (ch == '"') return;
  if (ch == '*') return;
  if (ch == ':') return;
  if (ch == '/') return;
  if (ch == '<') return;
  if (ch == '>') return;
  if (ch == '?') return;
  if (ch == 0x5C) return;
  if (ch == '|') return;

  len = 0;
  while ((len < SETUP_NAME_MAX) && (setup_delete_name_buf[len] != 0))
    len++;
  if (len >= SETUP_NAME_MAX) return;

  setup_delete_name_buf[len] = ch;
  setup_delete_name_buf[len + 1] = 0;
  setup_mkdir_error = SETUP_MKDIR_ERROR_NONE;
  setup_draw_mkdir_window();
}

void setup_mkdir_backspace()
{
  u8 len;

  len = 0;
  while ((len < SETUP_NAME_MAX) && (setup_delete_name_buf[len] != 0))
    len++;
  if (len == 0) return;

  setup_delete_name_buf[len - 1] = 0;
  setup_mkdir_error = SETUP_MKDIR_ERROR_NONE;
  setup_draw_mkdir_window();
}

void setup_mkdir_create()
{
  FRESULT fr;
  u8 len = 0;

  while ((len < SETUP_NAME_MAX) && (setup_delete_name_buf[len] != 0))
    len++;

  if (len == 0)
  {
    setup_mkdir_error = SETUP_MKDIR_ERROR_EMPTY;
    setup_draw_mkdir_window();
    return;
  }

  if (setup_sd_build_path(setup_delete_name_buf) == 0)
  {
    setup_mkdir_error = SETUP_MKDIR_ERROR_CREATE;
    setup_draw_mkdir_window();
    return;
  }

  fr = f_mkdir(setup_op_path);
  if (fr != FR_OK)
  {
    setup_sd_error = fr;
    if (fr == FR_EXIST) setup_mkdir_error = SETUP_MKDIR_ERROR_EXISTS;
    else setup_mkdir_error = SETUP_MKDIR_ERROR_CREATE;
    setup_draw_mkdir_window();
    return;
  }

  setup_mkdir_visible = 0;
  setup_sd_refresh_panel();
  setup_sd_select_name(setup_delete_name_buf);
  setup_draw_panels();
}

void setup_draw_mkdir_text(u8 rel_row)
{
  u8 x = SETUP_DELETE_X + 2;
  u8 len;

  if (rel_row == 1) setup_put_dialog_title(SETUP_DELETE_X, SETUP_DELETE_W, PSTR("Create folder"));
  else if (rel_row == 3) setup_put_text_p(x, PSTR("Folder name:"), SETUP_ATTR_DIALOG);
  else if (rel_row == 4)
  {
    setup_draw_delete_name(x, SETUP_DELETE_W - 5, SETUP_ATTR_DIALOG);
    len = 0;
    while ((len < SETUP_NAME_MAX) && (setup_delete_name_buf[len] != 0))
      len++;
    if (len < (SETUP_DELETE_W - 5)) setup_put_cell(x + len, '_', SETUP_ATTR_DIALOG_ACTIVE);
  }
  else if (rel_row == 5)
  {
    if (setup_mkdir_error == SETUP_MKDIR_ERROR_EMPTY) setup_put_text_p(x, PSTR("Name is empty"), SETUP_ATTR_INFO_ALERT);
    else if (setup_mkdir_error == SETUP_MKDIR_ERROR_EXISTS) setup_put_text_p(x, PSTR("Folder already exists"), SETUP_ATTR_INFO_ALERT);
    else if (setup_mkdir_error == SETUP_MKDIR_ERROR_CREATE) setup_put_text_p(x, PSTR("Create failed"), SETUP_ATTR_INFO_ALERT);
  }
  else if (rel_row == 7) setup_put_text_p(x, PSTR("Enter - create   Esc - cancel"), SETUP_ATTR_DIALOG);
}

void setup_draw_mkdir_window_row(u8 row)
{
  u8 rel;
  u8 x;
  u8 right;

  if (setup_mkdir_visible == 0) return;
  if (row < SETUP_DELETE_Y) return;
  if (row >= (SETUP_DELETE_Y + SETUP_DELETE_H)) return;

  rel = row - SETUP_DELETE_Y;
  right = SETUP_DELETE_X + SETUP_DELETE_W - 1;

  for (x = SETUP_DELETE_X; x <= right; x++)
    setup_put_cell(x, ' ', SETUP_ATTR_DIALOG);

  if (rel == 0)
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_TL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_DELETE_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_DIALOG_FRAME);
  }
  else if (rel == (SETUP_DELETE_H - 1))
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_BL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_DELETE_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_DIALOG_FRAME);
  }
  else
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_draw_mkdir_text(rel);
  }
}

void setup_draw_copy_confirm_text(u8 rel_row)
{
  u8 x = SETUP_DELETE_X + 2;

  if (rel_row == 1) setup_put_dialog_title(SETUP_DELETE_X, SETUP_DELETE_W, PSTR("Confirm copy"));
  else if (rel_row == 3) setup_put_text_p(x, PSTR("Copy selected file?"), SETUP_ATTR_DIALOG);
  else if (rel_row == 4) setup_draw_delete_name(x, SETUP_DELETE_W - 4, SETUP_ATTR_DIALOG_NAME);
  else if (rel_row == 6) setup_put_text_p(x, PSTR("Enter - copy   Esc - cancel"), SETUP_ATTR_DIALOG);
}

void setup_draw_copy_confirm_window_row(u8 row)
{
  u8 rel;
  u8 x;
  u8 right;

  if (setup_copy_confirm_visible == 0) return;
  if (row < SETUP_DELETE_Y) return;
  if (row >= (SETUP_DELETE_Y + SETUP_DELETE_H)) return;

  rel = row - SETUP_DELETE_Y;
  right = SETUP_DELETE_X + SETUP_DELETE_W - 1;

  for (x = SETUP_DELETE_X; x <= right; x++)
    setup_put_cell(x, ' ', SETUP_ATTR_DIALOG);

  if (rel == 0)
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_TL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_DELETE_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_DIALOG_FRAME);
  }
  else if (rel == (SETUP_DELETE_H - 1))
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_BL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_DELETE_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_DIALOG_FRAME);
  }
  else
  {
    setup_put_cell(SETUP_DELETE_X, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_draw_copy_confirm_text(rel);
  }
}


void setup_draw_copy_text(u8 rel_row)
{
  u8 x = SETUP_COPY_X + 2;

  if (rel_row == 1) setup_put_dialog_title(SETUP_COPY_X, SETUP_COPY_W, PSTR("Copy"));

  if (setup_copy_error != SETUP_COPY_ERROR_NONE)
  {
    if (rel_row == 3)
    {
      if (setup_copy_error == SETUP_COPY_ERROR_EXISTS)
        setup_put_text_p(x, PSTR("Destination file already exists"), SETUP_ATTR_INFO_ALERT);
      else if (setup_copy_error == SETUP_COPY_ERROR_NO_SPACE)
        setup_put_text_p(x, PSTR("Not enough free space"), SETUP_ATTR_INFO_ALERT);
    }
    else if (rel_row == 6) setup_put_text_p(x, PSTR("Esc - close"), SETUP_ATTR_DIALOG);
    return;
  }

  if (rel_row == 3) setup_put_text_p(x, PSTR("Copying file..."), SETUP_ATTR_DIALOG);
  else if (rel_row == 5) setup_put_text_p(x, PSTR("Progress"), SETUP_ATTR_DIALOG);

  if (rel_row == 7) setup_draw_progress_bar(SETUP_COPY_BAR_X, SETUP_COPY_BAR_W, setup_copy_progress, SETUP_ATTR_DIALOG);
}


void setup_draw_copy_window_row(u8 row)
{
  u8 rel;

  if (setup_copy_visible == 0) return;
  if (row < SETUP_COPY_Y) return;
  if (row >= (SETUP_COPY_Y + SETUP_COPY_H)) return;

  rel = row - SETUP_COPY_Y;
  setup_draw_dialog_frame_row(SETUP_COPY_X, SETUP_COPY_Y, SETUP_COPY_W, SETUP_COPY_H, row);
  if ((rel != 0) && (rel != (SETUP_COPY_H - 1))) setup_draw_copy_text(rel);
}


void setup_draw_copy_window()
{
  u8 row;

  if (setup_copy_visible == 0) return;

  for (row = SETUP_COPY_Y; row < (SETUP_COPY_Y + SETUP_COPY_H); row++)
  {
    setup_draw_copy_window_row(row);
    setup_write_row_range(row, SETUP_COPY_X, SETUP_COPY_W);
  }
}


void setup_copy_set_progress(u32 done, u32 total)
{
  u8 progress = setup_calc_progress(done, total, SETUP_COPY_BAR_W);

  if (progress == setup_copy_progress) return;
  setup_copy_progress = progress;
  if (setup_copy_visible)
    setup_redraw_progress_row(SETUP_COPY_Y + 7, SETUP_COPY_X, SETUP_COPY_W, SETUP_COPY_BAR_X, SETUP_COPY_BAR_W, setup_copy_progress, SETUP_ATTR_DIALOG);
}

void setup_copy_show_window()
{
  setup_copy_visible = 1;
  setup_copy_progress = 0;
  setup_copy_error = SETUP_COPY_ERROR_NONE;
  setup_draw_copy_window();
}

void setup_copy_show_error(u8 error)
{
  setup_copy_visible = 1;
  setup_copy_progress = 0;
  setup_copy_error = error;
  setup_draw_copy_window();
}

void setup_copy_hide_window()
{
  setup_copy_visible = 0;
  setup_copy_error = SETUP_COPY_ERROR_NONE;
}

u8 setup_copy_can_active_file()
{
  u8 is_dir = 0;

  setup_delete_name_buf[0] = 0;

  if (setup_panels[setup_active_panel].count == 0) return 0;

  if (setup_active_panel == 0)
  {
    if (setup_sd_mounted == 0) return 0;
    if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
    if (setup_sd_get_entry(setup_active_entry(0), setup_delete_name_buf, 0, &is_dir) == 0) return 0;
    if (is_dir) return 0;
    if ((setup_delete_name_buf[0] == '.') && (setup_delete_name_buf[1] == '.') && (setup_delete_name_buf[2] == 0)) return 0;
    return 1;
  }

  if (setup_sd_mounted == 0) return 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_tsf_get_entry(setup_active_entry(1), setup_delete_name_buf, 0) == 0) return 0;
  return 1;
}

void setup_copy_show_confirm()
{
  if (setup_copy_can_active_file() == 0) return;

  setup_copy_confirm_visible = 1;
  setup_draw_copy_confirm_window();
}

u32 setup_tsf_required_space(u32 size, const char *name)
{
  u8 name_len = 0;
  u32 head_payload;
  u32 body_payload;
  u32 blocks;

  while ((name[name_len] != 0) && (name_len < SETUP_NAME_MAX))
    name_len++;

  if ((sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + name_len) >= SETUP_TSF_BLOCK_SIZE)
    return 0xFFFFFFFFUL;

  head_payload = SETUP_TSF_BLOCK_SIZE - sizeof(TSF_CHUNK) - sizeof(TSF_HDR) - name_len;
  if (size <= head_payload) return SETUP_TSF_BLOCK_SIZE;

  size -= head_payload;
  body_payload = SETUP_TSF_BLOCK_SIZE - sizeof(TSF_CHUNK);
  blocks = 1 + ((size + body_payload - 1) / body_payload);
  return blocks * SETUP_TSF_BLOCK_SIZE;
}

u8 setup_copy_sd_to_tsf()
{
  UINT br;
  FRESULT fr;
  TSF_RESULT tr;
  u8 is_dir;
  u32 size;
  u32 done = 0;

  if (setup_sd_mounted == 0) return 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_sd_get_entry(setup_active_entry(0), setup_sd_name_buf, &size, &is_dir) == 0) return 0;
  if (is_dir) return 0;
  if ((setup_sd_name_buf[0] == '.') && (setup_sd_name_buf[1] == '.') && (setup_sd_name_buf[2] == 0)) return 0;
  if (setup_sd_build_path(setup_sd_name_buf) == 0) return 0;

  if (setup_tsf_dir_cache_valid)
  {
    if (setup_tsf_cached_head_by_name(setup_sd_name_buf, 0, 0, 0))
    {
      setup_copy_show_error(SETUP_COPY_ERROR_EXISTS);
      return 0;
    }
  }
  else
  {
    tr = tsf_stat(&setup_tsf_vol, (TSF_FILE_STAT*)(void*)setup_tsf_buf, setup_sd_name_buf);
    if (tr == TSF_RES_OK)
    {
      setup_copy_show_error(SETUP_COPY_ERROR_EXISTS);
      return 0;
    }
    if (tr != TSF_RES_FILE_NOT_FOUND)
    {
      setup_tsf_error = (u8)tr;
      return 0;
    }
  }

  if (setup_tsf_required_space(size, setup_sd_name_buf) > setup_tsf_vol.free)
  {
    setup_copy_show_error(SETUP_COPY_ERROR_NO_SPACE);
    return 0;
  }

  fr = f_open(&setup_sd_file, setup_op_path, FA_READ);
  if (fr != FR_OK)
  {
    setup_sd_error = fr;
    return 0;
  }

  tr = tsf_open(&setup_tsf_vol, &setup_tsf_file, setup_sd_name_buf, TSF_MODE_CREATE_WRITE);
  if (tr != TSF_RES_OK)
  {
    f_close(&setup_sd_file);
    setup_tsf_error = (u8)tr;
    return 0;
  }

  setup_copy_show_window();
  setup_copy_set_progress(0, size);

  while (1)
  {
    fr = f_read(&setup_sd_file, setup_copy_buf, SETUP_COPY_BUF_SIZE, &br);
    if (fr != FR_OK)
    {
      setup_sd_error = fr;
      setup_copy_hide_window();
      tsf_close(&setup_tsf_file);
      f_close(&setup_sd_file);
      tsf_delete(&setup_tsf_vol, setup_sd_name_buf);
      setup_tsf_refresh_panel_light();
      return 0;
    }

    if (br == 0) break;

    tr = tsf_write(&setup_tsf_file, setup_copy_buf, br);
    if (tr != TSF_RES_OK)
    {
      setup_tsf_error = (u8)tr;
      setup_copy_hide_window();
      tsf_close(&setup_tsf_file);
      f_close(&setup_sd_file);
      tsf_delete(&setup_tsf_vol, setup_sd_name_buf);
      setup_tsf_refresh_panel_light();
      return 0;
    }

    done += br;
    setup_copy_set_progress(done, size);
  }

  setup_copy_set_progress(size, size);
  setup_copy_hide_window();
  tsf_close(&setup_tsf_file);
  f_close(&setup_sd_file);
  setup_tsf_refresh_panel_light();
  return 1;
}

u8 setup_copy_tsf_stream_segment_to_sd(u32 addr, u32 size, u32 *done, u32 total, FRESULT *fr_out)
{
  UINT bw;
  u16 chunk;
  u16 i;
  u8 *p;

  setup_tsf_wait_ready();
  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_READ);

  while (size > 0)
  {
    chunk = (size > SETUP_COPY_BUF_SIZE) ? SETUP_COPY_BUF_SIZE : (u16)size;
    p = setup_copy_buf;

    for (i = 0; i < chunk; i++)
      *p++ = spi_flash_read(SPIFL_REG_DATA);

    *fr_out = f_write(&setup_sd_file, setup_copy_buf, chunk, &bw);
    if ((*fr_out != FR_OK) || (bw != chunk))
    {
      spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
      return 0;
    }

    *done += chunk;
    setup_copy_set_progress(*done, total);
    size -= chunk;
  }

  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
  return 1;
}

u8 setup_copy_tsf_advance_read_chunk()
{
  if (setup_tsf_file.next_chunk == 0xFFFF) return 0;

  setup_tsf_file.chunk_addr = setup_tsf_cfg.bulk_start + ((u32)setup_tsf_file.next_chunk * setup_tsf_cfg.block_size);
  setup_tsf_file.chunk_offset = sizeof(TSF_CHUNK);

  if (setup_tsf_hal_read(setup_tsf_file.chunk_addr + sizeof(u32), &setup_tsf_file.next_chunk, sizeof(setup_tsf_file.next_chunk)) != TSF_RES_OK)
    return 0;

  return 1;
}

u8 setup_copy_tsf_to_sd()
{
  FRESULT fr;
  TSF_RESULT tr;
  u32 size;
  u32 total;
  u32 done = 0;
  u32 head_addr = 0;
  u16 chunk;
  u8 head_valid = 0;

  if (setup_sd_mounted == 0) return 0;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_tsf_get_entry_head(setup_active_entry(1), setup_tsf_name_buf, &size, &head_addr, &head_valid) == 0) return 0;
  if (setup_sd_build_path(setup_tsf_name_buf) == 0) return 0;
  total = size;

  if (size > setup_sd_free)
  {
    setup_copy_show_error(SETUP_COPY_ERROR_NO_SPACE);
    return 0;
  }

  tr = setup_tsf_open_read_cached(&setup_tsf_vol, &setup_tsf_file, setup_tsf_name_buf, head_addr, head_valid);
  if (tr != TSF_RES_OK)
  {
    setup_tsf_error = (u8)tr;
    return 0;
  }

  setup_copy_show_window();
  setup_copy_set_progress(0, total);

  fr = f_open(&setup_sd_file, setup_op_path, FA_WRITE | FA_CREATE_NEW);
  if (fr != FR_OK)
  {
    tsf_close(&setup_tsf_file);
    if (fr == FR_EXIST) setup_copy_show_error(SETUP_COPY_ERROR_EXISTS);
    else
    {
      setup_copy_hide_window();
      setup_sd_error = fr;
    }
    return 0;
  }


  while (size > 0)
  {
    if (setup_tsf_file.chunk_offset == setup_tsf_cfg.block_size)
    {
      if (setup_copy_tsf_advance_read_chunk() == 0)
      {
        setup_tsf_error = (u8)TSF_RES_FS_ERROR;
        setup_copy_hide_window();
        tsf_close(&setup_tsf_file);
        f_close(&setup_sd_file);
        f_unlink(setup_op_path);
        setup_sd_refresh_panel();
        return 0;
      }
    }

    chunk = (u16)(setup_tsf_cfg.block_size - setup_tsf_file.chunk_offset);
    if (chunk > size) chunk = (u16)size;

    if (setup_copy_tsf_stream_segment_to_sd(setup_tsf_file.chunk_addr + setup_tsf_file.chunk_offset, chunk, &done, total, &fr) == 0)
    {
      setup_sd_error = fr;
      setup_copy_hide_window();
      tsf_close(&setup_tsf_file);
      f_close(&setup_sd_file);
      f_unlink(setup_op_path);
      setup_sd_refresh_panel();
      return 0;
    }

    setup_tsf_file.seek += chunk;
    setup_tsf_file.chunk_offset += chunk;
    size -= chunk;
  }

  setup_copy_set_progress(total, total);
  setup_copy_hide_window();
  tsf_close(&setup_tsf_file);
  f_close(&setup_sd_file);
  setup_sd_refresh_panel();
  return 1;
}

void setup_copy_active_file()
{
  if (setup_active_panel == 0)
    setup_copy_sd_to_tsf();
  else
    setup_copy_tsf_to_sd();

  setup_draw_panels();
}

void setup_rom_set_progress(u32 done, u32 total)
{
  u8 progress = setup_calc_progress(done, total, SETUP_ROM_BAR_W);

  if (progress == setup_rom_progress) return;
  setup_rom_progress = progress;
  if (setup_rom_visible)
    setup_redraw_progress_row(SETUP_ROM_Y + 13, SETUP_ROM_X, SETUP_ROM_W, SETUP_ROM_BAR_X, SETUP_ROM_BAR_W, setup_rom_progress, SETUP_ATTR_DIALOG);
}

u8 setup_rom_can_active_file()
{
  u8 is_dir = 0;

  setup_delete_name_buf[0] = 0;
  setup_rom_size = 0;
  setup_rom_panel = setup_active_panel;

  if (setup_panels[setup_active_panel].count == 0) return 0;

  if (setup_active_panel == 0)
  {
    if (setup_sd_mounted == 0) return 0;
    if (setup_sd_get_entry(setup_active_entry(0), setup_delete_name_buf, &setup_rom_size, &is_dir) == 0) return 0;
    if (is_dir) return 0;
    if ((setup_delete_name_buf[0] == '.') && (setup_delete_name_buf[1] == '.') && (setup_delete_name_buf[2] == 0)) return 0;
    if (setup_sd_build_path(setup_delete_name_buf) == 0) return 0;
  }
  else
  {
    if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
    if (setup_tsf_get_entry(setup_active_entry(1), setup_delete_name_buf, &setup_rom_size) == 0) return 0;
  }

  if (setup_rom_size == 0) return 0;
  if (setup_rom_size > SETUP_ROM_SIZE) return 0;
  return 1;
}

void setup_rom_put_hex_addr_at(u8 x, u32 addr, u8 attr)
{
  u8 i;
  u8 digit;

  for (i = 0; i < 6; i++)
  {
    digit = (u8)(addr >> (20 - (i * 4))) & 0x0F;
    setup_text_buf[i] = digit < 10 ? (u8)('0' + digit) : (u8)('A' + digit - 10);
  }
  setup_text_buf[6] = 0;
  setup_put_text(x, setup_text_buf, attr);
}

void setup_rom_exit_id_mode()
{
  setup_spi_rom_write(0, 0xF0);
  _delay_us(10);
  setup_spi_rom_write(0x000555UL, 0xF0);
  _delay_us(10);
  setup_spi_rom_write(0x000555UL, 0xAA);
  setup_spi_rom_write(0x0002AAUL, 0x55);
  setup_spi_rom_write(0x000555UL, 0xF0);
  _delay_us(10);
}

void setup_rom_read_chip_id()
{
  setup_rom_exit_id_mode();
  setup_spi_rom_write(0x000555UL, 0xAA);
  setup_spi_rom_write(0x0002AAUL, 0x55);
  setup_spi_rom_write(0x000555UL, 0x90);
  _delay_us(10);
  setup_rom_id_mfr = setup_spi_rom_read(0);
  setup_rom_id_dev0 = setup_spi_rom_read(1);
  setup_rom_id_dev1 = setup_spi_rom_read(2);
  setup_rom_id_dev2 = setup_spi_rom_read(3);
  setup_rom_exit_id_mode();
}

void setup_rom_show_dialog()
{
  if (setup_rom_can_active_file() == 0) return;

  setup_rom_visible = 1;
  setup_rom_running = 0;
  setup_rom_progress = 0;
  setup_rom_error = SETUP_ROM_ERROR_NONE;
  setup_rom_stage = SETUP_ROM_STAGE_CONFIRM;
  setup_rom_cancel = 0;
  setup_rom_start_block = 0;
  setup_rom_read_chip_id();
  setup_help_visible = 0;
  setup_draw_rom_window();
}

u8 setup_rom_open_file()
{
  FRESULT fr;
  TSF_RESULT tr;

  if (setup_rom_panel == 0)
  {
    fr = f_open(&setup_sd_file, setup_op_path, FA_READ);
    if (fr == FR_OK) return 1;
    setup_sd_error = fr;
    setup_rom_error = SETUP_ROM_ERROR_OPEN;
    return 0;
  }

  tr = setup_tsf_open_read_cached_by_name(&setup_tsf_vol, &setup_tsf_file, setup_delete_name_buf);
  if (tr == TSF_RES_OK) return 1;
  setup_tsf_error = (u8)tr;
  setup_rom_error = SETUP_ROM_ERROR_OPEN;
  return 0;
}

void setup_rom_close_file()
{
  if (setup_rom_panel == 0)
    f_close(&setup_sd_file);
  else
    tsf_close(&setup_tsf_file);
}

u8 setup_rom_read_file(u8 *buf, u16 size)
{
  UINT br;
  FRESULT fr;
  TSF_RESULT tr;

  if (setup_rom_panel == 0)
  {
    fr = f_read(&setup_sd_file, buf, size, &br);
    if ((fr == FR_OK) && (br == size)) return 1;
    setup_sd_error = fr;
    return 0;
  }

  tr = tsf_read(&setup_tsf_file, buf, size);
  if (tr == TSF_RES_OK) return 1;
  setup_tsf_error = (u8)tr;
  return 0;
}

void setup_rom_reset_flash()
{
  setup_rom_exit_id_mode();
}

void setup_rom_unlock()
{
  setup_spi_rom_write(0x000555UL, 0xAA);
  setup_spi_rom_write(0x0002AAUL, 0x55);
}

u8 setup_rom_poll_cancel()
{
  u8 code;
  u8 guard = 8;

  if (setup_rom_cancel) return 1;

  while (guard > 0)
  {
    code = ps2keyboard_from_log();
    if (code == 0) return setup_rom_cancel;
    if (code == 0xFF) return setup_rom_cancel;

    if (code == SETUP_KEY_EXT)
    {
      setup_key_ext = 1;
      guard--;
      continue;
    }

    if (code == SETUP_KEY_RELEASE)
    {
      setup_key_release = 1;
      guard--;
      continue;
    }

    if (setup_key_release)
    {
      setup_key_release = 0;
      setup_key_ext = 0;
      guard--;
      continue;
    }

    if ((setup_key_ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_rom_cancel = 1;
      return 1;
    }

    setup_key_ext = 0;
    guard--;
  }

  return setup_rom_cancel;
}

u8 setup_rom_wait_expected(u32 addr, u8 expected, u32 timeout, u8 stable_reads, u8 slow)
{
  u8 value = 0;
  u8 stable = 0;

  while (timeout > 0)
  {
    value = setup_spi_rom_read(addr);
    if (value == expected)
    {
      stable++;
      if (stable >= stable_reads) return 1;
    }
    else
      stable = 0;

    if ((timeout & 0x3F) == 0)
    {
      if (setup_rom_poll_cancel())
      {
        return 0;
      }
    }

    if (slow) _delay_us(50);
    else _delay_us(10);
    timeout--;
  }
  return 0;
}

u8 setup_rom_wait_erase_done(u32 addr)
{
  return setup_rom_wait_expected(addr, 0xFF, SETUP_ROM_ERASE_WAIT_LOOPS,
                                 SETUP_ROM_ERASE_STABLE_READS, 1);
}

u8 setup_rom_wait_toggle(u32 addr, u8 expected, u32 timeout, u8 slow)
{
  u8 first = 0;
  u8 second = 0;
  u8 diff;

  while (timeout > 0)
  {
    first = setup_spi_rom_read(addr);
    second = setup_spi_rom_read(addr);
    diff = first ^ second;

    if ((diff & 0x40) == 0) return 1;

    if (((diff & 0x20) == 0) && (first & 0x20))
    {
      return 0;
    }

    if ((timeout & 0x3F) == 0)
    {
      if (setup_rom_poll_cancel())
      {
        return 0;
      }
    }

    if (slow) _delay_us(50);
    else _delay_us(10);
    timeout--;
  }
  return 0;
}

u8 setup_rom_wait_program_done(u32 addr, u8 expected)
{
  return setup_rom_wait_toggle(addr, expected, SETUP_ROM_PROGRAM_WAIT_LOOPS, 0);
}

u8 setup_rom_erase_block(u32 addr)
{
  setup_rom_reset_flash();
  setup_rom_unlock();
  setup_spi_rom_write(0x000555UL, 0x80);
  setup_rom_unlock();
  setup_spi_rom_write(addr, 0x30);
  _delay_us(100);
  if (setup_rom_wait_erase_done(addr)) return 1;
  setup_rom_reset_flash();
  return 0;
}

u8 setup_rom_verify_erased_block(u32 addr)
{
  u32 offset;
  u8 value;

  for (offset = 0; offset < SETUP_ROM_BLOCK_SIZE; offset++)
  {
    value = setup_spi_rom_read(addr + offset);
    if (value != 0xFF)
    {
      return 0;
    }
    if ((offset & 0xFF) == 0)
    {
      if (setup_rom_poll_cancel()) return 0;
    }
  }

  return 1;
}

u8 setup_rom_program_byte(u32 addr, u8 data)
{
  if (data == 0xFF) return 1;

  setup_rom_unlock();
  setup_spi_rom_write(0x000555UL, 0xA0);
  setup_spi_rom_write(addr, data);
  _delay_us(10);
  if (setup_rom_wait_program_done(addr, data)) return 1;
  setup_rom_reset_flash();
  return 0;
}

u8 setup_rom_erase_blocks(u32 *done, u32 total)
{
  u32 addr;
  u32 block_count;
  u32 block;

  setup_rom_stage = SETUP_ROM_STAGE_ERASE;
  setup_rom_progress = 0;
  setup_draw_panels();

  block_count = (setup_rom_size + SETUP_ROM_BLOCK_SIZE - 1) / SETUP_ROM_BLOCK_SIZE;
  for (block = 0; block < block_count; block++)
  {
    if (setup_rom_poll_cancel()) return 0;
    addr = ((u32)setup_rom_start_block * SETUP_ROM_BLOCK_SIZE) + (block * SETUP_ROM_BLOCK_SIZE);
    if (setup_rom_erase_block(addr) == 0) return 0;
    if (setup_rom_verify_erased_block(addr) == 0) return 0;
    *done += SETUP_ROM_BLOCK_SIZE;
    if (*done > total) *done = total;
    setup_rom_set_progress(*done, total);
  }

  return 1;
}

u8 setup_rom_program_file(u32 *done, u32 total)
{
  u32 addr = ((u32)setup_rom_start_block * SETUP_ROM_BLOCK_SIZE);
  u32 left = setup_rom_size;
  u16 chunk;
  u16 i;

  setup_rom_stage = SETUP_ROM_STAGE_PROGRAM;
  setup_draw_panels();

  if (setup_rom_open_file() == 0) return 0;

  while (left > 0)
  {
    if (setup_rom_poll_cancel())
    {
      setup_rom_close_file();
      return 0;
    }

    chunk = (left > SETUP_COPY_BUF_SIZE) ? SETUP_COPY_BUF_SIZE : (u16)left;
    if (setup_rom_read_file(setup_copy_buf, chunk) == 0)
    {
      setup_rom_close_file();
      setup_rom_error = SETUP_ROM_ERROR_READ;
      return 0;
    }

    for (i = 0; i < chunk; i++)
    {
      if (setup_rom_program_byte(addr + i, setup_copy_buf[i]) == 0)
      {
        setup_rom_close_file();
        setup_rom_error = setup_rom_cancel ? SETUP_ROM_ERROR_CANCEL : SETUP_ROM_ERROR_PROGRAM;
        return 0;
      }
    }

    addr += chunk;
    left -= chunk;
    *done += chunk;
    setup_rom_set_progress(*done, total);
  }

  setup_rom_close_file();
  setup_rom_reset_flash();
  return 1;
}

u8 setup_rom_read_verify_byte(u32 addr, u8 expected, u8 *actual)
{
  u8 retry;
  u8 value;

  value = setup_spi_rom_read(addr);
  if (value == expected)
  {
    *actual = value;
    return 1;
  }

  for (retry = 0; retry < 8; retry++)
  {
    setup_rom_exit_id_mode();
    _delay_us(100);
    value = setup_spi_rom_read(addr);
    if (value == expected)
    {
      *actual = value;
      return 1;
    }
  }

  *actual = value;
  return 0;
}

u8 setup_rom_verify_file(u32 *done, u32 total)
{
  u32 addr = ((u32)setup_rom_start_block * SETUP_ROM_BLOCK_SIZE);
  u32 left = setup_rom_size;
  u16 chunk;
  u16 i;
  u8 value;

  setup_rom_stage = SETUP_ROM_STAGE_VERIFY;
  setup_rom_exit_id_mode();
  _delay_us(100);
  setup_draw_panels();

  if (setup_rom_open_file() == 0) return 0;

  while (left > 0)
  {
    if (setup_rom_poll_cancel())
    {
      setup_rom_close_file();
      return 0;
    }

    chunk = (left > SETUP_COPY_BUF_SIZE) ? SETUP_COPY_BUF_SIZE : (u16)left;
    if (setup_rom_read_file(setup_copy_buf, chunk) == 0)
    {
      setup_rom_close_file();
      setup_rom_error = SETUP_ROM_ERROR_READ;
      return 0;
    }

    for (i = 0; i < chunk; i++)
    {
      if (setup_rom_read_verify_byte(addr + i, setup_copy_buf[i], &value) == 0)
      {
        setup_rom_close_file();
        setup_rom_error = SETUP_ROM_ERROR_VERIFY;
        return 0;
      }
    }

    addr += chunk;
    left -= chunk;
    *done += chunk;
    setup_rom_set_progress(*done, total);
  }

  setup_rom_close_file();
  return 1;
}

void setup_rom_program_active_file()
{
  u32 done = 0;
  u32 total;
  u32 erase_size;

  if (setup_rom_running) return;
  if (setup_rom_size == 0)
  {
    setup_rom_error = SETUP_ROM_ERROR_SIZE;
    setup_draw_panels();
    return;
  }

  erase_size = ((setup_rom_size + SETUP_ROM_BLOCK_SIZE - 1) / SETUP_ROM_BLOCK_SIZE) * SETUP_ROM_BLOCK_SIZE;
  total = erase_size + (setup_rom_size * 2UL);
  setup_rom_running = 1;
  setup_rom_error = SETUP_ROM_ERROR_NONE;
  setup_rom_cancel = 0;
  setup_rom_progress = 0;
  setup_rom_read_chip_id();
  setup_rom_reset_flash();

  if (setup_rom_erase_blocks(&done, total) == 0)
  {
    if (setup_rom_error == SETUP_ROM_ERROR_NONE)
      setup_rom_error = setup_rom_cancel ? SETUP_ROM_ERROR_CANCEL : SETUP_ROM_ERROR_ERASE;
  }
  else if (setup_rom_program_file(&done, total) == 0)
  {
    if (setup_rom_error == SETUP_ROM_ERROR_NONE)
      setup_rom_error = setup_rom_cancel ? SETUP_ROM_ERROR_CANCEL : SETUP_ROM_ERROR_PROGRAM;
  }
  else if (setup_rom_verify_file(&done, total) == 0)
  {
    if (setup_rom_error == SETUP_ROM_ERROR_NONE)
      setup_rom_error = setup_rom_cancel ? SETUP_ROM_ERROR_CANCEL : SETUP_ROM_ERROR_VERIFY;
  }
  else
  {
    setup_rom_stage = SETUP_ROM_STAGE_DONE;
    setup_rom_set_progress(total, total);
  }

  setup_rom_reset_flash();
  setup_rom_running = 0;
  setup_draw_panels();
}


void setup_draw_rom_status(u8 x)
{
  if (setup_rom_error == SETUP_ROM_ERROR_OPEN) setup_put_text_p(x, PSTR("Open file error"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_READ) setup_put_text_p(x, PSTR("Read file error"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_SIZE) setup_put_text_p(x, PSTR("Bad ROM file size"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_ERASE) setup_put_text_p(x, PSTR("ROM erase/blank check error"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_PROGRAM) setup_put_text_p(x, PSTR("ROM program error"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_VERIFY) setup_put_text_p(x, PSTR("ROM verify error"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_error == SETUP_ROM_ERROR_CANCEL) setup_put_text_p(x, PSTR("Cancelled"), SETUP_ATTR_INFO_ALERT);
  else if (setup_rom_stage == SETUP_ROM_STAGE_ERASE) setup_put_text_p(x, PSTR("Erasing ROM blocks..."), SETUP_ATTR_DIALOG);
  else if (setup_rom_stage == SETUP_ROM_STAGE_PROGRAM) setup_put_text_p(x, PSTR("Programming ROM..."), SETUP_ATTR_DIALOG);
  else if (setup_rom_stage == SETUP_ROM_STAGE_VERIFY) setup_put_text_p(x, PSTR("Verifying ROM..."), SETUP_ATTR_DIALOG);
  else if (setup_rom_stage == SETUP_ROM_STAGE_DONE) setup_put_text_p(x, PSTR("ROM programmed and verified"), SETUP_ATTR_DIALOG);
  else setup_put_text_p(x, PSTR("Enter - program   Esc - cancel"), SETUP_ATTR_DIALOG);
}

const char *setup_rom_amd_chip_model(u8 dev)
{
  if (dev == 0xA4) return PSTR("(AMD AM29F040/B)");
  if (dev == 0x77) return PSTR("(AMD AM29F004T)");
  if (dev == 0x7B) return PSTR("(AMD AM29F004B)");
  if (dev == 0x23) return PSTR("(AMD AM29F400T)");
  if (dev == 0xAB) return PSTR("(AMD AM29F400B)");
  if (dev == 0x4F) return PSTR("(AMD AM29LV040B)");
  if (dev == 0xB9) return PSTR("(AMD AM29LV400T)");
  if (dev == 0xBA) return PSTR("(AMD AM29LV400B)");
  return PSTR("(AMD/Spansion unknown)");
}

const char *setup_rom_fujitsu_chip_model(u8 dev)
{
  if (dev == 0xA4) return PSTR("(Fujitsu MBM29F040)");
  if (dev == 0x77) return PSTR("(Fujitsu MBM29F004T)");
  if (dev == 0x7B) return PSTR("(Fujitsu MBM29F004B)");
  if (dev == 0x23) return PSTR("(Fujitsu MBM29F400T)");
  if (dev == 0xAB) return PSTR("(Fujitsu MBM29F400B)");
  if (dev == 0x4F) return PSTR("(Fujitsu MBM29LV040)");
  if (dev == 0xB9) return PSTR("(Fujitsu MBM29LV400T)");
  if (dev == 0xBA) return PSTR("(Fujitsu MBM29LV400B)");
  return PSTR("(Fujitsu unknown)");
}

const char *setup_rom_atmel_chip_model(u8 dev)
{
  if (dev == 0xA4) return PSTR("(Atmel AT29C040A)");
  if (dev == 0xC4) return PSTR("(Atmel AT29BV040A)");
  if (dev == 0x13) return PSTR("(Atmel AT49F040)");
  if (dev == 0xEE) return PSTR("(Atmel AT49LH004)");
  return PSTR("(Atmel unknown)");
}

const char *setup_rom_sst_chip_model(u8 dev)
{
  if (dev == 0xB7) return PSTR("(SST SST39SF040)");
  if (dev == 0xD7) return PSTR("(SST SST39VF040/LF040)");
  if (dev == 0x04) return PSTR("(SST SST28SF040)");
  if (dev == 0x13) return PSTR("(SST SST29SF040)");
  if (dev == 0x14) return PSTR("(SST SST29VF040)");
  if (dev == 0x50) return PSTR("(SST SST49LF040B)");
  if (dev == 0x51) return PSTR("(SST SST49LF040)");
  if (dev == 0x60) return PSTR("(SST SST49LF004A/B)");
  return PSTR("(SST unknown)");
}

const char *setup_rom_macronix_chip_model(u8 dev)
{
  if (dev == 0x46) return PSTR("(Macronix MX29F004B)");
  if (dev == 0x45) return PSTR("(Macronix MX29F004T)");
  if (dev == 0xA4) return PSTR("(Macronix MX29F040)");
  if (dev == 0x23) return PSTR("(Macronix MX29F400T)");
  if (dev == 0xAB) return PSTR("(Macronix MX29F400B)");
  if (dev == 0x4F) return PSTR("(Macronix MX29LV040)");
  if (dev == 0xB9) return PSTR("(Macronix MX29LV400T)");
  if (dev == 0xBA) return PSTR("(Macronix MX29LV400B)");
  return PSTR("(Macronix unknown)");
}

const char *setup_rom_st_chip_model(u8 dev)
{
  if (dev == 0xE2) return PSTR("(ST M29F040B)");
  if (dev == 0xE3) return PSTR("(ST M29W040B)");
  return PSTR("(ST/Micron unknown)");
}

const char *setup_rom_eon_chip_model(u8 dev, u8 ext)
{
  if (dev == 0x02) return PSTR("(EON EN29F040A)");
  if ((dev == 0x7F) && (ext == 0x04)) return PSTR("(EON EN29F040A)");
  if ((dev == 0x7F) && (ext == 0x4F)) return PSTR("(EON EN29LV040A)");
  if (dev == 0x4F) return PSTR("(EON EN29LV040)");
  return PSTR("(EON unknown)");
}

const char *setup_rom_amic_chip_model(u8 dev)
{
  if (dev == 0x86) return PSTR("(AMIC A29040B)");
  if (dev == 0x92) return PSTR("(AMIC A29L040)");
  if (dev == 0x34) return PSTR("(AMIC A29L004T)");
  if (dev == 0xB5) return PSTR("(AMIC A29L004U)");
  return PSTR("(AMIC unknown)");
}

const char *setup_rom_pmc_chip_model(u8 dev)
{
  if (dev == 0x4E) return PSTR("(PMC PM39F040)");
  if (dev == 0x3E) return PSTR("(PMC PM39LV040)");
  if (dev == 0x6E) return PSTR("(PMC PM49FL004)");
  return PSTR("(PMC unknown)");
}

const char *setup_rom_hynix_chip_model(u8 dev)
{
  if (dev == 0xA4) return PSTR("(Hynix HY29F040A)");
  if (dev == 0x23) return PSTR("(Hynix HY29F400T)");
  if (dev == 0xAB) return PSTR("(Hynix HY29F400B)");
  return PSTR("(Hynix unknown)");
}

const char *setup_rom_sharp_chip_model(u8 dev)
{
  if (dev == 0xCF) return PSTR("(Sharp LHF00L04)");
  return PSTR("(Sharp unknown)");
}

const char *setup_rom_chip_model()
{
  if ((setup_rom_id_mfr == 0xFF) && (setup_rom_id_dev0 == 0xFF)) return PSTR("(blank/no ID)");
  if ((setup_rom_id_mfr == 0x00) && (setup_rom_id_dev0 == 0x00)) return PSTR("(no ID/read error)");

  if (setup_rom_id_mfr == 0x01) return setup_rom_amd_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0x04) return setup_rom_fujitsu_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0x1F) return setup_rom_atmel_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0x20) return setup_rom_st_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0x37) return setup_rom_amic_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0x9D) return setup_rom_pmc_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0xAD) return setup_rom_hynix_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0xB0) return setup_rom_sharp_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0xBF) return setup_rom_sst_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0xC2) return setup_rom_macronix_chip_model(setup_rom_id_dev0);
  if (setup_rom_id_mfr == 0xE1) return setup_rom_eon_chip_model(setup_rom_id_dev0, setup_rom_id_dev1);

  if ((setup_rom_id_mfr == 0x7F) && (setup_rom_id_dev0 == 0x1C)) return setup_rom_eon_chip_model(setup_rom_id_dev1, setup_rom_id_dev2);
  if ((setup_rom_id_mfr == 0x7F) && (setup_rom_id_dev0 == 0x37)) return setup_rom_amic_chip_model(setup_rom_id_dev1);
  if ((setup_rom_id_mfr == 0x7F) && (setup_rom_id_dev0 == 0x9D)) return setup_rom_pmc_chip_model(setup_rom_id_dev1);

  return PSTR("(unknown)");
}

void setup_draw_rom_text(u8 rel_row)
{
  u8 x = SETUP_ROM_X + 2;
  u32 addr;

  if (rel_row == 1) setup_put_dialog_title(SETUP_ROM_X, SETUP_ROM_W, PSTR("ROM programmer"));
  else if (rel_row == 3)
  {
    setup_put_text_p(x, PSTR("File:"), SETUP_ATTR_DIALOG);
    setup_draw_delete_name(x + 6, SETUP_ROM_W - 10, SETUP_ATTR_DIALOG_NAME);
  }
  else if (rel_row == 4)
  {
    setup_put_text_p(x, PSTR("Size:"), SETUP_ATTR_DIALOG);
    setup_size_to_text(setup_text_buf, setup_rom_size);
    setup_put_text(x + 6, setup_text_buf, SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 5)
  {
    setup_put_text_p(x, PSTR("Chip ID:"), SETUP_ATTR_DIALOG);
    setup_put_hex_byte(x + 9, setup_rom_id_mfr, SETUP_ATTR_DIALOG_NAME);
    setup_put_hex_byte(x + 12, setup_rom_id_dev0, SETUP_ATTR_DIALOG_NAME);
    setup_put_hex_byte(x + 15, setup_rom_id_dev1, SETUP_ATTR_DIALOG_NAME);
    setup_put_hex_byte(x + 18, setup_rom_id_dev2, SETUP_ATTR_DIALOG_NAME);
    setup_put_text_p(x + 21, setup_rom_chip_model(), SETUP_ATTR_DIALOG_NAME);
  }
  else if (rel_row == 7)
  {
    setup_put_text_p(x, PSTR("Start block:"), SETUP_ATTR_DIALOG);
    setup_put_dec_at(x + 13, setup_rom_start_block, setup_rom_running ? SETUP_ATTR_DIALOG : SETUP_ATTR_DIALOG_ACTIVE);
    addr = ((u32)setup_rom_start_block * SETUP_ROM_BLOCK_SIZE);
    setup_put_cell(x + 16, '(', SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 17, PSTR("0x"), SETUP_ATTR_DIALOG);
    setup_rom_put_hex_addr_at(x + 19, addr, SETUP_ATTR_DIALOG);
    setup_put_cell(x + 25, ')', SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 9) setup_put_text_p(x, PSTR("Warning: selected ROM blocks will be erased"), SETUP_ATTR_INFO_ALERT);
  else if (rel_row == 11) setup_draw_rom_status(x);
  else if ((rel_row == 13) && setup_rom_running)
    setup_draw_progress_bar(SETUP_ROM_BAR_X, SETUP_ROM_BAR_W, setup_rom_progress, SETUP_ATTR_DIALOG);
  else if (rel_row == 15)
  {
    if (setup_rom_running) setup_put_text_p(x, PSTR("Esc - cancel after current operation"), SETUP_ATTR_DIALOG);
    else setup_put_text_p(x, PSTR("Up/Down block   Enter program   Esc close"), SETUP_ATTR_DIALOG);
  }
}


void setup_draw_rom_window_row(u8 row)
{
  u8 rel;

  if (setup_rom_visible == 0) return;
  if (row < SETUP_ROM_Y) return;
  if (row >= (SETUP_ROM_Y + SETUP_ROM_H)) return;

  rel = row - SETUP_ROM_Y;
  setup_draw_dialog_frame_row(SETUP_ROM_X, SETUP_ROM_Y, SETUP_ROM_W, SETUP_ROM_H, row);
  if ((rel != 0) && (rel != (SETUP_ROM_H - 1))) setup_draw_rom_text(rel);
}





u8 setup_delete_can_active_file()
{
  u8 is_dir = 0;

  setup_delete_name_buf[0] = 0;
  setup_delete_panel = setup_active_panel;
  setup_delete_is_dir = 0;

  if (setup_panels[setup_active_panel].count == 0) return 0;

  if (setup_active_panel == 0)
  {
    if (setup_sd_mounted == 0) return 0;
    if (setup_sd_get_entry(setup_active_entry(0), setup_delete_name_buf, 0, &is_dir) == 0) return 0;
    if ((setup_delete_name_buf[0] == '.') && (setup_delete_name_buf[1] == '.') && (setup_delete_name_buf[2] == 0)) return 0;
    setup_delete_is_dir = is_dir;
    return 1;
  }

  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_tsf_get_entry(setup_active_entry(1), setup_delete_name_buf, 0) == 0) return 0;
  return 1;
}

void setup_delete_show_confirm()
{
  if (setup_delete_can_active_file() == 0) return;

  setup_delete_running = 0;
  setup_delete_progress = 0;
  setup_delete_visible = 1;
  setup_draw_delete_window();
}

void setup_draw_delete_name(u8 x, u8 max_len, u8 attr)
{
  u8 i;
  u8 ch;

  for (i = 0; i < max_len; i++)
  {
    ch = setup_delete_name_buf[i];
    if (ch == 0) return;
    setup_put_cell(x + i, ch, attr);
  }
}


void setup_draw_delete_text(u8 rel_row)
{
  u8 x = SETUP_DELETE_X + 2;

  if (setup_delete_running)
  {
    if (rel_row == 1) setup_put_dialog_title(SETUP_DELETE_X, SETUP_DELETE_W, PSTR("Delete"));
    else if (rel_row == 3) setup_put_text_p(x, PSTR("Erasing TSF blocks..."), SETUP_ATTR_DIALOG);
    else if (rel_row == 4) setup_draw_delete_name(x, SETUP_DELETE_W - 4, SETUP_ATTR_DIALOG_NAME);
    else if (rel_row == 6) setup_draw_progress_bar(SETUP_DELETE_BAR_X, SETUP_DELETE_BAR_W, setup_delete_progress, SETUP_ATTR_DIALOG);
    return;
  }

  if (rel_row == 1) setup_put_dialog_title(SETUP_DELETE_X, SETUP_DELETE_W, PSTR("Confirm delete"));
  else if (rel_row == 3) setup_put_text_p(x, PSTR("Delete selected item?"), SETUP_ATTR_INFO_ALERT);
  else if (rel_row == 4) setup_draw_delete_name(x, SETUP_DELETE_W - 4, SETUP_ATTR_DIALOG_NAME);
  else if (rel_row == 6) setup_put_text_p(x, PSTR("Enter - delete   Esc - cancel"), SETUP_ATTR_DIALOG);
}


void setup_draw_delete_window_row(u8 row)
{
  u8 rel;

  if (setup_delete_visible == 0) return;
  if (row < SETUP_DELETE_Y) return;
  if (row >= (SETUP_DELETE_Y + SETUP_DELETE_H)) return;

  rel = row - SETUP_DELETE_Y;
  setup_draw_dialog_frame_row(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H, row);
  if ((rel != 0) && (rel != (SETUP_DELETE_H - 1))) setup_draw_delete_text(rel);
}


void setup_delete_set_progress(u32 done, u32 total)
{
  u8 progress = setup_calc_progress(done, total, SETUP_DELETE_BAR_W);

  if (progress == setup_delete_progress) return;
  setup_delete_progress = progress;
  if ((setup_delete_visible == 0) || (setup_delete_running == 0)) return;

  setup_redraw_progress_row(SETUP_DELETE_Y + 6, SETUP_DELETE_X, SETUP_DELETE_W, SETUP_DELETE_BAR_X, SETUP_DELETE_BAR_W, setup_delete_progress, SETUP_ATTR_DIALOG);
}

void setup_delete_progress_cb(u32 done, u32 total)
{
  setup_delete_set_progress(done, total);
}

void setup_delete_active_file()
{
  FRESULT fr;
  TSF_RESULT tr;

  if (setup_delete_name_buf[0] == 0) return;

  if (setup_delete_panel == 0)
  {
    setup_delete_visible = 0;
    if (setup_sd_mounted == 0) return;
    if ((setup_delete_name_buf[0] == '.') && (setup_delete_name_buf[1] == '.') && (setup_delete_name_buf[2] == 0)) return;
    if (setup_sd_build_path(setup_delete_name_buf) == 0) return;

    fr = f_unlink(setup_op_path);
    if (fr == FR_OK)
      setup_sd_refresh_panel();
    else
    {
      setup_sd_error = fr;
      setup_sd_refresh_panel();
    }
  }
  else
  {
    if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0))
    {
      setup_delete_visible = 0;
      return;
    }

    setup_delete_running = 1;
    setup_delete_progress = 0;
    setup_draw_delete_window();
    setup_tsf_cfg.progress_func = setup_delete_progress_cb;
    tr = tsf_delete(&setup_tsf_vol, setup_delete_name_buf);
    setup_tsf_cfg.progress_func = 0;
    setup_delete_running = 0;
    setup_delete_visible = 0;

    if (tr != TSF_RES_OK)
      setup_tsf_error = (u8)tr;
    setup_tsf_refresh_panel_light();
  }

  setup_delete_name_buf[0] = 0;
  setup_draw_panels();
}

void setup_sd_go_parent()
{
  if (setup_sd_mounted == 0) return;
  if (setup_sd_path[0] == 0) return;

  setup_sd_path_last_name(setup_delete_name_buf);
  setup_sd_path_parent();
  setup_sd_reload_panel();
  setup_sd_select_name(setup_delete_name_buf);
  setup_draw_panels();
}

void setup_sd_open_active()
{
  u8 entry;
  u8 is_dir;

  if (setup_sd_mounted == 0) return;
  if (setup_active_panel != 0) return;
  if (setup_panels[0].count == 0) return;

  entry = setup_active_entry(0);
  if (setup_sd_get_entry(entry, setup_sd_name_buf, 0, &is_dir) == 0) return;
  if (is_dir == 0) return;

  if ((setup_sd_name_buf[0] == '.') && (setup_sd_name_buf[1] == '.') && (setup_sd_name_buf[2] == 0))
  {
    setup_sd_go_parent();
    return;
  }

  if (setup_sd_path_append(setup_sd_name_buf) == 0) return;

  setup_sd_reload_panel();
  setup_draw_panels();
}

u8 setup_panel_count(u8 panel)
{
  if (panel == 0) return setup_sd_count;
  if ((setup_flash_detected == 0) || (setup_tsf_mounted == 0)) return 0;
  if (setup_tsf_vol.files_number > 255) return 255;
  return (u8)setup_tsf_vol.files_number;
}

u8 setup_get_entry_info(u8 panel, u8 index, char *name, u32 *size)
{
  if (name) name[0] = 0;
  if (size) *size = 0;

  if (panel == 0) return setup_sd_get_entry(index, name, size, 0);
  return setup_tsf_get_entry(index, name, size);
}

u8 setup_active_entry(u8 panel)
{
  SetupPanel *p = &setup_panels[panel];

  if (p->count == 0) return 0;
  return p->scroll + p->cursor;
}

u8 setup_u32_to_dec(char *dst, u32 value)
{
  u32 div = 1000000000UL;
  u8 len = 0;
  u8 digit;
  u8 started = 0;

  while (div > 0)
  {
    digit = (u8)(value / div);
    if ((digit != 0) || started || (div == 1))
    {
      dst[len] = (char)('0' + digit);
      len++;
      started = 1;
    }

    value %= div;
    div /= 10;
  }

  dst[len] = 0;
  return len;
}


void setup_put_dec_right(u8 right_x, u32 value, u8 attr)
{
  u8 len;
  u8 x;

  len = setup_u32_to_dec(setup_text_buf, value);
  if (len > right_x) len = right_x;

  x = right_x + 1 - len;
  setup_put_text(x, setup_text_buf, attr);
}

void setup_put_dec_at(u8 x, u32 value, u8 attr)
{
  setup_u32_to_dec(setup_text_buf, value);
  setup_put_text(x, setup_text_buf, attr);
}

void setup_append_char(char *dst, u8 *pos, u8 max, char ch)
{
  if (*pos >= max) return;
  dst[*pos] = ch;
  (*pos)++;
  dst[*pos] = 0;
}

void setup_append_text_buf_p(char *dst, u8 *pos, u8 max, const char *text)
{
  char ch;

  while (1)
  {
    ch = (char)pgm_read_byte(text);
    if (ch == 0) return;
    setup_append_char(dst, pos, max, ch);
    text++;
  }
}

void setup_append_u32(char *dst, u8 *pos, u8 max, u32 value)
{
  u32 div = 1000000000UL;
  u8 digit;
  u8 started = 0;

  while (div > 0)
  {
    digit = (u8)(value / div);
    if ((digit != 0) || started || (div == 1))
    {
      setup_append_char(dst, pos, max, (char)('0' + digit));
      started = 1;
    }

    value %= div;
    div /= 10;
  }
}

void setup_size_to_text(char *dst, u32 value)
{
  u32 whole;
  u32 frac;
  u8 pos = 0;
  const char *unit;

  dst[0] = 0;

  if (value >= (1UL << 30))
  {
    whole = value >> 30;
    frac = ((value & ((1UL << 30) - 1)) >> 20);
    frac = (frac * 100UL) >> 10;
    unit = PSTR(" GiB");
  }
  else if (value >= (1UL << 20))
  {
    whole = value >> 20;
    frac = ((value & ((1UL << 20) - 1)) >> 10);
    frac = (frac * 100UL) >> 10;
    unit = PSTR(" MiB");
  }
  else if (value >= (1UL << 10))
  {
    whole = value >> 10;
    frac = ((value & ((1UL << 10) - 1)) * 100UL) >> 10;
    unit = PSTR(" KiB");
  }
  else
  {
    setup_append_u32(dst, &pos, 18, value);
    setup_append_text_buf_p(dst, &pos, 18, PSTR(" bytes"));
    return;
  }

  setup_append_u32(dst, &pos, 18, whole);
  setup_append_char(dst, &pos, 18, '.');
  setup_append_char(dst, &pos, 18, (char)('0' + (frac / 10)));
  setup_append_char(dst, &pos, 18, (char)('0' + (frac % 10)));
  setup_append_text_buf_p(dst, &pos, 18, unit);
}

void setup_put_hex_byte(u8 x, u8 value, u8 attr)
{
  u8 digit;

  digit = (value >> 4) & 0x0F;
  setup_put_cell(x, digit < 10 ? (u8)('0' + digit) : (u8)('A' + digit - 10), attr);

  digit = value & 0x0F;
  setup_put_cell(x + 1, digit < 10 ? (u8)('0' + digit) : (u8)('A' + digit - 10), attr);
}

void setup_draw_panel_file_info(u8 panel, u8 x)
{
  u8 entry;
  u32 size;

  if (setup_panels[panel].count == 0)
  {
    if ((panel == 0) && (setup_sd_mounted == 0))
      setup_put_text_p(x + 1, PSTR("SD not mounted"), SETUP_ATTR_INFO_ALERT);
    else if ((panel == 1) && (setup_flash_detected == 0))
      setup_put_text_p(x + 1, PSTR("Flash not detected"), SETUP_ATTR_INFO_ALERT);
    return;
  }

  entry = setup_active_entry(panel);
  if (setup_get_entry_info(panel, entry, setup_text_buf, &size) == 0) return;
  setup_put_text(x + 1, setup_text_buf, SETUP_ATTR_INFO);
  setup_put_dec_right(x + SETUP_PANEL_W - 2, size, SETUP_ATTR_INFO);
}

void setup_draw_key_hint()
{
  setup_put_text_p(0, PSTR("F1 Help"), SETUP_ATTR_HINT);
  setup_put_text_p(8, PSTR("F2 Cfg"), SETUP_ATTR_HINT);
  setup_put_text_p(15, PSTR("F5 Copy"), SETUP_ATTR_HINT);
  setup_put_text_p(23, PSTR("F6 ROM"), SETUP_ATTR_HINT);
  setup_put_text_p(30, PSTR("F7 MkDir"), SETUP_ATTR_HINT);
  setup_put_text_p(39, PSTR("F8 Del"), SETUP_ATTR_HINT);
  setup_put_text_p(46, PSTR("F9 Chk"), SETUP_ATTR_HINT);
  setup_put_text_p(53, PSTR("F10 Fmt"), SETUP_ATTR_HINT);
  setup_put_text_p(61, PSTR("F12 Exit"), SETUP_ATTR_HINT);
  setup_draw_stack_debug();
}

void setup_draw_help_text(u8 rel_row)
{
  u8 x = SETUP_HELP_X + 2;

  if (rel_row == 1) setup_put_dialog_title(SETUP_HELP_X, SETUP_HELP_W, PSTR("Help"));
  else if (rel_row == 3) setup_put_text_p(x, PSTR("Tab        Switch panel"), SETUP_ATTR_HELP);
  else if (rel_row == 4) setup_put_text_p(x, PSTR("Up/Down    Move cursor"), SETUP_ATTR_HELP);
  else if (rel_row == 5) setup_put_text_p(x, PSTR("Left/Right Page up/down"), SETUP_ATTR_HELP);
  else if (rel_row == 6) setup_put_text_p(x, PSTR("PgUp/PgDn  Page up/down"), SETUP_ATTR_HELP);
  else if (rel_row == 7) setup_put_text_p(x, PSTR("Home/End   First/last item"), SETUP_ATTR_HELP);
  else if (rel_row == 9) setup_put_text_p(x, PSTR("F2         EEPROM config info"), SETUP_ATTR_HELP);
  else if (rel_row == 10) setup_put_text_p(x, PSTR("F5         Copy file"), SETUP_ATTR_HELP);
  else if (rel_row == 11) setup_put_text_p(x, PSTR("F6         Program ROM from file"), SETUP_ATTR_HELP);
  else if (rel_row == 12) setup_put_text_p(x, PSTR("F7         Create SD folder"), SETUP_ATTR_HELP);
  else if (rel_row == 13) setup_put_text_p(x, PSTR("F8         Delete file"), SETUP_ATTR_HELP);
  else if (rel_row == 14) setup_put_text_p(x, PSTR("F9         Check TSF"), SETUP_ATTR_HELP);
  else if (rel_row == 15) setup_put_text_p(x, PSTR("F10        Format TSF"), SETUP_ATTR_HELP);
  else if (rel_row == 16) setup_put_text_p(x, PSTR("Backspace  Parent SD folder"), SETUP_ATTR_HELP);
  else if (rel_row == 17) setup_put_text_p(x, PSTR("Enter TSF  FPGA file action"), SETUP_ATTR_HELP);
  else if (rel_row == 18) setup_put_text_p(x, PSTR("F12        Boot current config"), SETUP_ATTR_HELP);
  else if (rel_row == 20)
  {
    if (setup_flash_detected == 0) setup_put_text_p(x, PSTR("Flash: not found"), SETUP_ATTR_HELP);
    else
    {
      setup_put_text_p(x, PSTR("Flash: "), SETUP_ATTR_HELP);
      if ((setup_flash_capacity >> 20) > 0)
      {
        setup_put_dec_right(x + 10, setup_flash_capacity >> 20, SETUP_ATTR_HELP);
        setup_put_text_p(x + 12, PSTR("MB"), SETUP_ATTR_HELP);
      }
      else
      {
        setup_put_dec_right(x + 10, setup_flash_capacity >> 10, SETUP_ATTR_HELP);
        setup_put_text_p(x + 12, PSTR("KB"), SETUP_ATTR_HELP);
      }
    }
  }
  else if (rel_row == 22)
  {
    setup_put_text_p(x, PSTR("JEDEC ID:"), SETUP_ATTR_HELP);
    if (setup_flash_detected)
    {
      setup_put_hex_byte(x + 10, setup_flash_jedec_mfr, SETUP_ATTR_HELP);
      setup_put_cell(x + 12, ' ', SETUP_ATTR_HELP);
      setup_put_hex_byte(x + 13, setup_flash_jedec_type, SETUP_ATTR_HELP);
      setup_put_cell(x + 15, ' ', SETUP_ATTR_HELP);
      setup_put_hex_byte(x + 16, setup_flash_jedec_capacity, SETUP_ATTR_HELP);
    }
    else
      setup_put_text_p(x + 10, PSTR("not available"), SETUP_ATTR_HELP);
  }
  else if (rel_row == 23) setup_put_text_p(x, PSTR("F1 or Esc  Close help"), SETUP_ATTR_HELP);
}

void setup_draw_help_window_row(u8 row)
{
  u8 rel_row;
  u8 x;
  u8 right;

  if (setup_help_visible == 0) return;
  if (row < SETUP_HELP_Y) return;
  if (row >= (SETUP_HELP_Y + SETUP_HELP_H)) return;

  rel_row = row - SETUP_HELP_Y;
  right = SETUP_HELP_X + SETUP_HELP_W - 1;

  for (x = SETUP_HELP_X; x <= right; x++)
    setup_put_cell(x, ' ', SETUP_ATTR_HELP);

  if (rel_row == 0)
  {
    setup_put_cell(SETUP_HELP_X, SETUP_BOX_TL, SETUP_ATTR_HELP);
    for (x = SETUP_HELP_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_HELP);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_HELP);
    return;
  }

  if (rel_row == (SETUP_HELP_H - 1))
  {
    setup_put_cell(SETUP_HELP_X, SETUP_BOX_BL, SETUP_ATTR_HELP);
    for (x = SETUP_HELP_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_HELP);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_HELP);
    return;
  }

  setup_put_cell(SETUP_HELP_X, SETUP_BOX_V, SETUP_ATTR_HELP);
  setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_HELP);
  setup_draw_help_text(rel_row);
}


void setup_eeprom_draw_name(u8 x, u8 max_len)
{
  if (setup_delete_name_buf[0] == 0)
  {
    setup_put_text_p(x, PSTR("<empty>"), SETUP_ATTR_DIALOG);
    return;
  }

  setup_draw_delete_name(x, max_len, SETUP_ATTR_DIALOG_NAME);
}

u8 setup_put_dec_move(u8 x, u32 value, u8 attr)
{
  u8 len = setup_u32_to_dec(setup_text_buf, value);

  setup_put_text(x, setup_text_buf, attr);
  return x + len;
}

u8 setup_eeprom_put_tlv_tag(u8 x, u8 tag)
{
  if (tag == CFG_TAG_SIG)
  {
    setup_put_text_p(x, PSTR("SIG"), SETUP_ATTR_DIALOG_NAME);
    return 3;
  }

  if (tag == CFG_TAG_VER)
  {
    setup_put_text_p(x, PSTR("VER"), SETUP_ATTR_DIALOG_NAME);
    return 3;
  }

  if (tag == CFG_TAG_BSTREAM)
  {
    setup_put_text_p(x, PSTR("BSTREAM"), SETUP_ATTR_DIALOG_NAME);
    return 7;
  }

  if (tag == CFG_TAG_ROM)
  {
    setup_put_text_p(x, PSTR("ROM"), SETUP_ATTR_DIALOG_NAME);
    return 3;
  }

  if (tag == CFG_TAG_ISBASE)
  {
    setup_put_text_p(x, PSTR("ISBASE"), SETUP_ATTR_DIALOG_NAME);
    return 6;
  }

  if (tag == CFG_TAG_JOYSTICK)
  {
    setup_put_text_p(x, PSTR("JOY"), SETUP_ATTR_DIALOG_NAME);
    return 3;
  }

  setup_put_text_p(x, PSTR("0x"), SETUP_ATTR_DIALOG_NAME);
  setup_put_hex_byte(x + 2, tag, SETUP_ATTR_DIALOG_NAME);
  return 4;
}

void setup_eeprom_draw_tlv_item(u8 x, u8 tag, u8 len)
{
  u8 w;

  w = setup_eeprom_put_tlv_tag(x, tag);
  setup_put_cell(x + w, '(', SETUP_ATTR_DIALOG);
  x = setup_put_dec_move(x + w + 1, len, SETUP_ATTR_DIALOG);
  setup_put_cell(x, ')', SETUP_ATTR_DIALOG);
}

void setup_eeprom_draw_tlv_records(u8 rel_row)
{
  u16 ptr = 0;
  u8 record = 0;
  u8 line = rel_row - 9;
  u8 slot;
  u8 tag;
  u8 len;

  if (cfg_config_valid() == 0)
  {
    if (rel_row == 9) setup_put_text_p(SETUP_EEPROM_X + 4, PSTR("<not initialized>"), SETUP_ATTR_INFO_ALERT);
    return;
  }

  while (ptr < EEPROM_SIZE_BOOT_CFG)
  {
    tag = eeprom_read_byte(EEPROM_ADDR_BOOT_CFG + ptr++);
    if (tag == CFG_TAG_END) return;
    if (ptr >= EEPROM_SIZE_BOOT_CFG)
    {
      if (rel_row == 9) setup_put_text_p(SETUP_EEPROM_X + 4, PSTR("<invalid>"), SETUP_ATTR_INFO_ALERT);
      return;
    }

    len = eeprom_read_byte(EEPROM_ADDR_BOOT_CFG + ptr++);
    if ((u16)(ptr + len) > EEPROM_SIZE_BOOT_CFG)
    {
      if (rel_row == 9) setup_put_text_p(SETUP_EEPROM_X + 4, PSTR("<invalid>"), SETUP_ATTR_INFO_ALERT);
      return;
    }

    if ((record / 3) == line)
    {
      slot = record % 3;
      setup_eeprom_draw_tlv_item(SETUP_EEPROM_X + 4 + slot * 16, tag, len);
    }

    record++;
    ptr += len;
  }
}

void setup_draw_eeprom_text(u8 rel_row)
{
  u8 x = SETUP_EEPROM_X + 2;
  u8 pos;

  if (rel_row == 1)
  {
    setup_put_dialog_title(SETUP_EEPROM_X, SETUP_EEPROM_W, PSTR("EEPROM config"));
  }
  else if (rel_row == 3)
  {
    setup_put_text_p(x, PSTR("TLV: 0x0200-0x05FF  Status:"), SETUP_ATTR_DIALOG);
    if (cfg_config_valid())
    {
      setup_put_text_p(x + 31, PSTR("valid"), SETUP_ATTR_DIALOG_NAME);
    }
    else
    {
      setup_put_text_p(x + 31, PSTR("not initialized"), SETUP_ATTR_INFO_ALERT);
    }
  }
  else if (rel_row == 4)
  {
    setup_put_text_p(x, PSTR("Used/total:"), SETUP_ATTR_DIALOG);
    pos = setup_put_dec_move(x + 12, cfg_get_eeprom_used_size(), SETUP_ATTR_DIALOG);
    setup_put_cell(pos++, '/', SETUP_ATTR_DIALOG);
    pos = setup_put_dec_move(pos, EEPROM_SIZE_BOOT_CFG, SETUP_ATTR_DIALOG);
    setup_put_text_p(pos + 3, PSTR("Records:"), SETUP_ATTR_DIALOG);
    setup_put_dec_move(pos + 12, cfg_get_eeprom_record_count(), SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 6)
  {
    setup_put_text_p(x, PSTR("Bitstream:"), SETUP_ATTR_DIALOG);
    cfg_get_boot_bitstream(setup_delete_name_buf, SETUP_NAME_MAX + 1);
    setup_eeprom_draw_name(x + 12, SETUP_EEPROM_W - 16);
  }
  else if (rel_row == 8)
  {
    setup_put_text_p(x, PSTR("TLV records:"), SETUP_ATTR_DIALOG);
  }
  else if ((rel_row >= 9) && (rel_row <= 11))
  {
    setup_eeprom_draw_tlv_records(rel_row);
  }
  else if (rel_row == 15)
  {
    setup_put_text_p(x, PSTR("Enter/Esc - close"), SETUP_ATTR_DIALOG);
  }
}

void setup_draw_eeprom_window_row(u8 row)
{
  u8 rel_row;
  u8 x;
  u8 right;

  if (setup_eeprom_visible == 0) return;
  if (row < SETUP_EEPROM_Y) return;
  if (row >= (SETUP_EEPROM_Y + SETUP_EEPROM_H)) return;

  rel_row = row - SETUP_EEPROM_Y;
  right = SETUP_EEPROM_X + SETUP_EEPROM_W - 1;

  for (x = SETUP_EEPROM_X; x <= right; x++)
    setup_put_cell(x, ' ', SETUP_ATTR_DIALOG);

  if (rel_row == 0)
  {
    setup_put_cell(SETUP_EEPROM_X, SETUP_BOX_TL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_EEPROM_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_DIALOG_FRAME);
    return;
  }

  if (rel_row == (SETUP_EEPROM_H - 1))
  {
    setup_put_cell(SETUP_EEPROM_X, SETUP_BOX_BL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_EEPROM_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_DIALOG_FRAME);
    return;
  }

  setup_put_cell(SETUP_EEPROM_X, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
  setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
  setup_draw_eeprom_text(rel_row);
}

void setup_eeprom_show_dialog()
{
  u8 need_redraw = setup_any_dialog_visible();

  setup_help_visible = 0;
  setup_format_visible = 0;
  setup_chkdsk_visible = 0;
  setup_fpga_visible = 0;
  setup_mkdir_visible = 0;
  setup_copy_confirm_visible = 0;
  setup_delete_visible = 0;
  setup_eeprom_visible = 1;
  if (need_redraw) setup_draw_panels();
  else setup_draw_eeprom_window();
}

void setup_draw_format_choice(u8 option, const char *text)
{
  u8 attr = SETUP_ATTR_DIALOG;
  u8 x = SETUP_FORMAT_X + 5;

  if (setup_format_option == option) attr = SETUP_ATTR_DIALOG_ACTIVE;

  setup_put_text_p(x, text, attr);
}


void setup_draw_format_text(u8 rel_row)
{
  u8 x = SETUP_FORMAT_X + 3;

  if (rel_row == 1) setup_put_dialog_title(SETUP_FORMAT_X, SETUP_FORMAT_W, PSTR("Format TSF"));
  else if (rel_row == 3) setup_draw_format_choice(SETUP_FORMAT_FAST, PSTR("Fast: bulk erase"));
  else if (rel_row == 4) setup_draw_format_choice(SETUP_FORMAT_NORMAL, PSTR("Normal: block erase"));
  else if (rel_row == 5) setup_draw_format_choice(SETUP_FORMAT_SLOW, PSTR("Slow: sector erase"));
  else if (rel_row == 7)
  {
    setup_put_text_p(x + 2, PSTR("[ ] Check erased blocks"), SETUP_ATTR_DIALOG);
    if (setup_format_check) setup_put_cell(x + 3, 'x', SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 9) setup_put_text_p(x, PSTR("Warning: all TSF files will be lost"), SETUP_ATTR_INFO_ALERT);
  else if ((rel_row == 11) && setup_format_running)
  {
    if (setup_format_stage == SETUP_FORMAT_STAGE_ERASE)
      setup_put_text_p(x, PSTR("Erase... Esc - stop"), SETUP_ATTR_DIALOG);
    else if (setup_format_stage == SETUP_FORMAT_STAGE_CHECK)
      setup_put_text_p(x, PSTR("Check... Esc - stop"), SETUP_ATTR_DIALOG);
    else if (setup_format_stage == SETUP_FORMAT_STAGE_FORMAT)
      setup_put_text_p(x, PSTR("Format... Esc - stop"), SETUP_ATTR_DIALOG);
    else
      setup_put_text_p(x, PSTR("Working... Esc - stop"), SETUP_ATTR_DIALOG);
  }
  else if ((rel_row == 11) && setup_format_cancel)
    setup_put_text_p(x, PSTR("Cancelled"), SETUP_ATTR_DIALOG);
  else if ((rel_row == 11) && setup_tsf_error)
  {
    setup_put_text_p(x, PSTR("Error:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(SETUP_FORMAT_X + SETUP_FORMAT_W - 4, setup_tsf_error, SETUP_ATTR_DIALOG);
  }
  else if ((rel_row == 11) && (setup_format_free_blocks || setup_format_invalid_blocks))
  {
    setup_put_text_p(x, PSTR("Blocks free:"), SETUP_ATTR_DIALOG);
    setup_put_dec_at(x + 13, setup_format_free_blocks, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 22, PSTR("invalid:"), SETUP_ATTR_DIALOG);
    setup_put_dec_at(x + 31, setup_format_invalid_blocks, SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 11) setup_put_text_p(x, PSTR("Space - check, Enter - start"), SETUP_ATTR_DIALOG);
  if ((rel_row == 13) && setup_format_running)
    setup_draw_progress_bar(SETUP_FORMAT_BAR_X, SETUP_FORMAT_BAR_W, setup_format_progress, SETUP_ATTR_DIALOG_FRAME);
}


void setup_draw_format_window_row(u8 row)
{
  u8 rel_row;

  if (setup_format_visible == 0) return;
  if (row < SETUP_FORMAT_Y) return;
  if (row >= (SETUP_FORMAT_Y + SETUP_FORMAT_H)) return;

  rel_row = row - SETUP_FORMAT_Y;
  setup_draw_dialog_frame_row(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H, row);
  if ((rel_row != 0) && (rel_row != (SETUP_FORMAT_H - 1))) setup_draw_format_text(rel_row);
}





void setup_draw_chkdsk_choice(u8 option, const char *text)
{
  u8 attr = SETUP_ATTR_DIALOG;
  u8 x = SETUP_FORMAT_X + 5;

  if (setup_chkdsk_option == option) attr = SETUP_ATTR_DIALOG_ACTIVE;

  setup_put_text_p(x, text, attr);
}


void setup_draw_chkdsk_stats_line(u8 rel_row)
{
  u8 x = SETUP_FORMAT_X + 3;
  u8 c1 = x + 12;
  u8 c2 = x + 29;
  u8 c3 = x + 46;

  if (setup_chkdsk_has_stats == 0) return;

  if (rel_row == 9)
  {
    setup_put_text_p(x, PSTR("Free:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c1, setup_chkdsk_stats.free_blocks, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 17, PSTR("Used:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c2, setup_chkdsk_stats.used_blocks, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 34, PSTR("Bad:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c3, setup_chkdsk_stats.invalid_blocks, SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 10)
  {
    setup_put_text_p(x, PSTR("Broken:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c1, setup_chkdsk_stats.broken_files, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 17, PSTR("Size:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c2, setup_chkdsk_stats.wrong_size_files, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 34, PSTR("Lost:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c3, setup_chkdsk_stats.lost_blocks, SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 11)
  {
    setup_put_text_p(x, PSTR("Fixed:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c1, setup_chkdsk_stats.fixed_blocks, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 17, PSTR("Failed:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c2, setup_chkdsk_stats.failed_blocks, SETUP_ATTR_DIALOG);
    setup_put_text_p(x + 34, PSTR("Files:"), SETUP_ATTR_DIALOG);
    setup_put_dec_right(c3, setup_chkdsk_stats.files, SETUP_ATTR_DIALOG);
  }
}

void setup_draw_chkdsk_text(u8 rel_row)
{
  u8 x = SETUP_FORMAT_X + 3;

  if (rel_row == 1) setup_put_dialog_title(SETUP_FORMAT_X, SETUP_FORMAT_W, PSTR("Check TSF"));
  else if (rel_row == 3) setup_draw_chkdsk_choice(SETUP_CHK_SCAN, PSTR("Scan"));
  else if (rel_row == 4) setup_draw_chkdsk_choice(SETUP_CHK_SCAN_FIX, PSTR("Scan and fix"));
  else if (rel_row == 5) setup_draw_chkdsk_choice(SETUP_CHK_RETEST, PSTR("Re-test invalid blocks"));
  else if (rel_row == 7)
  {
    if (setup_chkdsk_running)
    {
      if (setup_chkdsk_stage == SETUP_CHK_STAGE_SCAN) setup_put_text_p(x, PSTR("Scan... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_CHECK) setup_put_text_p(x, PSTR("Check... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_BLANK) setup_put_text_p(x, PSTR("Blankcheck... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_ERASE) setup_put_text_p(x, PSTR("Erase... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_VERIFY) setup_put_text_p(x, PSTR("Verify... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_FORMAT) setup_put_text_p(x, PSTR("Format... Esc - stop"), SETUP_ATTR_DIALOG);
      else if (setup_chkdsk_stage == SETUP_CHK_STAGE_REPAIR) setup_put_text_p(x, PSTR("Repair... Esc - stop"), SETUP_ATTR_DIALOG);
      else setup_put_text_p(x, PSTR("Checking... Esc - stop"), SETUP_ATTR_DIALOG);
    }
    else if (setup_chkdsk_cancel) setup_put_text_p(x, PSTR("Cancelled"), SETUP_ATTR_DIALOG);
    else if (setup_tsf_error)
    {
      setup_put_text_p(x, PSTR("Error:"), SETUP_ATTR_DIALOG);
      setup_put_dec_right(SETUP_FORMAT_X + SETUP_FORMAT_W - 4, setup_tsf_error, SETUP_ATTR_DIALOG);
    }
    else setup_put_text_p(x, PSTR("Esc - cancel, Enter - start"), SETUP_ATTR_DIALOG);
  }
  else if (setup_chkdsk_has_stats) setup_draw_chkdsk_stats_line(rel_row);

  if ((rel_row == 12) && setup_chkdsk_running)
    setup_draw_progress_bar(SETUP_FORMAT_BAR_X, SETUP_FORMAT_BAR_W, setup_chkdsk_progress, SETUP_ATTR_DIALOG_FRAME);
}


void setup_draw_chkdsk_window_row(u8 row)
{
  u8 rel_row;

  if (setup_chkdsk_visible == 0) return;
  if (row < SETUP_FORMAT_Y) return;
  if (row >= (SETUP_FORMAT_Y + SETUP_FORMAT_H)) return;

  rel_row = row - SETUP_FORMAT_Y;
  setup_draw_dialog_frame_row(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H, row);
  if ((rel_row != 0) && (rel_row != (SETUP_FORMAT_H - 1))) setup_draw_chkdsk_text(rel_row);
}





void setup_draw_chkdsk_window()
{
  u8 row;

  setup_dialog_backup_save_if_needed(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H);

  for (row = SETUP_FORMAT_Y; row < (SETUP_FORMAT_Y + SETUP_FORMAT_H); row++)
  {
    setup_draw_chkdsk_window_row(row);
    setup_write_row_range(row, SETUP_FORMAT_X, SETUP_FORMAT_W);
  }
}

void setup_start_chkdsk()
{
  TSF_RESULT rc;

  setup_tsf_error = 0;
  setup_chkdsk_cancel = 0;
  setup_chkdsk_running = 1;
  setup_chkdsk_has_stats = 0;
  setup_draw_chkdsk_window();
  rc = setup_tsf_chkdsk_run();
  setup_chkdsk_running = 0;
  setup_chkdsk_stage = SETUP_CHK_STAGE_IDLE;
  setup_chkdsk_has_stats = 1;
  setup_tsf_mount_volume();
  setup_tsf_rebuild_dir_cache();
  if (setup_chkdsk_cancel) setup_tsf_error = 0;
  else setup_tsf_error = (u8)rc;
  setup_panels[1].cursor = 0;
  setup_panels[1].scroll = 0;
  setup_panels[1].count = setup_panel_count(1);
  setup_panel_fix_cursor(&setup_panels[1]);
  setup_draw_panels();
}


void setup_draw_fpga_text(u8 rel_row)
{
  u8 x = SETUP_FPGA_X + 2;
  if (rel_row == 1) setup_put_dialog_title(SETUP_FPGA_X, SETUP_FPGA_W, PSTR("FPGA config"));
  else if (rel_row == 3)
  {
    setup_put_text_p(x, PSTR("File:"), SETUP_ATTR_DIALOG);
    setup_put_text(x + 6, setup_tsf_name_buf, SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 5)
  {
    setup_put_text_p(x + 2, PSTR("Set as current config"), setup_fpga_option == SETUP_FPGA_SET_CURRENT ? SETUP_ATTR_DIALOG_ACTIVE : SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 6)
  {
    setup_put_text_p(x + 2, PSTR("Load and run now"), setup_fpga_option == SETUP_FPGA_LOAD_NOW ? SETUP_ATTR_DIALOG_ACTIVE : SETUP_ATTR_DIALOG);
  }
  else if (rel_row == 8)
  {
    if (setup_fpga_error == 1) setup_put_text_p(x, PSTR("EEPROM write error"), SETUP_ATTR_INFO_ALERT);
    else if (setup_fpga_error == 2) setup_put_text_p(x, PSTR("TSF read error"), SETUP_ATTR_INFO_ALERT);
    else if (setup_fpga_error == 3) setup_put_text_p(x, PSTR("FPGA config start error"), SETUP_ATTR_INFO_ALERT);
    else setup_put_text_p(x, PSTR("Esc - cancel, Enter - select"), SETUP_ATTR_DIALOG);
  }
}

void setup_draw_fpga_window_row(u8 row)
{
  u8 rel_row;
  u8 x;
  u8 right;

  if (setup_fpga_visible == 0) return;
  if (row < SETUP_FPGA_Y) return;
  if (row >= (SETUP_FPGA_Y + SETUP_FPGA_H)) return;

  rel_row = row - SETUP_FPGA_Y;
  right = SETUP_FPGA_X + SETUP_FPGA_W - 1;

  for (x = SETUP_FPGA_X; x <= right; x++)
    setup_put_cell(x, ' ', SETUP_ATTR_DIALOG);

  if (rel_row == 0)
  {
    setup_put_cell(SETUP_FPGA_X, SETUP_BOX_TL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_FPGA_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_DIALOG_FRAME);
  }
  else if (rel_row == (SETUP_FPGA_H - 1))
  {
    setup_put_cell(SETUP_FPGA_X, SETUP_BOX_BL, SETUP_ATTR_DIALOG_FRAME);
    for (x = SETUP_FPGA_X + 1; x < right; x++)
      setup_put_cell(x, SETUP_BOX_H, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_DIALOG_FRAME);
  }
  else
  {
    setup_put_cell(SETUP_FPGA_X, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_DIALOG_FRAME);
    setup_draw_fpga_text(rel_row);
  }
}

void setup_draw_dialogs_row(u8 row)
{
  setup_draw_help_window_row(row);
  setup_draw_eeprom_window_row(row);
  setup_draw_format_window_row(row);
  setup_draw_chkdsk_window_row(row);
  setup_draw_fpga_window_row(row);
  setup_draw_mkdir_window_row(row);
  setup_draw_copy_confirm_window_row(row);
  setup_draw_copy_window_row(row);
  setup_draw_rom_window_row(row);
  setup_draw_delete_window_row(row);
}

void setup_draw_dialog_area(u8 x, u8 y, u8 w, u8 h)
{
  u8 row;

  setup_dialog_backup_save_if_needed(x, y, w, h);

  for (row = y; row < (y + h); row++)
  {
    setup_draw_dialogs_row(row);
    setup_write_row_range(row, x, w);
  }
}

void setup_draw_help_window()
{
  setup_draw_dialog_area(SETUP_HELP_X, SETUP_HELP_Y, SETUP_HELP_W, SETUP_HELP_H);
}

void setup_draw_eeprom_window()
{
  setup_draw_dialog_area(SETUP_EEPROM_X, SETUP_EEPROM_Y, SETUP_EEPROM_W, SETUP_EEPROM_H);
}

void setup_draw_fpga_window()
{
  setup_draw_dialog_area(SETUP_FPGA_X, SETUP_FPGA_Y, SETUP_FPGA_W, SETUP_FPGA_H);
}

void setup_draw_mkdir_window()
{
  setup_draw_dialog_area(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
}

void setup_draw_copy_confirm_window()
{
  setup_draw_dialog_area(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
}

void setup_draw_rom_window()
{
  setup_draw_dialog_area(SETUP_ROM_X, SETUP_ROM_Y, SETUP_ROM_W, SETUP_ROM_H);
}

void setup_draw_delete_window()
{
  setup_draw_dialog_area(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
}

void setup_draw_format_window()
{
  u8 row;

  setup_dialog_backup_save_if_needed(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H);

  for (row = SETUP_FORMAT_Y; row < (SETUP_FORMAT_Y + SETUP_FORMAT_H); row++)
  {
    setup_draw_format_window_row(row);
    setup_write_row_range(row, SETUP_FORMAT_X, SETUP_FORMAT_W);
  }
}


u8 setup_format_poll_cancel()
{
  u8 code;
  u8 guard = 16;

  if (setup_format_cancel) return 1;

  while (guard > 0)
  {
    ps2keyboard_task();
    code = ps2keyboard_from_log();
    if (code == 0) return setup_format_cancel;
    if (code == 0xFF) return setup_format_cancel;

    if (code == SETUP_KEY_EXT)
    {
      setup_key_ext = 1;
      guard--;
      continue;
    }

    if (code == SETUP_KEY_RELEASE)
    {
      setup_key_release = 1;
      guard--;
      continue;
    }

    if (setup_key_release)
    {
      setup_key_release = 0;
      setup_key_ext = 0;
      guard--;
      continue;
    }

    if ((setup_key_ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_format_cancel = 1;
      return 1;
    }

    setup_key_ext = 0;
    guard--;
  }

  return setup_format_cancel;
}


void setup_format_set_progress(u16 done, u16 total)
{
  u8 progress = setup_calc_progress(done, total, SETUP_FORMAT_BAR_W);

  if (progress == setup_format_progress) return;
  setup_format_progress = progress;
  if (setup_format_visible)
    setup_redraw_progress_row(SETUP_FORMAT_Y + 13, SETUP_FORMAT_X, SETUP_FORMAT_W, SETUP_FORMAT_BAR_X, SETUP_FORMAT_BAR_W, setup_format_progress, SETUP_ATTR_DIALOG_FRAME);
}

void setup_format_begin_stage(u8 stage)
{
  if ((setup_format_stage == stage) && (setup_format_progress == 0)) return;
  setup_format_stage = stage;
  setup_format_progress = 0;
  if (setup_format_visible) setup_draw_format_window();
}

TSF_RESULT setup_format_tsf_poll()
{
  if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_blankcheck_stream(u32 addr, u32 size, u8 (*poll_func)())
{
  u16 poll_left = 0;
  u8 value;

  while (spi_flash_read(SPIFL_REG_STAT) & SPIFL_STAT_BUSY)
  {
    if (poll_func && poll_func()) return TSF_RES_FS_ERROR;
  }

  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_READ);

  while (size > 0)
  {
    if (poll_left == 0)
    {
      if (poll_func && poll_func())
      {
        spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
        return TSF_RES_FS_ERROR;
      }

      poll_left = SETUP_TSF_BLOCK_SIZE;
    }

    value = spi_flash_read(SPIFL_REG_DATA);
    if (value != 0xFF)
    {
      spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
      return TSF_RES_FS_ERROR;
    }

    size--;
    poll_left--;
  }

  spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_END);
  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_blankcheck_stream_format(u32 addr, u32 size)
{
  return setup_tsf_blankcheck_stream(addr, size, setup_format_poll_cancel);
}

void setup_format_badmap_clear()
{
  u16 i;

  for (i = 0; i < SETUP_COPY_BUF_SIZE; i++)
    setup_copy_buf[i] = 0;
}

u8 setup_format_badmap_can_track()
{
  return setup_tsf_total_blocks <= ((u16)SETUP_COPY_BUF_SIZE * 8U);
}

void setup_format_badmap_set(u16 block)
{
  setup_copy_buf[block >> 3] |= (u8)(1 << (block & 7));
}

u8 setup_format_badmap_get(u16 block)
{
  return (setup_copy_buf[block >> 3] & (u8)(1 << (block & 7))) != 0;
}

void setup_format_badmap_clear_block(u16 block)
{
  setup_copy_buf[block >> 3] &= (u8)~(1 << (block & 7));
}


u16 setup_format_badmap_count()
{
  u16 block;
  u16 count = 0;

  for (block = 0; block < setup_tsf_total_blocks; block++)
  {
    if (setup_format_badmap_get(block)) count++;
  }

  return count;
}

u8 setup_tsf_wait_ready_format()
{
  while (spi_flash_read(SPIFL_REG_STAT) & SPIFL_STAT_BUSY)
  {
    if (setup_format_poll_cancel()) return 1;
  }

  return 0;
}

TSF_RESULT setup_tsf_hal_erase_cmd_format(u32 addr, u8 cmd)
{
  if (setup_tsf_wait_ready_format()) return TSF_RES_FS_ERROR;
  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, cmd);
  if (setup_tsf_wait_ready_format()) return TSF_RES_FS_ERROR;
  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_hal_erase_range_format(u32 addr, u32 size)
{
  if ((addr == SETUP_TSF_BULK_START) && (size == setup_tsf_bulk_size))
  {
    if (setup_tsf_wait_ready_format()) return TSF_RES_FS_ERROR;
    spi_flash_write(SPIFL_REG_CMD, SPIFL_CMD_ERSBLK);
    if (setup_tsf_wait_ready_format()) return TSF_RES_FS_ERROR;
    return TSF_RES_OK;
  }

  while (size > 0)
  {
    if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
    if (setup_tsf_hal_erase_cmd_format(addr, SPIFL_CMD_ERSSEC) != TSF_RES_OK) return TSF_RES_FS_ERROR;
    addr += SETUP_TSF_BLOCK_SIZE;
    if (size < SETUP_TSF_BLOCK_SIZE) break;
    size -= SETUP_TSF_BLOCK_SIZE;
  }

  return TSF_RES_OK;
}

u8 setup_tsf_probe_erase_cmd(u8 cmd, u32 erase_size)
{
  u8 marker = 0;
  u8 value = 0;

  if (setup_tsf_bulk_size < erase_size) return 0;
  if ((SETUP_TSF_BULK_START & (erase_size - 1)) != 0) return 0;
  if (setup_format_poll_cancel()) return 0;

  if (setup_tsf_hal_write(SETUP_TSF_BULK_START, &marker, 1) != TSF_RES_OK) return 0;
  if (setup_format_poll_cancel()) return 0;
  if (setup_tsf_hal_erase_cmd_format(SETUP_TSF_BULK_START, cmd) != TSF_RES_OK) return 0;
  if (setup_format_poll_cancel()) return 0;
  if (setup_tsf_hal_read(SETUP_TSF_BULK_START, &value, 1) != TSF_RES_OK) return 0;
  return value == 0xFF;
}

TSF_RESULT setup_tsf_format_block_erase(u8 cmd, u32 erase_size)
{
  u32 addr;
  u16 block;
  u16 done = 0;
  u16 blocks_per_erase = (u16)(erase_size / SETUP_TSF_BLOCK_SIZE);
  TSF_RESULT rc;

  for (block = 0; block < setup_tsf_total_blocks; block += blocks_per_erase)
  {
    if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
    addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
    rc = setup_tsf_hal_erase_cmd_format(addr, cmd);
    if (rc != TSF_RES_OK) return rc;
    done += blocks_per_erase;
    if (done > setup_tsf_total_blocks) done = setup_tsf_total_blocks;
    setup_format_set_progress(done, setup_tsf_total_blocks);
  }

  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_format_selected()
{
  enum
  {
    tsf_erase_block_32k = 32768UL,
    tsf_erase_block_64k = 65536UL
  };
  u16 block;
  u32 addr;
  TSF_RESULT rc;
  u8 block_erase_cmd = 0;
  u32 block_erase_size = 0;

  setup_tsf_config_init();
  setup_tsf_cfg.poll_func = setup_format_tsf_poll;
  setup_format_free_blocks = 0;
  setup_format_invalid_blocks = 0;

  if (setup_flash_detected == 0) return TSF_RES_FS_ERROR;
  setup_format_check_active = setup_format_check;

  if (setup_format_check_active && (setup_format_badmap_can_track() == 0)) return TSF_RES_FS_ERROR;

  if (setup_format_check_active) setup_format_badmap_clear();

  setup_format_begin_stage(SETUP_FORMAT_STAGE_ERASE);

  if (setup_format_option == SETUP_FORMAT_FAST)
  {
    if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
    rc = setup_tsf_hal_erase_range_format(SETUP_TSF_BULK_START, setup_tsf_bulk_size);
    if (rc != TSF_RES_OK) return rc;
    setup_format_set_progress(setup_tsf_total_blocks, setup_tsf_total_blocks);
  }
  else
  {
    if (setup_format_option == SETUP_FORMAT_NORMAL)
    {
      if (setup_tsf_probe_erase_cmd(SPIFL_CMD_ERS32K, tsf_erase_block_32k))
      {
        block_erase_cmd = SPIFL_CMD_ERS32K;
        block_erase_size = tsf_erase_block_32k;
      }
      else if (setup_tsf_probe_erase_cmd(SPIFL_CMD_ERS64K, tsf_erase_block_64k))
      {
        block_erase_cmd = SPIFL_CMD_ERS64K;
        block_erase_size = tsf_erase_block_64k;
      }
    }

    if (block_erase_cmd != 0)
    {
      rc = setup_tsf_format_block_erase(block_erase_cmd, block_erase_size);
      if (rc != TSF_RES_OK) return rc;
    }
    else
    {
      for (block = 0; block < setup_tsf_total_blocks; block++)
      {
        if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
        addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
        rc = setup_tsf_hal_erase_cmd_format(addr, SPIFL_CMD_ERSSEC);
        if (rc != TSF_RES_OK) return rc;
        setup_format_set_progress(block + 1, setup_tsf_total_blocks);
      }
    }
  }

  if (setup_format_check_active)
  {
    setup_format_begin_stage(SETUP_FORMAT_STAGE_CHECK);

    for (block = 0; block < setup_tsf_total_blocks; block++)
    {
      if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
      addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
      rc = setup_tsf_blankcheck_stream_format(addr, SETUP_TSF_BLOCK_SIZE);
      if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
      if (rc != TSF_RES_OK)
      {
        setup_format_badmap_set(block);
        setup_format_invalid_blocks++;
      }

      setup_format_set_progress(block + 1, setup_tsf_total_blocks);
    }
  }

  setup_format_begin_stage(SETUP_FORMAT_STAGE_FORMAT);

  for (block = 0; block < setup_tsf_total_blocks; block++)
  {
    if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
    addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);

    if (setup_format_check_active && setup_format_badmap_get(block))
    {
      setup_format_set_progress(block + 1, setup_tsf_total_blocks);
      continue;
    }

    rc = tsf_init_erased_chunk(&setup_tsf_cfg, addr);
    if (rc != TSF_RES_OK) return rc;
    if (setup_format_poll_cancel()) return TSF_RES_FS_ERROR;
    setup_format_free_blocks++;
    setup_format_set_progress(block + 1, setup_tsf_total_blocks);
  }

  return TSF_RES_OK;
}

void setup_start_format()
{
  TSF_RESULT rc;

  setup_tsf_error = 0;
  setup_format_cancel = 0;
  setup_format_stage = SETUP_FORMAT_STAGE_IDLE;
  setup_format_running = 1;
  setup_draw_format_window();
  rc = setup_tsf_format_selected();
  setup_format_running = 0;
  setup_format_stage = SETUP_FORMAT_STAGE_IDLE;
  setup_tsf_mount_volume();
  setup_tsf_rebuild_dir_cache();
  if (setup_format_cancel) setup_tsf_error = 0;
  else setup_tsf_error = (u8)rc;
  setup_panels[1].cursor = 0;
  setup_panels[1].scroll = 0;
  setup_panels[1].count = setup_panel_count(1);
  setup_panel_fix_cursor(&setup_panels[1]);

  if (rc == TSF_RES_OK)
  {
    setup_format_free_blocks = setup_tsf_vol.free / SETUP_TSF_BLOCK_SIZE;
    setup_format_invalid_blocks = setup_tsf_total_blocks - setup_tsf_vol.chunks_number;
  }

  setup_draw_panels();
}


u8 setup_chkdsk_poll_cancel()
{
  u8 code;
  u8 guard = 8;

  if (setup_chkdsk_cancel) return 1;

  while (guard > 0)
  {
    ps2keyboard_task();
    code = ps2keyboard_from_log();
    if (code == 0) return setup_chkdsk_cancel;
    if (code == 0xFF) return setup_chkdsk_cancel;

    if (code == SETUP_KEY_EXT)
    {
      setup_key_ext = 1;
      guard--;
      continue;
    }

    if (code == SETUP_KEY_RELEASE)
    {
      setup_key_release = 1;
      guard--;
      continue;
    }

    if (setup_key_release)
    {
      setup_key_release = 0;
      setup_key_ext = 0;
      guard--;
      continue;
    }

    if ((setup_key_ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_chkdsk_cancel = 1;
      return 1;
    }

    setup_key_ext = 0;
    guard--;
  }

  return setup_chkdsk_cancel;
}

TSF_RESULT setup_chkdsk_tsf_poll()
{
  if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
  return TSF_RES_OK;
}

TSF_RESULT setup_tsf_blankcheck_stream_chkdsk(u32 addr, u32 size)
{
  return setup_tsf_blankcheck_stream(addr, size, setup_chkdsk_poll_cancel);
}

u8 setup_tsf_wait_ready_chkdsk()
{
  while (spi_flash_read(SPIFL_REG_STAT) & SPIFL_STAT_BUSY)
  {
    if (setup_chkdsk_poll_cancel()) return 1;
  }

  return 0;
}

TSF_RESULT setup_tsf_hal_erase_cmd_chkdsk(u32 addr, u8 cmd)
{
  if (setup_tsf_wait_ready_chkdsk()) return TSF_RES_FS_ERROR;
  setup_tsf_set_flash_addr(addr);
  spi_flash_write(SPIFL_REG_CMD, cmd);
  if (setup_tsf_wait_ready_chkdsk()) return TSF_RES_FS_ERROR;
  return TSF_RES_OK;
}

u8 setup_tsf_read_chunk(u16 block, TSF_CHUNK *chunk)
{
  u32 addr;

  if (block >= setup_tsf_total_blocks) return 0;
  addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
  setup_tsf_hal_read(addr, chunk, sizeof(TSF_CHUNK));
  return 1;
}

u8 setup_tsf_chunk_valid(u16 block, TSF_CHUNK *chunk)
{
  if (setup_tsf_read_chunk(block, chunk) == 0) return 0;
  if (chunk->magic != TSF_MAGIC) return 0;
  return 1;
}

u8 setup_tsf_chain_reaches(u16 head_block, u16 target_block)
{
  TSF_CHUNK chunk;
  u16 cur = head_block;
  u16 guard = 0;

  if (setup_chkdsk_poll_cancel()) return 1;
  if (setup_tsf_chunk_valid(cur, &chunk) == 0) return 0;
  if (chunk.type != (u8)TSF_CHUNK_HEAD) return 0;
  if (cur == target_block) return 1;

  while (chunk.next_chunk != 0xFFFF)
  {
    if (setup_chkdsk_poll_cancel()) return 1;
    cur = chunk.next_chunk;
    guard++;
    if (guard > setup_tsf_total_blocks) return 0;
    if (cur >= setup_tsf_total_blocks) return 0;
    if (setup_tsf_chunk_valid(cur, &chunk) == 0) return 0;
    if (chunk.type != (u8)TSF_CHUNK_BODY) return 0;
    if (cur == target_block) return 1;
  }

  return 0;
}

u8 setup_tsf_body_referenced(u16 body_block)
{
  u16 block;
  TSF_CHUNK chunk;

  for (block = 0; block < setup_tsf_total_blocks; block++)
  {
    if (setup_chkdsk_poll_cancel()) return 1;
    if (setup_tsf_chunk_valid(block, &chunk) == 0) continue;
    if (chunk.type != (u8)TSF_CHUNK_HEAD) continue;
    if (setup_tsf_chain_reaches(block, body_block)) return 1;
    if (setup_chkdsk_cancel) return 1;
  }

  return 0;
}

void setup_chkdsk_reset_stats()
{
  setup_chkdsk_stats.free_blocks = 0;
  setup_chkdsk_stats.used_blocks = 0;
  setup_chkdsk_stats.invalid_blocks = 0;
  setup_chkdsk_stats.files = 0;
  setup_chkdsk_stats.broken_files = 0;
  setup_chkdsk_stats.wrong_size_files = 0;
  setup_chkdsk_stats.lost_blocks = 0;
  setup_chkdsk_stats.fixed_blocks = 0;
  setup_chkdsk_stats.failed_blocks = 0;
}

TSF_RESULT setup_tsf_recover_block(u16 block)
{
  u32 addr;
  TSF_RESULT rc;

  if (block >= setup_tsf_total_blocks) return TSF_RES_FS_ERROR;
  addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);

  if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
  rc = setup_tsf_hal_erase_cmd_chkdsk(addr, SPIFL_CMD_ERSSEC);
  if (rc != TSF_RES_OK) return rc;
  if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;

  rc = tsf_init_erased_chunk(&setup_tsf_cfg, addr);
  if (rc != TSF_RES_OK) return rc;
  if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
  return TSF_RES_OK;
}

void setup_tsf_update_recover_stats(TSF_CHUNK *chunk)
{
  setup_chkdsk_stats.fixed_blocks++;

  if (chunk->magic != TSF_MAGIC)
  {
    if (setup_chkdsk_stats.invalid_blocks) setup_chkdsk_stats.invalid_blocks--;
    setup_chkdsk_stats.free_blocks++;
    return;
  }

  if (chunk->type == (u8)TSF_CHUNK_FREE) return;

  if (setup_chkdsk_stats.used_blocks) setup_chkdsk_stats.used_blocks--;
  setup_chkdsk_stats.free_blocks++;
}

TSF_RESULT setup_tsf_recover_file_chain(u16 head_block)
{
  TSF_CHUNK chunk;
  TSF_RESULT rc;
  u16 cur = head_block;
  u16 next;
  u16 guard = 0;
  u8 first = 1;

  while (cur < setup_tsf_total_blocks)
  {
    if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
    setup_tsf_read_chunk(cur, &chunk);
    next = chunk.next_chunk;

    if (chunk.magic == TSF_MAGIC)
    {
      if (first)
      {
        if (chunk.type != (u8)TSF_CHUNK_HEAD) return TSF_RES_FS_ERROR;
      }
      else if (chunk.type != (u8)TSF_CHUNK_BODY)
        return TSF_RES_OK;
    }

    rc = setup_tsf_recover_block(cur);
    if (rc != TSF_RES_OK)
    {
      if (setup_chkdsk_cancel) return rc;
      setup_chkdsk_stats.failed_blocks++;
      return rc;
    }

    setup_tsf_update_recover_stats(&chunk);
    if ((chunk.magic != TSF_MAGIC) || (next == 0xFFFF)) return TSF_RES_OK;

    first = 0;
    cur = next;
    guard++;
    if (guard > setup_tsf_total_blocks) return TSF_RES_OK;
  }

  return TSF_RES_OK;
}

u8 setup_tsf_check_head_file(u16 head_block, u8 count_stats, u8 mark_refs)
{
  TSF_CHUNK chunk;
  TSF_HDR hdr;
  u32 addr;
  u32 capacity = 0;
  u16 cur = head_block;
  u16 guard = 0;
  u8 broken = 0;
  u8 wrong = 0;

  if (setup_chkdsk_poll_cancel()) return 1;
  if (setup_tsf_chunk_valid(cur, &chunk) == 0) return 0;
  if (chunk.type != (u8)TSF_CHUNK_HEAD) return 0;

  addr = SETUP_TSF_BULK_START + ((u32)cur * SETUP_TSF_BLOCK_SIZE);
  setup_tsf_hal_read(addr + sizeof(TSF_CHUNK), &hdr, sizeof(hdr));
  if (hdr.fnlen > SETUP_NAME_MAX) wrong = 1;
  if (hdr.size == 0xFFFFFFFFUL) wrong = 1;

  if ((sizeof(TSF_CHUNK) + sizeof(TSF_HDR) + hdr.fnlen) < SETUP_TSF_BLOCK_SIZE)
    capacity = SETUP_TSF_BLOCK_SIZE - sizeof(TSF_CHUNK) - sizeof(TSF_HDR) - hdr.fnlen;
  else
    wrong = 1;

  while (chunk.next_chunk != 0xFFFF)
  {
    if (setup_chkdsk_poll_cancel()) return 1;
    cur = chunk.next_chunk;
    guard++;
    if (guard > setup_tsf_total_blocks)
    {
      broken = 1;
      break;
    }

    if (cur >= setup_tsf_total_blocks)
    {
      broken = 1;
      break;
    }

    if (setup_tsf_chunk_valid(cur, &chunk) == 0)
    {
      broken = 1;
      break;
    }

    if (chunk.type != (u8)TSF_CHUNK_BODY)
    {
      broken = 1;
      break;
    }

    if (mark_refs) setup_format_badmap_set(cur);
    capacity += SETUP_TSF_BLOCK_SIZE - sizeof(TSF_CHUNK);
  }

  if (hdr.size > capacity) wrong = 1;

  if (count_stats)
  {
    setup_chkdsk_stats.files++;
    if (broken) setup_chkdsk_stats.broken_files++;
    if (wrong) setup_chkdsk_stats.wrong_size_files++;
  }

  if (broken || wrong) return 0;
  return 1;
}

void setup_chkdsk_set_progress(u16 done, u16 total)
{
  u8 progress = setup_calc_progress(done, total, SETUP_FORMAT_BAR_W);

  if (progress == setup_chkdsk_progress) return;
  setup_chkdsk_progress = progress;
  if (setup_chkdsk_visible)
    setup_redraw_progress_row(SETUP_FORMAT_Y + 12, SETUP_FORMAT_X, SETUP_FORMAT_W, SETUP_FORMAT_BAR_X, SETUP_FORMAT_BAR_W, setup_chkdsk_progress, SETUP_ATTR_DIALOG_FRAME);
}

void setup_chkdsk_begin_stage(u8 stage)
{
  if ((setup_chkdsk_stage == stage) && (setup_chkdsk_progress == 0)) return;
  setup_chkdsk_stage = stage;
  setup_chkdsk_progress = 0;
  if (setup_chkdsk_visible) setup_draw_chkdsk_window();
}

TSF_RESULT setup_tsf_chkdsk_run()
{
  u16 block;
  TSF_CHUNK chunk;
  TSF_RESULT rc;
  u32 addr;
  u16 count;
  u16 total;
  u8 refmap_active = 0;

  if (setup_flash_detected == 0) return TSF_RES_FS_ERROR;

  setup_tsf_config_init();
  setup_tsf_cfg.poll_func = setup_chkdsk_tsf_poll;
  setup_chkdsk_reset_stats();

  if (setup_chkdsk_option == SETUP_CHK_RETEST)
  {
    if (setup_format_badmap_can_track() == 0) return TSF_RES_FS_ERROR;
    setup_format_badmap_clear();
  }
  else if (setup_format_badmap_can_track())
  {
    setup_format_badmap_clear();
    refmap_active = 1;
  }

  setup_chkdsk_begin_stage(SETUP_CHK_STAGE_SCAN);

  for (block = 0; block < setup_tsf_total_blocks; block++)
  {
    if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
    setup_tsf_read_chunk(block, &chunk);

    if (chunk.magic != TSF_MAGIC)
    {
      setup_chkdsk_stats.invalid_blocks++;
      if (setup_chkdsk_option == SETUP_CHK_RETEST) setup_format_badmap_set(block);
    }
    else if (chunk.type == (u8)TSF_CHUNK_FREE)
      setup_chkdsk_stats.free_blocks++;
    else
      setup_chkdsk_stats.used_blocks++;

    if ((setup_chkdsk_option != SETUP_CHK_RETEST) &&
        (chunk.magic == TSF_MAGIC) &&
        (chunk.type == (u8)TSF_CHUNK_HEAD))
      setup_tsf_check_head_file(block, 1, refmap_active);

    setup_chkdsk_set_progress(block + 1, setup_tsf_total_blocks);
  }

  if (setup_chkdsk_option == SETUP_CHK_RETEST)
  {
    total = setup_chkdsk_stats.invalid_blocks;
    if (total == 0) return TSF_RES_OK;

    setup_chkdsk_begin_stage(SETUP_CHK_STAGE_BLANK);
    count = 0;
    for (block = 0; block < setup_tsf_total_blocks; block++)
    {
      if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
      if (setup_format_badmap_get(block))
      {
        addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
        rc = setup_tsf_blankcheck_stream_chkdsk(addr, SETUP_TSF_BLOCK_SIZE);
        if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
        if (rc == TSF_RES_OK) setup_format_badmap_clear_block(block);
        count++;
        setup_chkdsk_set_progress(count, total);
      }
    }

    total = setup_format_badmap_count();
    if (total)
    {
      setup_chkdsk_begin_stage(SETUP_CHK_STAGE_ERASE);
      count = 0;
      for (block = 0; block < setup_tsf_total_blocks; block++)
      {
        if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
        if (setup_format_badmap_get(block))
        {
          addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
          rc = setup_tsf_hal_erase_cmd_chkdsk(addr, SPIFL_CMD_ERSSEC);
          if ((rc != TSF_RES_OK) && setup_chkdsk_cancel) return TSF_RES_FS_ERROR;
          count++;
          setup_chkdsk_set_progress(count, total);
        }
      }

      setup_chkdsk_begin_stage(SETUP_CHK_STAGE_VERIFY);
      count = 0;
      for (block = 0; block < setup_tsf_total_blocks; block++)
      {
        if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
        if (setup_format_badmap_get(block))
        {
          addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
          rc = setup_tsf_blankcheck_stream_chkdsk(addr, SETUP_TSF_BLOCK_SIZE);
          if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
          if (rc == TSF_RES_OK) setup_format_badmap_clear_block(block);
          count++;
          setup_chkdsk_set_progress(count, total);
        }
      }
    }

    setup_chkdsk_begin_stage(SETUP_CHK_STAGE_FORMAT);
    count = 0;
    total = setup_chkdsk_stats.invalid_blocks;
    for (block = 0; block < setup_tsf_total_blocks; block++)
    {
      if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
      setup_tsf_read_chunk(block, &chunk);
      if (chunk.magic != TSF_MAGIC)
      {
        if (setup_format_badmap_get(block))
        {
          setup_chkdsk_stats.failed_blocks++;
        }
        else
        {
          addr = SETUP_TSF_BULK_START + ((u32)block * SETUP_TSF_BLOCK_SIZE);
          rc = tsf_init_erased_chunk(&setup_tsf_cfg, addr);
          if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
          if (rc == TSF_RES_OK)
          {
            setup_chkdsk_stats.fixed_blocks++;
            if (setup_chkdsk_stats.invalid_blocks) setup_chkdsk_stats.invalid_blocks--;
            setup_chkdsk_stats.free_blocks++;
          }
          else
            setup_chkdsk_stats.failed_blocks++;
        }

        count++;
        setup_chkdsk_set_progress(count, total);
      }
    }

    return TSF_RES_OK;
  }

  setup_chkdsk_begin_stage(setup_chkdsk_option == SETUP_CHK_SCAN_FIX ? SETUP_CHK_STAGE_REPAIR : SETUP_CHK_STAGE_CHECK);

  for (block = 0; block < setup_tsf_total_blocks; block++)
  {
    if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
    setup_tsf_read_chunk(block, &chunk);

    if ((setup_chkdsk_option == SETUP_CHK_SCAN_FIX) &&
        (chunk.magic == TSF_MAGIC) &&
        (chunk.type == (u8)TSF_CHUNK_HEAD))
    {
      if (setup_tsf_check_head_file(block, 0, 0) == 0)
      {
        if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
        rc = setup_tsf_recover_file_chain(block);
        if ((rc != TSF_RES_OK) && setup_chkdsk_cancel) return TSF_RES_FS_ERROR;
      }
    }
    else if ((chunk.magic == TSF_MAGIC) && (chunk.type == (u8)TSF_CHUNK_BODY))
    {
      if (((refmap_active) && (setup_format_badmap_get(block) == 0)) ||
          ((refmap_active == 0) && (setup_tsf_body_referenced(block) == 0)))
      {
        if (setup_chkdsk_poll_cancel()) return TSF_RES_FS_ERROR;
        setup_chkdsk_stats.lost_blocks++;
        if (setup_chkdsk_option == SETUP_CHK_SCAN_FIX)
        {
          rc = setup_tsf_recover_block(block);
          if (rc == TSF_RES_OK) setup_tsf_update_recover_stats(&chunk);
          else
          {
            if (setup_chkdsk_cancel) return TSF_RES_FS_ERROR;
            setup_chkdsk_stats.failed_blocks++;
          }
        }
      }
    }

    setup_chkdsk_set_progress(block + 1, setup_tsf_total_blocks);
  }

  return TSF_RES_OK;
}


void setup_draw_tsf_no_flash_box(u8 row)
{
  u8 box_x = SETUP_PANEL_RIGHT_X + 3;
  u8 box_y = 9;
  u8 box_w = 34;
  u8 box_h = 7;
  u8 rel;
  u8 i;
  u8 right = box_x + box_w - 1;

  if (row < box_y) return;
  if (row >= (box_y + box_h)) return;

  rel = row - box_y;

  if (rel == 0)
  {
    setup_put_cell(box_x, SETUP_BOX_TL, SETUP_ATTR_FRAME);
    for (i = 1; i < (box_w - 1); i++)
      setup_put_cell(box_x + i, SETUP_BOX_H, SETUP_ATTR_FRAME);
    setup_put_cell(right, SETUP_BOX_TR, SETUP_ATTR_FRAME);
    return;
  }

  if (rel == (box_h - 1))
  {
    setup_put_cell(box_x, SETUP_BOX_BL, SETUP_ATTR_FRAME);
    for (i = 1; i < (box_w - 1); i++)
      setup_put_cell(box_x + i, SETUP_BOX_H, SETUP_ATTR_FRAME);
    setup_put_cell(right, SETUP_BOX_BR, SETUP_ATTR_FRAME);
    return;
  }

  setup_put_cell(box_x, SETUP_BOX_V, SETUP_ATTR_FRAME);
  setup_put_cell(right, SETUP_BOX_V, SETUP_ATTR_FRAME);

  if (rel == 1) setup_put_text_p(box_x + 12, PSTR("SPI flash"), SETUP_ATTR(SETUP_COLOR_BRIGHT_RED, SETUP_COLOR_BLUE));
  else if (rel == 3) setup_put_text_p(box_x + 8, PSTR("Flash not detected"), SETUP_ATTR(SETUP_COLOR_BRIGHT_RED, SETUP_COLOR_BLUE));
  else if (rel == 4) setup_put_text_p(box_x + 8, PSTR("Check SPI flash"), SETUP_ATTR(SETUP_COLOR_BRIGHT_RED, SETUP_COLOR_BLUE));
}

void setup_put_entry(u8 panel, u8 x, u8 entry_row)
{
  SetupPanel *p = &setup_panels[panel];
  u8 entry = p->scroll + entry_row;
  u8 attr = SETUP_ATTR_PANEL;
  u8 i;
  u8 ch;

  if ((panel == 0) && (setup_sd_mounted == 0)) return;
  if ((panel == 1) && ((setup_flash_detected == 0) || (setup_tsf_mounted == 0))) return;

  if (entry_row == p->cursor)
  {
    if (panel == setup_active_panel)
      attr = SETUP_ATTR_CURSOR_ACTIVE;
    else
      attr = SETUP_ATTR_CURSOR_INACTIVE;
  }

  for (i = 0; i < SETUP_PANEL_INNER_W; i++)
    setup_put_cell(x + 1 + i, ' ', attr);

  if (entry >= p->count) return;

  if (panel == 0)
  {
    if (setup_sd_get_entry(entry, setup_sd_name_buf, 0, 0) == 0) return;
    for (i = 0; i < (SETUP_PANEL_INNER_W - 2); i++)
    {
      ch = setup_sd_name_buf[i];
      if (ch == 0) return;
      setup_put_cell(x + 2 + i, ch, attr);
    }

    return;
  }

  if (setup_tsf_get_entry(entry, setup_tsf_name_buf, 0) == 0) return;
  for (i = 0; i < (SETUP_PANEL_INNER_W - 2); i++)
  {
    ch = setup_tsf_name_buf[i];
    if (ch == 0) return;
    setup_put_cell(x + 2 + i, ch, attr);
  }
}

void setup_draw_panel_entries(u8 row)
{
  u8 entry_row;

  if (row >= SETUP_PANEL_H) return;
  if ((row == 0) || (row == (SETUP_PANEL_H - 1))) return;

  entry_row = row - 1;
  setup_put_entry(0, SETUP_PANEL_LEFT_X, entry_row);
  if (setup_flash_detected == 0) setup_draw_tsf_no_flash_box(row);
  else setup_put_entry(1, SETUP_PANEL_RIGHT_X, entry_row);
}

void setup_clear_range(u8 x, u8 w, u8 attr)
{
  u8 i;

  for (i = 0; i < w; i++)
    setup_put_cell(x + i, ' ', attr);
}

void setup_fill_attr_range(u8 x, u8 w, u8 attr)
{
  u8 i;

  for (i = 0; i < w; i++)
    dbuf[SETUP_ATTR_BUF_OFS + x + i] = attr;
}

void setup_draw_panel_info_only(u8 panel)
{
  u8 x = (panel == 0) ? SETUP_PANEL_LEFT_X : SETUP_PANEL_RIGHT_X;

  setup_clear_range(x, SETUP_PANEL_W, SETUP_ATTR_INFO);
  setup_draw_panel_file_info(panel, x);
  setup_write_row_range(SETUP_PANEL_INFO_ROW, x, SETUP_PANEL_W);
}

void setup_draw_panel_cursor_move(u8 panel, u8 old_cursor, u8 new_cursor)
{
  u8 x = ((panel == 0) ? SETUP_PANEL_LEFT_X : SETUP_PANEL_RIGHT_X) + 1;

  setup_fill_attr_range(x, SETUP_PANEL_INNER_W, SETUP_ATTR_PANEL);
  setup_write_attr_row_range(old_cursor + 1, x, SETUP_PANEL_INNER_W);

  setup_fill_attr_range(x, SETUP_PANEL_INNER_W, SETUP_ATTR_CURSOR_ACTIVE);
  setup_write_attr_row_range(new_cursor + 1, x, SETUP_PANEL_INNER_W);

  setup_draw_panel_info_only(panel);
}

void setup_draw_panel_cursor_attr(u8 panel, u8 attr)
{
  u8 x;
  SetupPanel *p = &setup_panels[panel];

  if (p->count == 0) return;

  x = ((panel == 0) ? SETUP_PANEL_LEFT_X : SETUP_PANEL_RIGHT_X) + 1;
  setup_fill_attr_range(x, SETUP_PANEL_INNER_W, attr);
  setup_write_attr_row_range(p->cursor + 1, x, SETUP_PANEL_INNER_W);
}

void setup_draw_panel_active_switch(u8 old_panel, u8 new_panel)
{
  setup_draw_panel_cursor_attr(old_panel, SETUP_ATTR_CURSOR_INACTIVE);
  setup_draw_panel_cursor_attr(new_panel, SETUP_ATTR_CURSOR_ACTIVE);
}

void setup_draw_panels_area(u8 x, u8 y, u8 w, u8 h)
{
  u8 row;


  for (row = y; row < (y + h); row++)
  {
    if (row < SETUP_PANEL_H)
      setup_clear_line(' ', SETUP_ATTR_PANEL);
    else if (row == SETUP_PANEL_HINT_ROW)
      setup_clear_line(' ', SETUP_ATTR_HINT);
    else
      setup_clear_line(' ', SETUP_ATTR_INFO);

    if (row < SETUP_PANEL_H)
    {
      setup_put_panel_frame(SETUP_PANEL_LEFT_X, SETUP_PANEL_W, row);
      setup_put_panel_frame(SETUP_PANEL_RIGHT_X, SETUP_PANEL_W, row);
      setup_draw_panel_entries(row);
      if (row == 0) setup_put_titles();
    }
    else
    {
      if (row == SETUP_PANEL_INFO_ROW)
      {
        setup_draw_panel_file_info(0, SETUP_PANEL_LEFT_X);
        setup_draw_panel_file_info(1, SETUP_PANEL_RIGHT_X);
      }
      else if (row == SETUP_PANEL_HINT_ROW)
      {
        setup_draw_key_hint();
      }
    }

    setup_write_row_range(row, x, w);
  }
}

void setup_draw_panels()
{
  enum
  {
    screen_rows = 30
  };
  u8 row;
  u8 backup_active;

  setup_dialog_backup_valid = 0;
  backup_active = setup_dialog_get_rect(&setup_dialog_backup_x, &setup_dialog_backup_y,
                                        &setup_dialog_backup_w, &setup_dialog_backup_h);


  for (row = 0; row < screen_rows; row++)
  {
    if (row < SETUP_PANEL_H)
      setup_clear_line(' ', SETUP_ATTR_PANEL);
    else if (row == SETUP_PANEL_HINT_ROW)
      setup_clear_line(' ', SETUP_ATTR_HINT);
    else
      setup_clear_line(' ', SETUP_ATTR_INFO);

    if (row < SETUP_PANEL_H)
    {
      setup_put_panel_frame(SETUP_PANEL_LEFT_X, SETUP_PANEL_W, row);
      setup_put_panel_frame(SETUP_PANEL_RIGHT_X, SETUP_PANEL_W, row);
      setup_draw_panel_entries(row);
      if (row == 0) setup_put_titles();
    }
    else
    {
      if (row == SETUP_PANEL_INFO_ROW)
      {
        setup_draw_panel_file_info(0, SETUP_PANEL_LEFT_X);
        setup_draw_panel_file_info(1, SETUP_PANEL_RIGHT_X);
      }
      else if (row == SETUP_PANEL_HINT_ROW)
      {
        setup_draw_key_hint();
      }
    }

    if (backup_active) setup_dialog_backup_save_drawn_row(row);
    setup_draw_dialogs_row(row);
    setup_write_row(row);
  }

  if (backup_active) setup_dialog_backup_valid = 1;
}

void setup_panel_fix_cursor(SetupPanel *p)
{
  if (p->count == 0)
  {
    p->cursor = 0;
    p->scroll = 0;
    return;
  }

  if (p->scroll >= p->count) p->scroll = p->count - 1;
  if ((p->scroll + p->cursor) >= p->count) p->cursor = p->count - 1 - p->scroll;
}

void setup_panel_move_up(SetupPanel *p)
{
  if (p->cursor > 0)
  {
    p->cursor--;
    return;
  }

  if (p->scroll > 0) p->scroll--;
}

void setup_panel_move_down(SetupPanel *p)
{
  if ((p->scroll + p->cursor + 1) >= p->count) return;

  if (p->cursor < (SETUP_PANEL_VISIBLE_ROWS - 1))
  {
    p->cursor++;
    return;
  }

  p->scroll++;
}

void setup_panel_page_up(SetupPanel *p)
{
  u8 i;

  for (i = 0; i < SETUP_PANEL_VISIBLE_ROWS; i++)
    setup_panel_move_up(p);
}

void setup_panel_page_down(SetupPanel *p)
{
  u8 i;

  for (i = 0; i < SETUP_PANEL_VISIBLE_ROWS; i++)
    setup_panel_move_down(p);
}

void setup_panel_home(SetupPanel *p)
{
  p->cursor = 0;
  p->scroll = 0;
}

void setup_panel_end(SetupPanel *p)
{
  if (p->count == 0) return;

  if (p->count <= SETUP_PANEL_VISIBLE_ROWS)
  {
    p->cursor = p->count - 1;
    p->scroll = 0;
    return;
  }

  p->cursor = SETUP_PANEL_VISIBLE_ROWS - 1;
  p->scroll = p->count - SETUP_PANEL_VISIBLE_ROWS;
}


void setup_exit_to_runtime()
{
  flags_register |= FLAG_HARD_RESET;
}

void setup_handle_key(u8 ext, u8 code)
{
  SetupPanel *p = &setup_panels[setup_active_panel];

  if (setup_copy_visible)
  {
    if ((ext == 0) && ((code == SETUP_KEY_ESC) || (code == SETUP_KEY_ENTER)))
    {
      setup_copy_hide_window();
      setup_dialog_backup_restore_or_redraw(SETUP_COPY_X, SETUP_COPY_Y, SETUP_COPY_W, SETUP_COPY_H);
    }
    return;
  }

  if (setup_mkdir_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_mkdir_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      setup_mkdir_create();
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_BACKSPACE))
    {
      setup_mkdir_backspace();
      return;
    }

    if (ext == 0)
    {
      setup_mkdir_add_char(setup_scancode_to_char(code));
      return;
    }

    return;
  }

  if (setup_copy_confirm_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_copy_confirm_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      setup_copy_confirm_visible = 0;
      setup_copy_active_file();
      return;
    }

    return;
  }

  if (setup_delete_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_delete_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_DELETE_X, SETUP_DELETE_Y, SETUP_DELETE_W, SETUP_DELETE_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      setup_delete_active_file();
      return;
    }

    return;
  }

  if (setup_rom_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      if (setup_rom_running)
      {
        setup_rom_cancel = 1;
        return;
      }

      setup_rom_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_ROM_X, SETUP_ROM_Y, SETUP_ROM_W, SETUP_ROM_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      setup_rom_program_active_file();
      return;
    }

    if ((setup_rom_running == 0) && ext && (code == SETUP_KEY_UP))
    {
      if (setup_rom_start_block > 0) setup_rom_start_block--;
      setup_draw_rom_window();
      return;
    }

    if ((setup_rom_running == 0) && ext && (code == SETUP_KEY_DOWN))
    {
      if (setup_rom_size < SETUP_ROM_SIZE)
      {
        if (setup_rom_start_block < (u8)((SETUP_ROM_SIZE - setup_rom_size) / SETUP_ROM_BLOCK_SIZE))
          setup_rom_start_block++;
      }
      setup_draw_rom_window();
      return;
    }

    return;
  }

  if (setup_format_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      if (setup_format_running)
      {
        setup_format_cancel = 1;
        return;
      }

      setup_format_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H);
      return;
    }

    if ((setup_format_running == 0) && (ext == 0) && (code == SETUP_KEY_SPACE))
    {
      setup_format_check ^= 1;
      setup_draw_format_window();
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      if (setup_flash_detected != 0) setup_start_format();
      return;
    }

    if (ext && (code == SETUP_KEY_UP))
    {
      if (setup_format_option > 0) setup_format_option--;
      setup_draw_format_window();
      return;
    }

    if (ext && (code == SETUP_KEY_DOWN))
    {
      if (setup_format_option < SETUP_FORMAT_SLOW) setup_format_option++;
      setup_draw_format_window();
      return;
    }

    return;
  }

  if (setup_chkdsk_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      if (setup_chkdsk_running)
      {
        setup_chkdsk_cancel = 1;
        return;
      }

      setup_chkdsk_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_FORMAT_X, SETUP_FORMAT_Y, SETUP_FORMAT_W, SETUP_FORMAT_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      if (setup_flash_detected != 0) setup_start_chkdsk();
      return;
    }

    if (ext && (code == SETUP_KEY_UP))
    {
      if (setup_chkdsk_option > 0) setup_chkdsk_option--;
      setup_draw_chkdsk_window();
      return;
    }

    if (ext && (code == SETUP_KEY_DOWN))
    {
      if (setup_chkdsk_option < SETUP_CHK_RETEST) setup_chkdsk_option++;
      setup_draw_chkdsk_window();
      return;
    }

    return;
  }

  if (setup_eeprom_visible)
  {
    if ((ext == 0) && ((code == SETUP_KEY_ESC) || (code == SETUP_KEY_ENTER) || (code == SETUP_KEY_F2)))
    {
      setup_eeprom_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_EEPROM_X, SETUP_EEPROM_Y, SETUP_EEPROM_W, SETUP_EEPROM_H);
    }

    return;
  }

  if (setup_fpga_visible)
  {
    if ((ext == 0) && (code == SETUP_KEY_ESC))
    {
      setup_fpga_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_FPGA_X, SETUP_FPGA_Y, SETUP_FPGA_W, SETUP_FPGA_H);
      return;
    }

    if ((ext == 0) && (code == SETUP_KEY_ENTER))
    {
      setup_fpga_action();
      return;
    }

    if (ext && ((code == SETUP_KEY_UP) || (code == SETUP_KEY_DOWN)))
    {
      setup_fpga_option ^= 1;
      setup_fpga_error = 0;
      setup_draw_fpga_window();
      return;
    }

    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F1))
  {
    if (setup_help_visible)
    {
      setup_help_visible = 0;
      setup_dialog_backup_restore_or_redraw(SETUP_HELP_X, SETUP_HELP_Y, SETUP_HELP_W, SETUP_HELP_H);
      return;
    }

    if (setup_any_dialog_visible())
    {
      setup_help_visible = 1;
      setup_eeprom_visible = 0;
      setup_draw_panels();
      return;
    }

    setup_help_visible = 1;
    setup_eeprom_visible = 0;
    setup_draw_help_window();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F2))
  {
    setup_eeprom_show_dialog();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F12))
  {
    setup_exit_to_runtime();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F10))
  {
    u8 need_redraw;

    if (setup_key_alt) return;
    if (setup_flash_detected == 0) return;

    need_redraw = setup_any_dialog_visible();
    setup_help_visible = 0;
    setup_chkdsk_visible = 0;
    setup_mkdir_visible = 0;
    setup_format_visible = 1;
    setup_format_option = SETUP_FORMAT_FAST;
    setup_format_check = 0;
    setup_format_check_active = 0;
    setup_format_progress = 0;
    setup_format_running = 0;
    setup_format_cancel = 0;
    setup_format_stage = SETUP_FORMAT_STAGE_IDLE;
    setup_format_free_blocks = 0;
    setup_format_invalid_blocks = 0;
    setup_tsf_error = 0;
    if (need_redraw) setup_draw_panels();
    else setup_draw_format_window();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F9))
  {
    u8 need_redraw;

    if (setup_flash_detected == 0) return;

    need_redraw = setup_any_dialog_visible();
    setup_help_visible = 0;
    setup_format_visible = 0;
    setup_mkdir_visible = 0;
    setup_chkdsk_visible = 1;
    setup_chkdsk_option = SETUP_CHK_SCAN;
    setup_chkdsk_progress = 0;
    setup_chkdsk_stage = SETUP_CHK_STAGE_IDLE;
    setup_chkdsk_running = 0;
    setup_chkdsk_cancel = 0;
    setup_chkdsk_has_stats = 0;
    setup_tsf_error = 0;
    setup_chkdsk_reset_stats();
    if (need_redraw) setup_draw_panels();
    else setup_draw_chkdsk_window();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_ESC) && setup_help_visible)
  {
    setup_help_visible = 0;
    setup_dialog_backup_restore_or_redraw(SETUP_HELP_X, SETUP_HELP_Y, SETUP_HELP_W, SETUP_HELP_H);
    return;
  }

  if (setup_help_visible) return;

  if ((ext == 0) && (code == SETUP_KEY_F5))
  {
    setup_copy_show_confirm();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F6))
  {
    setup_rom_show_dialog();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F7))
  {
    setup_mkdir_show_dialog();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_F8))
  {
    setup_delete_show_confirm();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_ENTER))
  {
    if (setup_active_panel == 0) setup_sd_open_active();
    else setup_tsf_enter_active();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_BACKSPACE))
  {
    if (setup_active_panel == 0) setup_sd_go_parent();
    return;
  }

  if ((ext == 0) && (code == SETUP_KEY_TAB))
  {
    u8 old_panel = setup_active_panel;

    setup_active_panel ^= 1;
    setup_draw_panel_active_switch(old_panel, setup_active_panel);
    return;
  }

  if (ext == 0) return;

  {
    u8 old_cursor = p->cursor;
    u8 old_scroll = p->scroll;

    if (code == SETUP_KEY_LEFT)
      setup_panel_page_up(p);
    else if (code == SETUP_KEY_RIGHT)
      setup_panel_page_down(p);
    else if (code == SETUP_KEY_UP)
      setup_panel_move_up(p);
    else if (code == SETUP_KEY_DOWN)
      setup_panel_move_down(p);
    else if (code == SETUP_KEY_PGUP)
      setup_panel_page_up(p);
    else if (code == SETUP_KEY_PGDN)
      setup_panel_page_down(p);
    else if (code == SETUP_KEY_HOME)
      setup_panel_home(p);
    else if (code == SETUP_KEY_END)
      setup_panel_end(p);
    else
      return;

    if ((old_cursor == p->cursor) && (old_scroll == p->scroll)) return;

    if (old_scroll == p->scroll)
    {
      setup_draw_panel_cursor_move(setup_active_panel, old_cursor, p->cursor);
      return;
    }
  }

  setup_draw_panels();
}

void setup_process_scancode(u8 code)
{
  if (code == 0) return;
  if (code == 0xFF) return;

  if (code == SETUP_KEY_EXT)
  {
    setup_key_ext = 1;
    return;
  }

  if (code == SETUP_KEY_RELEASE)
  {
    setup_key_release = 1;
    return;
  }

  if (setup_key_release)
  {
    if (code == SETUP_KEY_ALT) setup_key_alt = 0;
    if ((code == SETUP_KEY_LSHIFT) || (code == SETUP_KEY_RSHIFT)) setup_key_shift = 0;
    setup_key_release = 0;
    setup_key_ext = 0;
    return;
  }

  if (code == SETUP_KEY_ALT)
  {
    setup_key_alt = 1;
    setup_key_ext = 0;
    return;
  }

  if ((code == SETUP_KEY_LSHIFT) || (code == SETUP_KEY_RSHIFT))
  {
    setup_key_shift = 1;
    setup_key_ext = 0;
    return;
  }

  setup_handle_key(setup_key_ext, code);
  setup_key_ext = 0;
}

void setup_init()
{
  setup_active_panel = 0;
  setup_key_ext = 0;
  setup_key_release = 0;
  setup_key_alt = 0;
  setup_key_shift = 0;
  setup_help_visible = 0;
  setup_eeprom_visible = 0;
  setup_format_visible = 0;
  setup_format_option = SETUP_FORMAT_FAST;
  setup_format_check = 0;
  setup_format_check_active = 0;
  setup_format_progress = 0;
  setup_format_running = 0;
  setup_format_cancel = 0;
  setup_format_stage = SETUP_FORMAT_STAGE_IDLE;
  setup_format_free_blocks = 0;
  setup_format_invalid_blocks = 0;
  setup_chkdsk_visible = 0;
  setup_fpga_visible = 0;
  setup_fpga_option = SETUP_FPGA_SET_CURRENT;
  setup_fpga_type = SETUP_FPGA_TYPE_NONE;
  setup_fpga_size = 0;
  setup_fpga_error = 0;
  setup_fpga_halt_enabled = 1;
  setup_copy_visible = 0;
  setup_copy_progress = 0;
  setup_copy_error = SETUP_COPY_ERROR_NONE;
  setup_copy_confirm_visible = 0;
  setup_rom_visible = 0;
  setup_rom_running = 0;
  setup_rom_progress = 0;
  setup_rom_error = SETUP_ROM_ERROR_NONE;
  setup_rom_stage = SETUP_ROM_STAGE_CONFIRM;
  setup_rom_cancel = 0;
  setup_rom_panel = 0;
  setup_rom_start_block = 0;
  setup_rom_size = 0;
  setup_mkdir_visible = 0;
  setup_mkdir_error = SETUP_MKDIR_ERROR_NONE;
  setup_delete_visible = 0;
  setup_delete_running = 0;
  setup_delete_progress = 0;
  setup_dialog_backup_valid = 0;
  setup_dialog_backup_x = 0;
  setup_dialog_backup_y = 0;
  setup_dialog_backup_w = 0;
  setup_dialog_backup_h = 0;
  setup_chkdsk_option = SETUP_CHK_SCAN;
  setup_chkdsk_progress = 0;
  setup_chkdsk_stage = SETUP_CHK_STAGE_IDLE;
  setup_chkdsk_running = 0;
  setup_chkdsk_cancel = 0;
  setup_chkdsk_has_stats = 0;
  setup_chkdsk_reset_stats();
  setup_workspace_init();
  setup_stack_debug_init();

  setup_sd_mount();
  setup_flash_detect();
  setup_tsf_mount_volume();
  setup_tsf_rebuild_dir_cache();

  setup_panels[0].cursor = 0;
  setup_panels[0].scroll = 0;
  setup_panels[0].count = setup_panel_count(0);

  setup_panels[1].cursor = 0;
  setup_panels[1].scroll = 0;
  setup_panels[1].count = setup_panel_count(1);

  setup_panel_fix_cursor(&setup_panels[0]);
  setup_panel_fix_cursor(&setup_panels[1]);
  setup_draw_panels();
}

void setup_task()
{
  u8 code;
  u8 guard = 16;

  while (guard > 0)
  {
    code = ps2keyboard_from_log();
    if (code == 0) return;
    setup_process_scancode(code);
    guard--;
  }
}
#endif
