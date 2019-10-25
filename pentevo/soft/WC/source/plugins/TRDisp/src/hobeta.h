
typedef struct
{
    char filename[8];
    u8 type;
    u16 start;
    u16 length;
    u8 zero;
    u8 secsize;
    u16 checksum;
} HOBETA_t;

u16 hobeta_checksum(u8 *header);
bool is_hobeta(HOBETA_t *header);
