;NOTE! Sure no warning like:
; "warning: A .db segment with an odd number of bytes is detected. A zero byte is added."
;
;------------------------------------------------------------------------------
;
.EQU    MAX_LANG=2
;
;------------------------------------------------------------------------------
;
MLMSG_TITLE1:
        .DW     MSG_TITLE1_RUS*2, MSG_TITLE1_ENG*2
MSG_TITLE1_RUS:
        .DB     "      Сервисная прошивка ZX Evolution "               ,0,0
MSG_TITLE1_ENG:
        .DB     "            ZX Evolution Service "                    ,0
;width limited! "01234567890123456789012345678901234567890123456789012"
;
;------------------------------------------------------------------------------
;
MSG_TITLE2:
        .DB     $16,15,24,"http://www.NedoPC.com/",0
;
;------------------------------------------------------------------------------
;
MLMSG_PINTEST:
        .DW     MSG_PINTEST_RUS*2, MSG_PINTEST_ENG*2
MSG_PINTEST_RUS:
        .DB     $0D,$0A,$0A,"Проверка выводов ATMEGA128... ",0
MSG_PINTEST_ENG:
        .DB     $0D,$0A,$0A,"ATMEGA128 pins check... ",0
;
;------------------------------------------------------------------------------
;
MLMSG_PINTEST_OK:
        .DW     MSG_PINTEST_OK_RUS*2, MSG_PINTEST_OK_ENG*2
MSG_PINTEST_OK_RUS:
        .DB     "Проблем не обнаружено.",0,0
MSG_PINTEST_OK_ENG:
        .DB     "No problems found.",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_PINTEST_ERROR:
        .DW     MSG_PINTEST_ERROR_RUS*2, MSG_PINTEST_ERROR_ENG*2
MSG_PINTEST_ERROR_RUS:
        .DB     $0D,$0A,"Обнаружена проблема на порту(-ах): ",0
MSG_PINTEST_ERROR_ENG:
        .DB     $0D,$0A,"Have a problem at port(s): ",0
;
;------------------------------------------------------------------------------
;
MSG_PINTEST_PA:
        .DB     "PAx ",0,0
MSG_PINTEST_PB:
        .DB     "PBx ",0,0
MSG_PINTEST_PC:
        .DB     "PCx ",0,0
MSG_PINTEST_PD:
        .DB     "PD5 ",0,0
MSG_PINTEST_PE:
        .DB     "PEx ",0,0
MSG_PINTEST_PF:
        .DB     "PFx ",0,0
MSG_PINTEST_PG:
        .DB     "PGx ",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_HALT:
        .DW     MSG_HALT_RUS*2, MSG_HALT_ENG*2
MSG_HALT_RUS:
        .DB     $0D,$0A,"Программа остановлена!",0,0
MSG_HALT_ENG:
        .DB     $0D,$0A,"Program is halted!",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_STATUSOF_CRLF:
        .DW     MSG_STATUSOF_RUS*2,MSG_STATUSOF_ENG*2
MLMSG_STATUSOF_CR:
        .DW     (MSG_STATUSOF_RUS*2)+1,(MSG_STATUSOF_ENG*2)+1
MSG_STATUSOF_RUS:
        .DB     $0A,$0D,"Состояние ",0,0
MSG_STATUSOF_ENG:
        .DB     $0A,$0D,"Status of ",0,0
MSG_POWER_PG:
        .DB     "POWERGOOD=",0,0
MSG_POWER_VCC5:
        .DB     ", VCC5=",0
;
;------------------------------------------------------------------------------
;
MLMSG_POWER_ON:
        .DW     MSG_POWER_ON_RUS*2, MSG_POWER_ON_ENG*2
MSG_POWER_ON_RUS:
        .DB     $0D,$0A,"Включение питания ATX...",$0A,0
MSG_POWER_ON_ENG:
        .DB     $0D,$0A,"ATX power up...",$0A,0,0
;
;------------------------------------------------------------------------------
;
MLMSG_CFGFPGA:
        .DW     MSG_CFGFPGA_RUS*2, MSG_CFGFPGA_ENG*2
MSG_CFGFPGA_RUS:
        .DB     $0D,$0A,"Загрузка конфигурации в FPGA... ",0,0
MSG_CFGFPGA_ENG:
        .DB     $0D,$0A,"Set FPGA configuration... ",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_DONE:
        .DW     MSG_DONE_RUS*2, MSG_DONE_ENG*2
MSG_DONE_RUS:
        .DB     "Завершено.",0,0
MSG_DONE_ENG:
        .DB     "Done.",0
;
;------------------------------------------------------------------------------
;
MLMSG_KBD_DETECT:
        .DW     MSG_KBD_DETECT_RUS*2, MSG_KBD_DETECT_ENG*2
MSG_KBD_DETECT_RUS:
        .DB     $0D,$0A,"Проверка клавиатуры PS/2...",$0D,$0A,0
MSG_KBD_DETECT_ENG:
        .DB     $0D,$0A,"PS/2 keyboard check...",$0D,$0A,0,0
;
;------------------------------------------------------------------------------
;
MLMSG_NORESPONSE:
        .DW     MSG_NORESPONSE_RUS*2, MSG_NORESPONSE_ENG*2
MSG_NORESPONSE_RUS:
        .DB     " ...нет ответа",$0D,$0A,0,0
MSG_NORESPONSE_ENG:
        .DB     " ...no response",$0D,$0A,0
;
;------------------------------------------------------------------------------
;
MLMSG_UNWANTED:
        .DW     MSG_UNWANTED_RUS*2, MSG_UNWANTED_ENG*2
MSG_UNWANTED_RUS:
        .DB     " ...неожидаемый ответ",$0D,$0A,0
MSG_UNWANTED_ENG:
        .DB     " ...unwanted response",$0D,$0A,0
;
;------------------------------------------------------------------------------
;
MLMSG_TXFAIL:
        .DW     MSG_TXFAIL_RUS*2, MSG_TXFAIL_ENG*2
MSG_TXFAIL_RUS:
        .DB     " ...сбой при передаче",$0D,$0A,0
MSG_TXFAIL_ENG:
        .DB     " ...fail to transmit",$0D,$0A,0,0
;
;------------------------------------------------------------------------------
;
MENU_MAIN:
        .DB     6,3,26+2,6,$9F,$F0
        .DW     MTST_SHOW_REPORT,1000
        ;handlers
        .DW     TESTPS2KEYB
        .DW     TESTZXKEYB
        .DW     TESTMOUSE
        .DW     TESTBEEP
        .DW     TESTVIDEO
        .DW     FLASHER
        ;lang0
        .DB     "──────────────────────────"
        .DB     "Тест клавиатуры PS/2      "
        .DB     "Тест клавиатуры ZX и др.  "
        .DB     "Тест мыши                 "
        .DB     "Тест BEEP/TAPEOUT/COVOX   "
        .DB     "Тест видео                "
        .DB     "Программирование Flash-ROM"
        ;lang1
        .DB     "──────────────────────────"
        .DB     "PS/2 keyboard test        "
        .DB     "ZX keyboard test and etc  "
        .DB     "Mouse test                "
        .DB     "BEEP/TAPEOUT/COVOX test   "
        .DB     "Video test                "
        .DB     "Write Flash-ROM           "
;width fixed!   "12345678901234567890123456"
;
;------------------------------------------------------------------------------
;
MLMSG_MENU_HELP:
        .DW     MSG_MENU_HELP_RUS*2, MSG_MENU_HELP_ENG*2
MSG_MENU_HELP_RUS:
        .DB     $16,5,14,"Основные клавиши управления:"     ,$16,5,15,"<>, <>"
        .DB     $16,5,16,"<Enter> - ",$22,"Да",$22,$2C," выбор"
        .DB     $16,5,17,"<Esc> - ",$22,"Нет",$22,$2C," отмена, выход "
        .DB     $16,5,18,"Горячие клавиши (только в меню): "
        .DB     $16,5,19,"<ScrollLock> - режим TV/VGA"
        .DB     $16,5,20,"<CapsLock> - язык интерфейса"     ,0
MSG_MENU_HELP_ENG:
        .DB     $16,5,14,"Usage:"                           ,$16,5,15,"<>, <>"
        .DB     $16,5,16,"<Enter> - ",$22,"Yes",$22,$2C," select"
        .DB     $16,5,17,"<Esc> - ",$22,"No",$22,$2C," cancel, exit "
        .DB     $16,5,18,"Hot-keys (in menu only): "
        .DB     $16,5,19,"<ScrollLock> - toggle TV/VGA mode"
        .DB     $16,5,20,"<CapsLock> - language switch"     ,0
;width limited!          "567890123456789012345678901234567"
;
;------------------------------------------------------------------------------
;
MLMSG_TBEEP:
        .DW     MSG_TBEEP_RUS*2, MSG_TBEEP_ENG*2
MSG_TBEEP_RUS:
        .DB     $16,26,10,"Гц",$16,10,12,"<>, <> - изменение частоты",$15,$0F,0,0
MSG_TBEEP_ENG:
        .DB     $16,26,10,"Hz",$16,14,12,    "<>, <> - frequence"    ,$15,$0F,0,0
;width limited!                          "0123456789012345678901234567"
;
;------------------------------------------------------------------------------
;
MLMSG_TZXK_1:
        .DW     MSG_TZXK_1_RUS*2, MSG_TZXK_1_ENG*2
MSG_TZXK_1_RUS:
        .DB     $16,14, 7,"Клавиатура ZX",$16,35, 7,"Джойстик",0
MSG_TZXK_1_ENG:
        .DB     $16,15, 7,"ZX Keyboard",$16,35, 7,"Joystick",0
;
;------------------------------------------------------------------------------
;
MSG_TZXK_2:
        .DB     $16,11, 9,"1 2 3 4 5 6 7 8 9 0"
        .DB     $16,11,10,"Q W E R T Y U I O P"
        .DB     $16,11,11,"A S D F G H J K L e"
        .DB     $16,11,12,"c Z X C V B N M s s"
        .DB     $16,38,10,$18
        .DB     $16,36,11,$1B," F ",$1A
        .DB     $16,38,12,$19
        .DB     $16,14,15,"SoftReset"
        .DB     $16,30,15,"TurboKey",0
;
;------------------------------------------------------------------------------
;
MSG_TPS2K_1:
        .DB     $16,5, 7,"e   1 2 3 4 5 6 7 8 9 0 1 2  p s p  ",$07,$20,$07,$20,$07
        .DB     $16,5, 9,"` 1 2 3 4 5 6 7 8 9 0 - = ",$1B,"  i h u  n / * -"
        .DB     $16,5,10,"t Q W E R T Y U I O P [ ] \  d e d  7 8 9"
        .DB     $16,5,11,"c A S D F G H J K L ",$3B," '   e         4 5 6 +"
        .DB     $16,5,12,"s Z X C V B N M , . /     s    ",$18,"    1 2 3"
        .DB     $16,5,13,"c w a       s       a w m c  ",$1B,$20,$19,$20,$1A,"  0   . e"
        .DB     $16,5,16,"Raw data:",$16,4,15,0
;
;------------------------------------------------------------------------------
;
MLMSG_TPS2K_0:
        .DW     MSG_TPS2K_0_RUS*2, MSG_TPS2K_0_ENG*2
MSG_TPS2K_0_RUS:
        .DB     $16, 5,19,"Трёхкратное нажатие <ESC> - выход из теста.",0,0
MSG_TPS2K_0_ENG:
        .DB     $16,10,19,     "Press <ESC> three times to exit.",0
;width limited!           "5678901234567890123456789012345678901234567"
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_TEST:
        .DW     MSG_MOUSE_TEST_RUS*2, MSG_MOUSE_TEST_ENG*2
MSG_MOUSE_TEST_RUS:
        .DB     $0D,$0A,"Тестирование мыши... ",0
MSG_MOUSE_TEST_ENG:
        .DB     $0D,$0A,"Mouse test... ",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_DETECT:
        .DW     MSG_MOUSE_DETECT_RUS*2, MSG_MOUSE_DETECT_ENG*2
MSG_MOUSE_DETECT_RUS:
        .DB     "Обнаружение мыши...  ",0
MSG_MOUSE_DETECT_ENG:
        .DB     "Detecting mouse...  ",0,0
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_SETUP:
        .DW     MSG_MOUSE_SETUP_RUS*2, MSG_MOUSE_SETUP_ENG*2
MSG_MOUSE_SETUP_RUS:
        .DB     "Настройка... ",0
MSG_MOUSE_SETUP_ENG:
        .DB     "Customization... ",0
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_LETSGO:
        .DW     MSG_MOUSE_LETSGO_RUS*2, MSG_MOUSE_LETSGO_ENG*2
MSG_MOUSE_LETSGO_RUS:
        .DB     "Поехали!",0,0
MSG_MOUSE_LETSGO_ENG:
        .DB     "Let",$27,"s go!",0
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_FAIL0:
        .DW     MSG_MOUSE_FAIL0_RUS*2, MSG_MOUSE_FAIL0_ENG*2
MSG_MOUSE_FAIL0_RUS:
        .DB     "      Нет ответа от мыши."      ,0
MSG_MOUSE_FAIL0_ENG:
        .DB     "      No mouse response."       ,0,0
;width limited! "1234567890123456789012345678901"
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_FAIL1:
        .DW     MSG_MOUSE_FAIL1_RUS*2, MSG_MOUSE_FAIL1_ENG*2
MSG_MOUSE_FAIL1_RUS:
        .DB     "   Имеются некоторые проблемы." ,0,0
MSG_MOUSE_FAIL1_ENG:
        .DB     "    There are some problems."   ,0,0
;width limited! "1234567890123456789012345678901"
;
;------------------------------------------------------------------------------
;
MLMSG_MOUSE_RESTART:
        .DW     MSG_MOUSE_RESTART_RUS*2, MSG_MOUSE_RESTART_ENG*2
MSG_MOUSE_RESTART_RUS:
        .DB     "  <Enter> - перезапустить тест.",0
MSG_MOUSE_RESTART_ENG:
        .DB     "     <Enter> - restart test."   ,0,0
;width limited! "1234567890123456789012345678901"
;
;------------------------------------------------------------------------------
;
MLMSG_MTST:
        .DW     MSG_MTST_RUS*2, MSG_MTST_ENG*2
MSG_MTST_RUS:
        .DB     $16,35,18,   " Тест DRAM "
        .DB     $16,32,19,"Проведено циклов",$16,32,20,"без ошибок"
        .DB     $16,32,21,"с ошибками",0
MSG_MTST_ENG:
        .DB     $16,35,18,   " DRAM test "
;width limited!           "23456789012345678"
        .DB     $16,32,19,"Loops",  $16,32,20,"Pass",  $16,32,21,"Fail",0,0
;width limited!           "23456789012345678" "2345678901"       "2345678901"
;
;------------------------------------------------------------------------------
;
MLMSG_MENU_SWLNG:
        .DW     MSG_MENU_SWLNG_RUS*2, MSG_MENU_SWLNG_ENG*2
MSG_MENU_SWLNG_RUS:
        .DB     $16,23,12,         "Русский"        ,0,0
MSG_MENU_SWLNG_ENG:
        .DB     $16,23,12,         "English"        ,0,0
;width limited!           "456789012345678901234567"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_MENU:
        .DW     MSG_FL_MENU_RUS*2, MSG_FL_MENU_ENG*2
MSG_FL_MENU_RUS:
        .DB     $16,2,2,"Выход"
        .DB     $16,2,3,"Всё снова"
        .DB     $16,2,4,"Стереть м/сх."
        .DB     $16,2,5,"Добав.задание"
        .DB     $16,2,6,"Выполнить "   ,0
MSG_FL_MENU_ENG:
        .DB     $16,2,2,"Exit "
        .DB     $16,2,3,"Retrieve all "
        .DB     $16,2,4,"Erase chip "
        .DB     $16,2,5,"Add job"
        .DB     $16,2,6,"Execute jobs" ,0
;width limited!         "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FP_NOFILES:
        .DW     MSG_FP_NOFILES_RUS*2,MSG_FP_NOFILES_ENG*2
MSG_FP_NOFILES_RUS:
        .DB     $15,$9F," Нет файлов ",0,0
MSG_FP_NOFILES_ENG:
        .DB     $15,$9F,"  No files  ",0,0
;width fixed!           "123456789012"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_READROM:
        .DW     MSG_FL_READROM_RUS*2,MSG_FL_READROM_ENG*2
MSG_FL_READROM_RUS:
        .DB     $16, 2,10,$15,$9E,"Чтение Flash" ,$15,$9F,$16, 2,11,"<ESC> - выход",0
MSG_FL_READROM_ENG:
        .DB     $16, 2,10,$15,$9E,"Read Flash...",$15,$9F,$16, 2,11,"<ESC> - exit" ,0
;width limited!                   "2345678901234"                   "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_SDINIT:
        .DW     MSG_FL_SDINIT_RUS*2,MSG_FL_SDINIT_ENG*2
MSG_FL_SDINIT_RUS:
        .DB     $16, 2,11,$15,$9F,"Иниц.SD карты",0,0
MSG_FL_SDINIT_ENG:
        .DB     $16, 2,11,$15,$9F,"SDcard init. ",0,0
;width limited!                   "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_SDERROR1:
        .DW     MSG_FL_SDERROR1_RUS*2,MSG_FL_SDERROR1_ENG*2
MSG_FL_SDERROR1_RUS:
        .DB     " Нет SD карты! ",0
MSG_FL_SDERROR1_ENG:
        .DB     "  No SD-card!  ",0
;width fixed!   "123456789012345"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_SDERROR2:
        .DW     MSG_FL_SDERROR2_RUS*2,MSG_FL_SDERROR2_ENG*2
MSG_FL_SDERROR2_RUS:
        .DB     " Ошибка чт. SD ",0
MSG_FL_SDERROR2_ENG:
        .DB     " SD read error ",0
;width fixed!   "123456789012345"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_SDERROR3:
        .DW     MSG_FL_SDERROR3_RUS*2,MSG_FL_SDERROR3_ENG*2
MSG_FL_SDERROR3_RUS:
        .DB     "   Нет FAT !   ",0
MSG_FL_SDERROR3_ENG:
        .DB     " FAT no found! ",0
;width fixed!   "123456789012345"
;
;------------------------------------------------------------------------------
;это сообщение никогда ;) не должно появляться
MLMSG_FL_SDERRORX:
        .DW     MSG_FL_SDERRORX_RUS*2,MSG_FL_SDERRORX_ENG*2
MSG_FL_SDERRORX_RUS:
        .DB     " О, глюкануло! ",0
MSG_FL_SDERRORX_ENG:
        .DB     " Great glitch! ",0
;width fixed!   "123456789012345"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_SURE:
        .DW     MSG_FL_SURE_RUS*2,MSG_FL_SURE_ENG*2
MSG_FL_SURE_RUS:
        .DB     $16, 2,12,$15,$9E," Уверен? <Y> ",0,0
MSG_FL_SURE_ENG:
        .DB     $16, 2,12,$15,$9E,"You sure? <Y>",0,0
;width fixed!                     "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_ERASE:
        .DW     MSG_FL_ERASE_RUS*2,MSG_FL_ERASE_ENG*2
MSG_FL_ERASE_RUS:
        .DB     $16, 2,12,$15,$9E,"Стирание...  ",0,0
MSG_FL_ERASE_ENG:
        .DB     $16, 2,12,$15,$9E,"Erase...     ",0,0
;width fixed!                     "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_WRITE:
        .DW     MSG_FL_WRITE_RUS*2,MSG_FL_WRITE_ENG*2
MSG_FL_WRITE_RUS:
        .DB     $16, 2,12,$15,$9E,"Запись...    ",0,0
MSG_FL_WRITE_ENG:
        .DB     $16, 2,12,$15,$9E,"Write...     ",0,0
;width fixed!                     "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_VERIFY:
        .DW     MSG_FL_VERIFY_RUS*2,MSG_FL_VERIFY_ENG*2
MSG_FL_VERIFY_RUS:
        .DB     $16, 2,12,$15,$9E,"Проверка...  ",0,0
MSG_FL_VERIFY_ENG:
        .DB     $16, 2,12,$15,$9E,"Verify...    ",0,0
;width fixed!                     "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FL_COMPLETE:
        .DW     MSG_FL_COMPLETE_RUS*2,MSG_FL_COMPLETE_ENG*2
MSG_FL_COMPLETE_RUS:
        .DB     $16, 2,12,$15,$9E,"Завершено.   ",0,0
MSG_FL_COMPLETE_ENG:
        .DB     $16, 2,12,$15,$9E,"Complete.    ",0,0
;width limited!                   "2345678901234"
;
;------------------------------------------------------------------------------
;
MLMSG_FLRES0:
        .DW     MSG_FLRES0_RUS*2,MSG_FLRES0_ENG*2
MSG_FLRES0_RUS:
        .DB     $16,13, 6, "Запись в FlashROM завершена" ,0,0
MSG_FLRES0_ENG:
        .DB     $16,18, 6,      "Job(s) completed."      ,0,0
;width limited!           "23456789012345678901234567890"
;
;------------------------------------------------------------------------------
;
MLMSG_FLRES1:
        .DW     MSG_FLRES1_RUS*2,MSG_FLRES1_ENG*2
MSG_FLRES1_RUS:
        .DB     $16,21, 7,         "без ошибок."         ,0,0
MSG_FLRES1_ENG:
        .DB     $16,17, 7,     "No errors detected."     ,0,0
;width limited!           "23456789012345678901234567890"
;
;------------------------------------------------------------------------------
;
MLMSG_FLRES2:
        .DW     MSG_FLRES2_RUS*2,MSG_FLRES2_ENG*2
MSG_FLRES2_RUS:
        .DB     $16,21, 7,         "с ошибками!"         ,0,0
MSG_FLRES2_ENG:
        .DB     $16,14, 7,  "Some errors are detected!"  ,0,0
;width limited!           "23456789012345678901234567890"
;
;------------------------------------------------------------------------------
;
