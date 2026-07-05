#pragma once

#include "esp_err.h"
#include "main.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  OPL_MODE_OFF = 0,
  OPL_MODE_YMF262,
  OPL_MODE_YMF278
} OPL_MODE;

typedef enum
{
  OPL_FM_RENDER_OPL3_FULL = 0,
  OPL_FM_RENDER_OPL2_MELODY
} OPL_FM_RENDER_MODE;

#define OPL_PORT_FM_REG1   0x00C4
#define OPL_PORT_FM_DAT1   0x00C5
#define OPL_PORT_FM_REG2   0x00C6
#define OPL_PORT_FM_DAT2   0x00C7
#define OPL_PORT_WAVE_REG  0x007E
#define OPL_PORT_WAVE_DAT  0x007F

#define OPL_ROM_PART_LABEL "oplrom"
#define OPL_ROM_PART_SUBTYPE 0x85
#define OPL_ROM_SIZE 0x200000
#define OPL_YMF278_DEFAULT_RAM_SIZE 0x400000
#define OPL_FM_BANK_COUNT 2
#define OPL_FM_REG_COUNT 256
#define OPL_WAVE_REG_COUNT 256

typedef struct
{
  u16 reg;
  u8 value;
} OPL_FM_REG_WRITE;

typedef struct
{
  OPL_MODE mode;
  u8 enabled;
  size_t rom_size;
  size_t ram_size;
  u8 fm_latch_bank;
  u8 fm_latch[2];
  u8 wave_latch;
  u8 status;
  u32 queued_writes;
} OPL_INFO;

esp_err_t opl_enable(OPL_MODE mode, size_t ram_size);
void opl_disable();
void opl_reset();
int opl_is_enabled();
esp_err_t opl_get_info(OPL_INFO *info);

u8 opl_read_status();
u8 opl_read_port(u16 port);
void opl_write_port(u16 port, u8 value);
esp_err_t opl_queue_write_port(u16 port, u8 value);
esp_err_t opl_queue_write_fm_reg(u16 reg, u8 value);
esp_err_t opl_queue_write_wave_reg(u8 reg, u8 value);

u8 opl_read_fm_reg(u16 reg);
void opl_write_fm_reg(u16 reg, u8 value);
esp_err_t opl_write_fm_regs(const OPL_FM_REG_WRITE *writes, size_t count);
u8 opl_read_wave_reg(u8 reg);
void opl_write_wave_reg(u8 reg, u8 value);

esp_err_t opl_set_fm_render_mode(OPL_FM_RENDER_MODE mode);
esp_err_t opl_set_volume(u8 volume);
u8 opl_get_volume();
void opl_render(float *mix, int sample_count, int sample_rate);
void opl_console_register_system_commands();

#ifdef __cplusplus
}
#endif
