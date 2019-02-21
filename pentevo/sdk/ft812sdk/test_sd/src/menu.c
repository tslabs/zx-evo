
// menus
enum
{
  M_NONE,
  M_MAIN,
  M_INFO,
  M_CID,
  M_CSD,
  M_OCR,
  M_SCR,
  M_LOCK,
  M_LOCK_SET_PWD,
  M_LOCK_CLR_PWD,
  M_LOCK_ERASE
};

u8 submenu_init()
{
  u8 rc = disk_initialize();
  printf(C_NORM "Card init: " C_DATA "%s\n", sd_init_txt[rc]);
  printf(C_NORM "Card type: " C_DATA "%s\n", sd_type_txt[ctype & 7]);
  
  if (rc == STA_INIT)
  {
    read_status();
    u8 st = sd_rbuf[0];
    printf(C_NORM "STATUS:    " C_DATA "%slocked (%02X)\n\n\n", (st & 1) ? "" : "un", st);
  }
  
  return rc;
}

void menu_main()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "SD Card Utility");
  x0(5); y(3);
  printf(C_BUTN "1. " C_MENU "Card info\n\n");
  printf(C_BUTN "2. " C_MENU "CID\n\n");
  printf(C_BUTN "3. " C_MENU "CSD\n\n");
  printf(C_BUTN "4. " C_MENU "OCR\n\n");
  printf(C_BUTN "5. " C_MENU "SCR\n\n");
  printf(C_BUTN "6. " C_ACHT "Lock/Erase\n\n");
}

void menu_info()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "Card Info");
  xy(0, 2);
  submenu_init();
}

void menu_cid()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "CID");

  read_cid();
  CID cid;
  parse_cid(&cid);
  printf(C_DATA "\n\n");
  hexstr(sdbuf, 16);
  printf("\n\n");

  printf("MID: %02X\n", cid.mid);
  printf("OID: %02X\n", cid.oid[0]);
  printf("PNM: %02X\n", cid.pnm[0]);
  printf("PRV: %02X\n", cid.prv);
  printf("PSN: %02X\n", cid.psn[0]);
  printf("MDT: %04X\n", cid.mdt);
}

void menu_csd()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "CSD");

  read_csd();
  printf(C_DATA "\n\n");
  hexstr(sdbuf, 16);
}

void menu_ocr()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "OCR");

  read_ocr();
  printf(C_DATA "\n\n");
  hexstr(sd_rbuf, 4);
}

void menu_scr()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "SCR");

  read_scr();
  printf(C_DATA "\n\n");
  hexstr(sdbuf, 8);
}

void menu_lock()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "Card lock/erase");
  xy(0, 2);
  u8 rc = submenu_init();

  if (rc == STA_INIT)
  {
    printf(C_BUTN "1. " C_MENU "Set password\n\n");
    printf(C_BUTN "2. " C_MENU "Clear password\n\n");
    printf(C_BUTN "3. " C_ACHT "Erase card\n\n");
    printf(C_BUTN "4. " C_MENU "Re-detect card\n\n\n\n");
  }
  else
  {
    menu = M_NONE;
    return;
  }

  sd_cs_on();
  sd_cs_off();
}

void menu_lock_set_pwd()
{
  printf(C_PROC "Setting password... ");
  set_password_sd();
  printf(C_OK "OK\n\n");
}

void menu_lock_clr_pwd()
{
  printf(C_PROC "Removing password... ");
  u8 rc = clear_password_sd();
  printf(C_OK "OK\n\n");
}

void menu_lock_erase()
{
  read_status();
  if (!(sd_rbuf[0] & 1))
  {
    printf(C_ERR "Error: " C_INFO "card must be locked\n");
    printf(C_INFO "Set password, then remove card from the slot and insert it again");
  }
  else
  {
    printf(C_PROC "Erasing card... ");
    u8 rc = erase_sd();
    printf(C_OK "OK\n\n");
  }
}

void menu_disp()
{
  switch (menu)
  {
    case M_MAIN:          menu_main(); break;
    case M_INFO:          menu_info(); break;
    case M_CID:           menu_cid(); break;
    case M_CSD:           menu_csd(); break;
    case M_OCR:           menu_ocr(); break;
    case M_SCR:           menu_scr(); break;
    case M_LOCK:          menu_lock(); break;
    case M_LOCK_SET_PWD:  menu_lock_set_pwd(); break;
    case M_LOCK_CLR_PWD:  menu_lock_clr_pwd(); break;
    case M_LOCK_ERASE:    menu_lock_erase(); break;
  }
}

bool key_disp()
{
  bool rc = false;
  u8 key = getkey();

  if (req_unpress)
  {
    if (key == KEY_NONE)
      req_unpress = false;
  }
  else switch (menu)
  {
    case M_MAIN:
      switch(key)
      {
        case KEY_1: menu = M_INFO; rc = true; break;
        case KEY_2: menu = M_CID;  rc = true; break;
        case KEY_3: menu = M_CSD;  rc = true; break;
        case KEY_4: menu = M_OCR;  rc = true; break;
        case KEY_5: menu = M_SCR;  rc = true; break;
        case KEY_6: menu = M_LOCK; rc = true; break;
      }
    break;

    case M_LOCK_SET_PWD:
    case M_LOCK_CLR_PWD:
    case M_LOCK_ERASE:
      if (key != KEY_NONE)
        { menu = M_LOCK; rc = true; break; }
    break;
    
    case M_LOCK:
      switch(key)
      {
        case KEY_1: menu = M_LOCK_SET_PWD; rc = true; break;
        case KEY_2: menu = M_LOCK_CLR_PWD; rc = true; break;
        case KEY_3: menu = M_LOCK_ERASE; rc = true; break;
        case KEY_4: menu = M_LOCK; rc = true; break;
        
        default:
          if (key != KEY_NONE)
            { menu = M_MAIN; rc = true; break; }
      }
    break;
    
    default:
      if (key != KEY_NONE)
        { menu = M_MAIN; rc = true; break; }
  }

  return rc;
}
