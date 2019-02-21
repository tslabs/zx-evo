
char *const sd_init_txt[] =
{
  "OK",
  "GO_IDLE_STATE error",
  "Non compatible voltage range",
  "SEND_OP_COND_SD error",
  "READ_OCR error",
  "Error leaving IDLE state"
};

char *const sd_type_txt[] =
{
  "No card",
  "MMC v.3",
  "SD v.1",
  "SD v.2",
  "SDHC"
};

typedef struct
{
  u8 mid;
  u8 oid[2];
  u8 pnm[5];
  u8 prv;
  u8 psn[4];
  u16 mdt;
} CID;

typedef struct
{
  u8 a;
} CSD1;

typedef struct
{
  u8 a;
} CSD2;
