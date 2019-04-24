
char *const sd_init_txt[] =
{
  "OK",
  "GO_IDLE_STATE error",
  "Non compatible voltage range",
  "SEND_OP_COND_SD error",
  "READ_OCR error",
  "Error leaving IDLE state"
};

char *const r1_err_txt[] =
{
  "Idle state",
  "Erase reset",
  "Illegal command",
  "Com CRC error",
  "Erase sequence error",
  "Address error",
  "Parameter error"
};
  
char *const sd_type_txt[] =
{
  "No card",
  "MMC v.3",
  "SD v.1",
  "SD v.2",
  "SDHC"
};

char *const month_txt[] =
{
  "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};

typedef struct
{
  u8 mid;
  char *name;
} MID_TAB;

typedef struct
{
  union
  {
    struct
    {
      u8 unit:3;
      u8 value:4;
      u8 _res:1;
    };
    u8 byte;
  };
} TRAN_SPEED;

const MID_TAB mid_tab[] =
{
  { 0x00, "Invalid"        },
  { 0x01, "Panasonic"      },
  { 0x02, "Toshiba"        },
  { 0x03, "SanDisk"        },
  { 0x09, "Apacer(?)"      },
  { 0x11, "(?)"            },
  { 0x12, "(?)"            },
  { 0x13, "KingMax"        },
  { 0x1a, "PQI(?)"         },
  { 0x1b, "Samsung"        },
  { 0x1c, "Transcend"      },
  { 0x1d, "AData"          },
  { 0x27, "Phison"         },
  { 0x28, "Lexar"          },
  { 0x31, "Silicon Power"  },
  { 0x41, "Kingston"       },
  { 0x6f, "Silicon Motion" },
  { 0x70, "(?)"            },
  { 0x73, "Fujifilm(?)"    },
  { 0x74, "Transcend"      },
  { 0x76, "Patriot(?)"     },
  { 0x82, "Sony(?)"        },
};

char *const csd_ver_txt[] = { "1.0", "2.0", "(reserved)", "(reserved)" };
const u8 spd_mul[] = { 10, 100, 1, 10, 0, 0, 0, 0 };
const u8 spd_val[] = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
char *const spd_unit_txt[] = { "k", "k", "M", "M", "", "", "", "" };
