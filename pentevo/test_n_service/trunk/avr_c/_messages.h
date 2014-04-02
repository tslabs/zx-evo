#ifndef _MESSAGES_H
#define _MESSAGES_H 1

#define TOTAL_LANG 2

#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
.extern msg_tsd_out
.extern msg_tsd_in
.extern msg_tsd_skip
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

#include "_types.h"

extern const u8 msg_title1[] PROGMEM;
extern const u8 msg_title2[] PROGMEM;
extern const u8 * const mlmsg_pintest[] PROGMEM;
extern const u8 * const mlmsg_pintest_ok[] PROGMEM;
extern const u8 * const mlmsg_pintest_error[] PROGMEM;
extern const u8 msg_pintest_pa[] PROGMEM;
extern const u8 msg_pintest_pb[] PROGMEM;
extern const u8 msg_pintest_pc[] PROGMEM;
extern const u8 msg_pintest_pd[] PROGMEM;
extern const u8 msg_pintest_pe[] PROGMEM;
extern const u8 msg_pintest_pf[] PROGMEM;
extern const u8 msg_pintest_pg[] PROGMEM;
extern const u8 * const mlmsg_halt[] PROGMEM;
extern const u8 * const mlmsg_statusof_crlf[] PROGMEM;
extern const u8 * const mlmsg_statusof_cr[] PROGMEM;
extern const u8 msg_power_pg[] PROGMEM;
extern const u8 msg_power_vcc5[] PROGMEM;
extern const u8 * const mlmsg_power_on[] PROGMEM;
extern const u8 * const mlmsg_cfgfpga[] PROGMEM;
extern const u8 * const mlmsg_done1[] PROGMEM;
extern const u8 msg_ok[] PROGMEM;
extern const u8 * const mlmsg_someerrors[] PROGMEM;
extern const u8 * const mlmsg_spi_test[] PROGMEM;
extern const u8 * const mlmsg_kbd_detect[] PROGMEM;
extern const u8 * const mlmsg_noresponse[] PROGMEM;
extern const u8 * const mlmsg_unwanted[] PROGMEM;
extern const u8 * const mlmsg_txfail[] PROGMEM;
extern const u8 msg_ready[] PROGMEM;
extern const u8 * const mlmsg_menu_help[] PROGMEM;
extern const u8 * const mlmsg_tbeep[] PROGMEM;
extern const u8 * const mlmsg_tzxk1[] PROGMEM;
extern const u8 msg_tzxk2[] PROGMEM;
extern const u8 * const mlmsg_tps2k0[] PROGMEM;
extern const u8 msg_tps2k1[] PROGMEM;
extern const u8 * const mlmsg_mouse_test[] PROGMEM;
extern const u8 * const mlmsg_mouse_detect[] PROGMEM;
extern const u8 * const mlmsg_mouse_setup[] PROGMEM;
extern const u8 * const mlmsg_mouse_letsgo[] PROGMEM;
extern const u8 * const mlmsg_mouse_fail0[] PROGMEM;
extern const u8 * const mlmsg_mouse_fail1[] PROGMEM;
extern const u8 * const mlmsg_mouse_restart[] PROGMEM;
extern const u8 msg_tpsm_1[] PROGMEM;
extern const u8 * const mlmsg_mtst[] PROGMEM;
extern const u8 * const mlmsg_swlng[] PROGMEM;
extern const u8 * const mlmsg_fl_menu[] PROGMEM;
extern const u8 * const mlmsg_fp_nofiles[] PROGMEM;
extern const u8 * const mlmsg_fl_readrom[] PROGMEM;
extern const u8 * const mlmsg_fl_sdinit[] PROGMEM;
extern const u8 * const mlmsg_fl_sderror1[] PROGMEM;
extern const u8 * const mlmsg_fl_sderror2[] PROGMEM;
extern const u8 * const mlmsg_fl_sderror3[] PROGMEM;
extern const u8 * const mlmsg_fl_sderror4[] PROGMEM;
extern const u8 * const mlmsg_fl_sure[] PROGMEM;
extern const u8 * const mlmsg_fl_erase[] PROGMEM;
extern const u8 * const mlmsg_fl_write[] PROGMEM;
extern const u8 * const mlmsg_fl_verify[] PROGMEM;
extern const u8 * const mlmsg_fl_complete[] PROGMEM;
extern const u8 * const mlmsg_flres0[] PROGMEM;
extern const u8 * const mlmsg_flres1[] PROGMEM;
extern const u8 * const mlmsg_flres2[] PROGMEM;
extern const u8 * const mlmsg_sensors[] PROGMEM;
extern const u8 * const mlmsg_s_nocard[] PROGMEM;
extern const u8 * const mlmsg_s_inserted[] PROGMEM;
extern const u8 * const mlmsg_s_readonly[] PROGMEM;
extern const u8 * const mlmsg_s_writeen[] PROGMEM;
extern const u8 * const mlmsg_tsd_init[] PROGMEM;
extern const u8 * const mlmsg_tsd_nocard[] PROGMEM;
extern const u8 * const mlmsg_tsd_foundcard[] PROGMEM;
extern const u8 * const mlmsg_tsd_menu[] PROGMEM;
extern const u8 * const mlmsg_tsd_foundfat[] PROGMEM;
extern const u8 * const mlmsg_tsd_detect[] PROGMEM;
extern const u8 * const mlmsg_tsd_readfile[] PROGMEM;
extern const u8 * const mlmsg_tsd_complete[] PROGMEM;
extern const u8 msg_tsd_out[] PROGMEM;
extern const u8 msg_tsd_in[] PROGMEM;
extern const u8 msg_tsd_cmd[] PROGMEM;
extern const u8 msg_tsd_acmd41[] PROGMEM;
extern const u8 msg_tsd_csup[] PROGMEM;
extern const u8 msg_tsd_csdown[] PROGMEM;
extern const u8 msg_tsd_mmc[] PROGMEM;
extern const u8 msg_tsd_sdv1[] PROGMEM;
extern const u8 msg_tsd_sdsc[] PROGMEM;
extern const u8 msg_tsd_sdhc[] PROGMEM;
extern const u8 msg_tsd_ocr[] PROGMEM;
extern const u8 msg_tsd_csd[] PROGMEM;
extern const u8 msg_tsd_cid0[] PROGMEM;
extern const u8 msg_tsd_cid1[] PROGMEM;
extern const u8 msg_tsd_cid2[] PROGMEM;
extern const u8 msg_tsd_cid3[] PROGMEM;
extern const u8 msg_tsd_cid4[] PROGMEM;
extern const u8 msg_tsd_cid5[] PROGMEM;
extern const u8 msg_tsd_cid6[] PROGMEM;
extern const u8 msg_tsd_cid6b[] PROGMEM;
extern const u8 msg_tsd_cid6c[] PROGMEM;
extern const u8 msg_tsd_crc[] PROGMEM;
extern const u8 msg_tsd_readsector[] PROGMEM;
extern const u8 msg_tsd_skip[] PROGMEM;
extern const u8 msg_trs_1[] PROGMEM;

extern const u8 str_menu_main[] PROGMEM;

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _MESSAGES_H
