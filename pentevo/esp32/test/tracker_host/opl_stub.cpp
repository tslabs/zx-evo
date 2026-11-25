// opl.h functions for the host build (no OPL)
#include "tracker.h"
#include "opl.h"
#include <string.h>
esp_err_t opl_enable(OPL_MODE, size_t) { return 0; }
void opl_disable() {}
void opl_reset() {}
int opl_is_enabled() { return 0; }
esp_err_t opl_get_info(OPL_INFO *i) { memset(i, 0, sizeof(*i)); return 0; }
u8 opl_read_status() { return 0; }
u8 opl_read_port(u16) { return 0; }
void opl_write_port(u16, u8) {}
esp_err_t opl_queue_write_port(u16, u8) { return 0; }
esp_err_t opl_queue_write_fm_reg(u16, u8) { return 0; }
esp_err_t opl_queue_write_wave_reg(u8, u8) { return 0; }
u8 opl_read_fm_reg(u16) { return 0; }
void opl_write_fm_reg(u16, u8) {}
esp_err_t opl_write_fm_regs(const OPL_FM_REG_WRITE *, size_t) { return 0; }
u8 opl_read_wave_reg(u8) { return 0; }
void opl_write_wave_reg(u8, u8) {}
esp_err_t opl_set_fm_render_mode(OPL_FM_RENDER_MODE) { return 0; }
esp_err_t opl_set_volume(u8) { return 0; }
u8 opl_get_volume() { return 0; }
void opl_render(float *, int, int) {}
void opl_console_register_system_commands() {}
