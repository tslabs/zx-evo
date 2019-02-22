
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
    printf(C_NORM "STATUS:    " C_DATA "%slocked (%02X)\n", (st & 1) ? "" : "un", st);
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
  printf(C_BUTN "0. " C_ACHT "Lock/Erase\n\n");
}

void menu_info()
{
  cls();
  xy(3, 0);
  printf(C_HEAD "Card Info");
  xy(0, 2);
  submenu_init();

  u8 buf[16];

  // CID
  read_cid();
  bitmax = 127;

  printf(C_INFO "\n\nCID: " C_DATA);
  hexstr(sdbuf, 16);
  printf("\n\n");

  u8 mid = get_bf(127, 8);
  printf(C_INFO "Manufactorer ID:    " C_DATA "%s (%02X)\n", look_up_mid(mid), mid);
  
  get_bfa(buf, 119, 2);
  printf(C_INFO "OEM/Application ID: " C_DATA "%.2s\n", buf);
  
  get_bfa(buf, 103, 5);
  printf(C_INFO "Product name:       " C_DATA "%.5s\n", buf);
  
  get_bfa(buf, 63, 1);
  printf(C_INFO "Product revision:   " C_DATA "%d.%d\n", buf[0] >> 4, buf[0] & 0x0F);
  
  get_bfa(buf, 55, 4);
  printf(C_INFO "Product serial num: " C_DATA);
  hexstr(buf, 4);
  
  u16 mfd = get_bf(19, 12);
  printf(C_INFO "\nManufacturing date: " C_DATA "%s %d\n", month_txt[(mfd & 0x0F) - 1], (mfd >> 4) + 2000);

  // CSD
  read_csd();
  printf(C_INFO "\n\nCSD: " C_DATA);
  hexstr(sdbuf, 16);

  // OCR
  read_ocr();
  printf(C_INFO "\n\nOCR: " C_DATA);
  hexstr(sd_rbuf, 4);

  // SCR
  read_scr();
  printf(C_INFO "\n\nSCR: " C_DATA);
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
    printf(C_BUTN "\n\n1. " C_MENU "Set password\n\n");
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
        case KEY_0: menu = M_LOCK; rc = true; break;
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
