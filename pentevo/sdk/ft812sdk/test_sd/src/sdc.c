
/*
   8 (IF)  - <R1> <4b>
   9 (CSD) - <R1> <dtok> <16b> <CRC>
  10 (CID) - <R1> <dtok> <16b> <CRC>
  58 (OCR) - <R1> <4b>
A 51 (SCR) - <R1> <dtok> <8b> <CRC>

  *Registers data is stored in MSB format
*/

u8 read_cid()
{
  u8 rc;

  sd_cs_on();
  rc = sd_cmd(SDC_SEND_CID, 0);
  sd_recv_data(sdbuf, 16);
  sd_cs_off();

  return rc;
}

u8 read_csd()
{
  u8 rc;

  sd_cs_on();
  rc = sd_cmd(SDC_SEND_CSD, 0);
  sd_recv_data(sdbuf, 16);
  sd_cs_off();

  return rc;
}

u8 read_ocr()
{
  u8 rc;

  sd_cs_on();
  rc = sd_cmd(SDC_READ_OCR, 0);
  sd_cs_off();

  return rc;
}

u8 read_scr()
{
  u8 rc;

  sd_cs_on();
  rc = sd_acmd(SDC_SEND_SCR, 0);
  sd_recv_data(sdbuf, 8);
  sd_cs_off();

  return rc;
}

u8 read_status()
{
  u8 rc;

  sd_cs_on();
  rc = sd_cmd(SDC_SEND_STATUS, 0);
  sd_cs_off();

  return rc;
}

u8 lock_unlock_sd_(u8 op, const char *pwd, u8 len)
{
  u8 rc;

  sd_cs_on();
  if (rc = sd_cmd(SDC_SET_BLOCKLEN, len + 2)) goto exit;
  if (rc = sd_cmd(SDC_LOCK_UNLOCK, 0)) goto exit;
  sd_wr(0xFF); sd_wr(0xFE);         // data block header
  sd_wr(op);                        // operation
  sd_wr(len);                       // PWD_LEN
  sd_send(pwd, len);                // PWD
  sd_skip(2 + 8);                   // CRC + shit
  if (!sd_wait_busy()) rc = 0xFF;
exit:
  sd_cs_off();

  return rc;
}

u8 lock_unlock_sd(u8 op, const char *pwd)
{
  return lock_unlock_sd_(op, pwd, strlen(pwd));
}

u8 set_password_sd(const char *pwd)
{
  return lock_unlock_sd(SDC_SET_PWD, pwd);
}

u8 clear_password_sd(const char *pwd)
{
  return lock_unlock_sd(SDC_CLR_PWD, pwd);    // CLR_PWD
}

u8 lock_sd(const char *pwd)
{
  return lock_unlock_sd(SDC_LOCK, pwd);
}

u8 unlock_sd(const char *pwd)
{
  return lock_unlock_sd(SDC_UNLOCK, pwd);
}

u8 erase_sd()
{
  u8 rc;

  sd_cs_on();
  if (rc = sd_cmd(SDC_SET_BLOCKLEN, 1)) goto exit;
  if (rc = sd_cmd(SDC_LOCK_UNLOCK, 0)) goto exit;
  sd_wr(0xFF); sd_wr(0xFE);         // data block header
  sd_wr(SDC_FORCE_ERASE);
  sd_skip(2 + 8);                   // CRC + shit
  if (!sd_wait_busy_long()) rc = 0xFF;
exit:
  sd_cs_off();

  return rc;
}

u8 erase_sec()
{
#define ERASE_SECS (2097152UL)  // 1GB
// #define ERASE_SECS (262144UL)  // 128MB

  u8 rc = 0;
  u32 sec = 0;

  sd_cs_on();
  
  while (sec < sd_size)
  {
    printf(C_NORM "Current: " C_DATA "%lu GB\r", sec >> 21);
    
    u32 secs = min(ERASE_SECS, sd_size - sec);
    
    if (rc = sd_cmd(SDC_ERASE_START_ADDR, sec)) goto exit;
    if (rc = sd_cmd(SDC_ERASE_END_ADDR, sec + secs - 1)) goto exit;
    if (rc = sd_cmd(SDC_ERASE, 0)) goto exit;

    // if (!sd_wait_busy_long()) rc = 0xFF;
    while (sd_rd() != 0xFF);  // unlimited wait
    
    sec += secs;
  }

exit:
  sd_cs_off();

  return rc;
}
