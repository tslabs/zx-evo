#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "colors.h"
#include "wc_api.h"
#include "numbers.h"
#include "trdos.h"
#include "hobeta.h"

extern u8 call_type;
extern u16 ret_sp;
extern u32 filesize;
extern u8 file_ext;

#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

#define CRDX_WINMAIN 16
#define CRDY_WINMAIN 3
#define FILES_IN_WINDOW 20

u8 sector_buff[256];                // TRD sector buffer
TRD_FILE_t cat[128];                // TRD catalogue
bool mark[128];                     // marked files
TRD_9SEC_t disk_info[1];

u16 buff_index;                     // index for buffer
u8 buff_512[512];                   // temporary buffer for fputc()

u8 pos_offset;                      // offset file position into the window
u8 num_files;                       // count of files

u16 wc_marked_cnt;                  // count of marked files
char filename[256];                 // temporary filename for marked files

// --- Windows -------------------------------------------------
const WC_TX_WINDOW win_main =
{
  /* window with header and text    */  WC_WIND_HDR_TXT_FRM2 | 0x80,
  /* cursor color mask              */  0,
  /* X,Y (position)                 */  CRDX_WINMAIN, CRDY_WINMAIN,
  /* W,H (size)                     */  52, 25,
  /* paper/ink (window color)       */  (COL_WHITE<<4) | COL_BRIGHT_WHITE,
  /* -reserved-                     */  0,
  /* window restore buffer address  */  0,
  /* separators                     */  0, 0,
  /* header text                    */  "\x09\x0E""TRDispatcher v0.1b",
  /* footer text                    */  "",
  /* window text                    */  ""
};

const WC_TX_WINDOW win_info =
{
  /* window with header and text    */  WC_WIND_TXT_FRM2,
  /* cursor color mask              */  0,
  /* X,Y (position)                 */  CRDX_WINMAIN+18, CRDY_WINMAIN+2,
  /* W,H (size)                     */  33, 7,
  /* paper/ink (window color)       */  (COL_WHITE<<4) | COL_BRIGHT_WHITE,
  /* -reserved-                     */  0,
  /* window restore buffer address  */  0xFFFF,
  /* separators                     */  0, 0,
  /* header text                    */  "",
  /* footer text                    */  "",
  /* window text                    */  "Disk title............:\r"
                                        "Total sectors............:\r"
                                        "Free sectors.............:\r"
                                        "Files....................:\r"
                                        "Deleted..................:\r"
};

const WC_TX_WINDOW win_fileinfo =
{
  /* window with header and text    */  WC_WIND_TXT_FRM2,
  /* cursor color mask              */  0,
  /* X,Y (position)                 */  CRDX_WINMAIN+18, CRDY_WINMAIN+10,
  /* W,H (size)                     */  33, 7,
  /* paper/ink (window color)       */  (COL_WHITE<<4) | COL_BRIGHT_WHITE,
  /* -reserved-                     */  0,
  /* window restore buffer address  */  0xFFFF,
  /* separators                     */  0, 0,
  /* header text                    */  "",
  /* footer text                    */  "",
  /* window text                    */  "Start address............:\r"
                                        "Length...................:\r"
                                        "Size.....................:\r"
                                        "Begin sector.............:\r"
                                        "Begin track..............:\r"
};

WC_TX_WINDOW win_filecopy =
{
  /* window with header and text    */  WC_WIND_TXT_FRM2,
  /* cursor color mask              */  0,
  /* X,Y (position)                 */  CRDX_WINMAIN+6, CRDY_WINMAIN+8,
  /* W,H (size)                     */  40, 7,
  /* paper/ink (window color)       */  (COL_GREEN<<4) | COL_BRIGHT_WHITE,
  /* -reserved-                     */  0,
  /* window restore buffer address  */  0,
  /* separators                     */  0, 0,
  /* header text                    */  "",
  /* footer text                    */  "",
  /* window text                    */  "\r\r        Extracting:"
};

WC_MNU_WINDOW win_files =
{
  /* window with header and text    */  WC_WIND_HDR_TXT_CUR_FRM2 | 0x40,
  /* cursor color mask              */  0,
  /* X,Y (position)                 */  CRDX_WINMAIN+1, CRDY_WINMAIN+2,
  /* W,H (size)                     */  16, FILES_IN_WINDOW+2,
  /* paper/ink (window color)       */  (COL_BLACK<<4) | COL_BRIGHT_WHITE,
  /* -reserved-                     */  0,
  /* window restore buffer address  */  0xFFFF,
  /* separators                     */  0, 0,
  /* cur_pos                        */  1,
  /* cur_stop                       */  20,
  /* cur_col                        */  (COL_CYAN<<4) | COL_BLACK,
  /* win_col                        */  (COL_BLACK<<4) | COL_BRIGHT_WHITE,
  /* header text                    */  "",
  /* footer text                    */  "",
  /* window text                    */  ""
};

const char invalid_chars[] = {'.', '\\', '/', ':', '?', '*', '<', '>', '|', '"'};

// -------------------------------------------------------------
void fputc(u8 byte) __z88dk_fastcall
{
    buff_512[buff_index++] = byte;

    if(buff_index == 512)
    {
        buff_index = 0;
        wc_save512(buff_512, 1);
        memset(buff_512, 0, 512);
    }
}

void fflush()
{
    memset(buff_512+buff_index, 0, 512-buff_index);
    buff_index = 0;
    wc_save512(buff_512, 1);
    memset(buff_512, 0, 512);
}

void fput_bytes(u8 *addr, u16 size)
{
    for (u16 i=0; i<size; i++) fputc(addr[i]);
}

bool is_marked()
{
    bool rc = false;
    for(u8 i=0; i<128; i++)
    {
        if(mark[i]) rc = true;
    }
    return rc;
}

bool is_validchar(char c)
{
    if(c < 0x20 || c > 0x7F) return false;
    for(u8 i=0; i<sizeof(invalid_chars); i++)
    {
        if(c == invalid_chars[i]) return false;
    }
    return true;
}

void validating_filename(char *filename)
{
    bool flag = false;
    s8 i = 7;
    char c;

    while(i >= 0)
    {
        c = filename[i];
        if(!is_validchar(c)) c = '_';
        if(c != 0x20) flag = true;
        if(c == 0x20 && flag == true) c = '_';
        filename[i] = c;
        i--;
    }

    c = filename[10];
    if(!is_validchar(c)) c = '_';
    filename[10] = c;
}

// -------------------------------------------------------------
u8 mkfile(u8 *file_name, u32 size)
{
    WC_FILENAME FILENAME;
    char c;
    u8 i = 0;

    FILENAME.flag = 0;
    FILENAME.size = size;

    while (c=file_name[i])
    {
        FILENAME.name[i] = c;
        i++;
    }
    FILENAME.name[i] = 0;
    return wc_mkfile(&FILENAME);
}

void write_file(u8 filenum)
{
    u8 track = cat[filenum].track;
    u8 sector = cat[filenum].sector;
    u8 numsec = cat[filenum].n_sec;

    u8 i;

    for(i=0; i<numsec; i++)
    {
        trd_read_sector(sector_buff, track, sector);
        fput_bytes(sector_buff, 256);

        sector++;
        if(!(sector&=0x0F)) track++;
    }
    fflush();
}

// -------------------------------------------------------------
void extract_to_hobeta()
{
    HOBETA_t header[1];
    char filename[16];
    u16 len;
    u8 i, j;

    wc_print_window(&win_filecopy);

    for(i=0; i<128; i++)
    {
        if(mark[i])
        {
            // preparing hobeta header
            memcpy(header->filename, cat[i].name, 8);
            header->type = cat[i].type;
            header->start = cat[i].start;
            header->length = cat[i].length;
            header->zero = 0x00;
            header->secsize = cat[i].n_sec;
            header->checksum = hobeta_checksum((u8*)header);

            // preparing filename
            memcpy(filename, cat[i].name, 8);
            memcpy(filename+8, ".$", 2);
            memcpy(filename+10, &cat[i].type, 1);
            filename[11] = 0x00;

            wc_print(&win_filecopy, 20, 3, filename);

            len = (cat[i].length>>8 < cat[i].n_sec) ? cat[i].n_sec<<8 : cat[i].length;

            validating_filename(filename);

            for(j=0; j<255; j++)
            {
                if(!mkfile(filename, len+17))
                {
                    fput_bytes((u8*)header, 17);
                    write_file(i);
                    break;
                }
                else
                {
                    memcpy(filename+11, dec2asc8(j), 4);
                }
            }
        }
    }
    wc_restore_window(&win_filecopy);
}
// -------------------------------------------------------------

void trd_save_file()
{
    //trd_write_sector(sector_buff, trk, sec);
}

void push_files()
{
    u8 trk, sec;
    u8 trd_secnum;
    u16 free_sec;
    u32 fsize;
    WC_FILENAME wc_filename[1];

    wc_get_marked_file(1, filename);

    wc_filename->flag = 0;
    strcpy(wc_filename->name, filename);

    fsize = wc_ffind(wc_filename);

    trd_secnum = (fsize>>8) ? fsize>>8 : (fsize>>8) + 1;

    free_sec = disk_info->n_free_sec;

    if(trd_secnum>free_sec)
    {
        // no disk space!
        return;
    }

    trk = disk_info->free_trk_next;
    sec = disk_info->free_sec_next;

    for(u8 i=0; i<trd_secnum; i++)
    {
        wc_load256(sector_buff, 1);
        trd_write_sector(sector_buff, trk, sec);
        sec++;
        if(!(sec&=0x0F)) trk++;
    }
}

// -------------------------------------------------------------
void print_file(u8 file, u8 y)
{
    u8 a, b;

    wc_print_w(&win_files, 1, y+1, (!mark[file])?" ":"\x16", 1);

    wc_print_w(&win_files, 2, y+1, cat[file].name, 8);
    wc_print_w(&win_files, 10, y+1, " ", 1);

    a = (u8)(cat[file].start)&0xFF;
    b = (u8)((cat[file].start)>>8)&0xFF;

    if(((a>=0x20 && a<0x80) && (b>=0x20 && b<0x80)) && (a+b!=0x40))
    {
        wc_print_w(&win_files, 11, y+1, cat[file].ext, 3);
    } else
    {
        wc_print_w(&win_files, 11, y+1, "<", 1);
        wc_print_w(&win_files, 12, y+1, (char*)&cat[file].type, 1);
        wc_print_w(&win_files, 13, y+1, ">", 1);
    }
}

void print_cat()
{
    u8 i;
    u8 j;

    i = 0;

    while(i<FILES_IN_WINDOW)
    {
        j=i+pos_offset;
        if(j>=num_files)
        {
            wc_print(&win_files, 1, i+1, "              ");
        }
        else
        {
            print_file(j, i);
        }
        i++;
    }
}

void print_num(void *win, u16 num, u8 y)
{
    char str[10];
    strcpy(str,dec2asc16(num));
    wc_print(win, 32-(strlen(str)), y, str);
}

void print_disk_info()
{
    wc_print_w(&win_info, 32-8, 1, disk_info[0].disk_title, 8);
    print_num(&win_info, disk_info[0].n_free_sec, 3);
    print_num(&win_info, disk_info[0].n_files, 4);
    print_num(&win_info, disk_info[0].n_del_files, 5);
}

void print_file_info()
{
    u8 i = win_files.cur_pos+pos_offset-1;

    wc_print_w(&win_fileinfo, 32-5, 1, "     ", 5); print_num(&win_fileinfo, cat[i].start, 1);
    wc_print_w(&win_fileinfo, 32-5, 2, "     ", 5); print_num(&win_fileinfo, cat[i].length, 2);
    wc_print_w(&win_fileinfo, 32-3, 3, "   ", 3); print_num(&win_fileinfo, cat[i].n_sec, 3);
    wc_print_w(&win_fileinfo, 32-3, 4, "   ", 3); print_num(&win_fileinfo, cat[i].sector, 4);
    wc_print_w(&win_fileinfo, 32-3, 5, "   ", 3); print_num(&win_fileinfo, cat[i].track, 5);
}

void read_trd()
{
    u8 i;

    for(i=0; i<64; i++)
    {
        wc_vpage3(i);
        wc_load512((u16*)0xC000, 32);
    }
}

void init_variables()
{
    pos_offset = 0;
    num_files = disk_info[0].n_files;
    buff_index = 0;
    memset(&mark[0], false, sizeof mark);
}

void cursor_dn()
{
    if(win_files.cur_pos<FILES_IN_WINDOW && (win_files.cur_pos+pos_offset)<num_files)
    {
        wc_restore_cursor(&win_files);
        win_files.cur_pos++;
        wc_print_cursor(&win_files);
    } else if(pos_offset<(num_files-FILES_IN_WINDOW))
    {
        pos_offset++;
        print_cat();
    }
    print_file_info();
}

void cursor_up()
{
    if(win_files.cur_pos>1)
    {
        wc_restore_cursor(&win_files);
        win_files.cur_pos--;
        wc_print_cursor(&win_files);

    } else if(pos_offset>0)
    {
        pos_offset--;
        print_cat();
    }
    print_file_info();
}

void cursor_pgdn()
{
    wc_restore_cursor(&win_files);
    for(u8 i=0;i<FILES_IN_WINDOW;i++)
    {
        if(win_files.cur_pos>1) win_files.cur_pos--;
        else if(pos_offset>0) pos_offset--;
    }
    print_cat();
    wc_print_cursor(&win_files);
    print_file_info();
}

void cursor_pgup()
{
    wc_restore_cursor(&win_files);
    for(u8 i=0;i<FILES_IN_WINDOW;i++)
    {
        if(win_files.cur_pos<FILES_IN_WINDOW && (win_files.cur_pos+pos_offset)<num_files) win_files.cur_pos++;
        else if(pos_offset<(num_files-FILES_IN_WINDOW)) pos_offset++;
    }
    print_cat();
    wc_print_cursor(&win_files);
    print_file_info();
}

void browse()
{
    u8 file_cur;

    while(1)
    {
        EIHALT

        // if esc key pressed pressed
        if(wc_api__bool(_ESC)) break;

        file_cur = win_files.cur_pos+pos_offset-1;

        // if space key pressed
        if(wc_api__bool(_SPKE))
        {
            mark[file_cur] = !mark[file_cur];
            print_file(file_cur, win_files.cur_pos-1);
            cursor_dn();
        }

        // if cursor down pressed
        if(wc_api__bool(_DWWW)) cursor_dn();

        // if cursor up pressed
        if(wc_api__bool(_UPPP)) cursor_up();

        // if pgdn pressed
        if(wc_api__bool(_PGD)) cursor_pgup();

        // if pgdn pressed
        if(wc_api__bool(_PGU)) cursor_pgdn();

        if(wc_keyscan() == 'x')
        {
            if (!is_marked()) mark[file_cur] = true;
            print_cat();
            extract_to_hobeta();
            memset(mark, false, 128);
            print_cat();
        }
    }
}

void browse_entry()
{
    read_trd();
    wc_print_window(&win_main);
    wc_print_window(&win_files);
    wc_print_window(&win_info);
    wc_print_window(&win_fileinfo);

    trd_read_cat(cat, disk_info);

    init_variables();

    print_cat();
    wc_print_cursor(&win_files);
    print_disk_info();
    print_file_info();

    wc_print(&win_main, 20, 23, "Press ""\x06\x06""X""\x07\x07"" key to extract file(s)");

    wc_marked_cnt = wc_get_marked_file(0, 0);

    if(!wc_marked_cnt)
    {
        browse();
        wc_restore_window(&win_main);
    } else
    {
        wc_restore_window(&win_main);
    }
}

// -------------------------------------------------------------
void main()
{
    switch (call_type)
    {
        case WC_CALL_EXT:
            browse_entry();
            wc_exit(WC_REREAD_DIR);
        break;

        case WC_CALL_EMENU:
            browse_entry();
            wc_exit(WC_REREAD_DIR);
        break;

        case WC_CALL_MENU:
            wc_exit(WC_REREAD_DIR);
        break;

        default:
            wc_exit(WC_REREAD_DIR);
    }
}
