
/*
   8 (IF)  - <R1> <4b>
   9 (CSD) - <R1> <dtok> <16b> <CRC>
  10 (CID) - <R1> <dtok> <16b> <CRC>
  58 (OCR) - <R1> <4b>
A 51 (SCR) - <R1> <dtok> <8b> <CRC>

  *Registers data is stored in MSB format
*/

bool read_cid()
{
  sd_cs_on();
  sd_cmd(SDC_SEND_CID, 0);
  sd_recv_data(sdbuf, 16);
  sd_cs_off();

  return true;
}

bool read_csd()
{
  sd_cs_on();
  sd_cmd(SDC_SEND_CSD, 0);
  sd_recv_data(sdbuf, 16);
  sd_cs_off();

  return true;
}

bool read_ocr()
{
  sd_cs_on();
  sd_cmd(SDC_READ_OCR, 0);
  sd_cs_off();

  return true;
}

bool read_scr()
{
  sd_cs_on();
  sd_acmd(SDC_SEND_SCR, 0);
  sd_recv_data(sdbuf, 8);
  sd_cs_off();

  return true;
}

bool read_status()
{
  sd_cs_on();
  sd_cmd(SDC_SEND_STATUS, 0);
  sd_cs_off();

  return true;
}

bool lock_unlock_sd(u8 op)
{
  sd_cs_on();
  sd_cmd(SDC_SET_BLOCKLEN, 3);
  sd_cmd(SDC_LOCK_UNLOCK, 0);
  sd_wr(0xFF); sd_wr(0xFE);         // data block header
  sd_wr(op);                        // operation
  sd_wr(1);                         // PWD_LEN = 1
  sd_wr('1');                       // PWD = "1"
  sd_skip(2 + 32);                  // CRC + shit
  while (sd_rd() != 0xFF);
  sd_cs_off();

  return true;
}

bool set_password_sd()
{
  return lock_unlock_sd(1);         // SET_PWD
}

bool clear_password_sd()
{
  return lock_unlock_sd(2);         // CLR_PWD
}

bool lock_sd()
{
  return lock_unlock_sd(4);         // LOCK
}

bool unlock_sd()
{
  return lock_unlock_sd(0);         // UNLOCK
}

bool erase_sd()
{
  sd_cs_on();
  sd_cmd(SDC_SET_BLOCKLEN, 1);
  sd_cmd(SDC_LOCK_UNLOCK, 0);
  sd_wr(0xFF); sd_wr(0xFE);         // data block header
  sd_wr(8);                         // FORCE ERASE
  sd_skip(2 + 32);                  // CRC + shit
  while (sd_rd() != 0xFF);
  sd_cs_off();

  return true;
}

void parse_cid(CID *cid)
{
  bitmax = 127;

  cid->mid = get_bf(127, 8);
  get_bfa(cid->oid, 119, 2);
  get_bfa(cid->pnm, 103, 5);
  cid->prv = get_bf(63, 8);
  get_bfa(cid->psn, 55, 4);
  cid->mdt = get_bf(19, 12);
}
