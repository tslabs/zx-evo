
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

const MID_TAB mid_tab[] =
{
  { 0x00, "Invalid"                      },
  { 0x01, "Panasonic"                    },
  { 0x02, "Toshiba"                      },
  { 0x03, "SanDisk"                      },
  { 0x09, "Apacer(?)"                    },
  { 0x13, "KingMax"                      },
  { 0x1a, "PQI(?)"                       },
  { 0x1b, "Samsung"                      },
  { 0x1c, "Transcend"                    },
  { 0x1d, "AData"                        },
  { 0x27, "PHISON"                       },
  { 0x28, "Lexar"                        },
  { 0x31, "Silicon Power"                },
  { 0x41, "Kingston"                     },
  { 0x6f, "Silicon Motion SD Controller" },
  { 0x73, "Fujifilm(?)"                  },
  { 0x74, "Transcend"                    },
  { 0x76, "Patriot(?)"                   },
  { 0x82, "Sony(?)"                      },
};
