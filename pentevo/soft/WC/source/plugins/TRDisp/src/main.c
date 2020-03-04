//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                   TRDispatcher v0.43b                   ::
//::                  Wild Commander plugin                  ::
//::                    dr_max^gc (c)2019                    ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "colors.h"
#include "wc_api.h"
#include "numbers.h"
#include "trdos.h"
#include "hobeta.h"
#include "ui.h"

extern u8 wc_call_type;
extern u16 wc_ret_sp;
extern u32 wc_filesize;
extern u8 wc_file_ext;
extern u16 wc_filename;

#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

u8 sector_buff[256];                // TRD sector buffer
TRD_FILE_t cat[128];                // TRD catalogue
TRD_9SEC_t disk_info;               // TRD 9 sector
bool mark[128];                     // marked files

u8 track;                           // current track
u8 sector;                          // current sector
u16 total_sectors;                  // number of sectors

u16 buff_index;                     // index for buffer
u8 buff_512[512];                   // temp buffer for fputc()

u8 pos_offset;                      // offset file position into the window

u16 wc_marked_cnt;                  // count of marked files
char filename[256];                 // temp filename for marked files
char trd_filename[256];             // temp filename to save TRD filename at startup

const char invalid_chars[] = {'.', '\\', '/', ':', '?', '*', '<', '>', '|', '"'};

char txt_extract[] = "\r\r        Extracting:";
char txt_copy[] = "\r                Copying:";

// -------------------------------------------------------------
void infobox(char *message)
{
    win_message.attr = (COL_GREEN<<4) | COL_BRIGHT_WHITE;
    wc_print_window(&win_message);
    wc_print(&win_message, (MESSAGEBOX_WIDTH - strlen(message))>>1, 3, message);
}

void infobox_close()
{
    wc_restore_window(&win_message);
}

void messagebox(ALERT_COLORS_t alert, char *message)
{
    switch(alert)
    {
    case ALERT_INFO:
        win_message.attr = (COL_GREEN<<4) | COL_BRIGHT_WHITE;
        break;

    case ALERT_ERROR:
        win_message.attr = (COL_RED<<4) | COL_BRIGHT_WHITE;
        break;

    case ALERT_NOTICE:
        win_message.attr = (COL_YELLOW<<4) | COL_BRIGHT_WHITE;
        break;

    default:
        win_message.attr = (COL_GREEN<<4) | COL_BRIGHT_WHITE;
        break;
    }

    wc_print_window(&win_message);
    wc_print(&win_message, (MESSAGEBOX_WIDTH - strlen(message))>>1, 3, message);
    wc_api_u8(_USPO);
    wc_api_u8(_NUSP);
    wc_restore_window(&win_message);
}

bool sure()
{
    wc_print_window(&win_sure);
    while(1)
    {
        EIHALT;
        switch (wc_keyscan())
        {
        case 'y':
            {
                wc_restore_window(&win_sure);
                return true;
            }
        case 'n':
            {
                wc_restore_window(&win_sure);
                return false;
            }
        }
    }
}

void print_num(void *win, u16 num, u8 y)
{
    char str[10];
    strcpy(str,dec2asc16(num));
    wc_print(win, 32-(strlen(str)), y, str);
}

// -------------------------------------------------------------
void fseek_begin()
{
    buff_index = 0;
}

u8 fgetc()
{
    if(!buff_index)
        wc_load512(buff_512, 1);
    if(buff_index == 512)
    {
        buff_index = 0;
        wc_load512(buff_512, 1);
    }
    return buff_512[buff_index++];
}

void fget_bytes(u8 *addr, u16 size)
{
    for (u16 i=0; i<size; i++)
        addr[i] = fgetc();
}

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

void fput_bytes(u8 *addr, u16 size)
{
    for (u16 i=0; i<size; i++)
        fputc(addr[i]);
}

void fflush()
{
    memset(buff_512+buff_index, 0, 512-buff_index);
    buff_index = 0;
    wc_save512(buff_512, 1);
    memset(buff_512, 0, 512);
}

// -------------------------------------------------------------
bool is_marked()
{
    bool rc = false;
    for(u8 i=0; i<128; i++)
        if(mark[i]) rc = true;
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
    if(!is_validchar(c))
        c = '_';
    filename[10] = c;
}

char* find_ext(char *filename)
{
    u8 len = strlen(filename);
    while(len--)
    {
        if(filename[len]=='.')
            break;
    }
    return &filename[len];
}

// -------------------------------------------------------------
u8 mkfile(u8 *file_name, u32 size)
{
    WC_FILENAME filename;
    filename.flag = 0;
    filename.size = size;
    strcpy((char*)&(filename).name, file_name);
    return wc_mkfile(&filename);
}

void write_file(u8 filenum)
{
    u8 track = cat[filenum].track;
    u8 sector = cat[filenum].sector;
    u8 numsec = cat[filenum].num_sec;

    for(u8 i=0; i<numsec; i++)
    {
        trd_read_sector(sector_buff, track, sector);
        fput_bytes(sector_buff, 256);

        sector++;
        if(!(sector&=0x0F))
            track++;
    }
    fflush();
}

// -------------------------------------------------------------
void extract_to_hobeta()
{
    HOBETA_t hobeta_header;
    char filename[16];
    u16 filesize;
    u8 i, j;

    win_filecopy.wnd_txt = txt_extract;
    wc_print_window(&win_filecopy);

    for(i=0; i<128; i++)
    {
        if(wc_api__bool(_ESC))
        {
            messagebox(ALERT_NOTICE, "Breaked");
            break;
        }

        if(mark[i])
        {
            // preparing hobeta header
            memcpy(hobeta_header.filename, cat[i].name, 8);
            hobeta_header.type = cat[i].type;
            hobeta_header.start = cat[i].start;
            hobeta_header.length = cat[i].length;
            hobeta_header.zero = 0x00;
            hobeta_header.secsize = cat[i].num_sec;
            hobeta_header.checksum = hobeta_checksum((u8*)hobeta_header);

            // preparing filename
            memcpy(filename, cat[i].name, 8);
            memcpy(filename+8, ".$", 2);
            memcpy(filename+10, &cat[i].type, 1);
            filename[11] = 0x00;

            validating_filename(filename);
            wc_print(&win_filecopy, 20, 3, filename);

            filesize = cat[i].num_sec<<8;

            for(j=0; j<255; j++)
            {
                if(!mkfile(filename, filesize + sizeof(hobeta_header)))
                {
                    fput_bytes((u8*)hobeta_header, sizeof(hobeta_header));
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
void put_file_to_trd(u8 track, u8 sector, u8 num_sectors)
{
    for(u8 i=0; i<num_sectors; i++)
    {
        fget_bytes(sector_buff, 256);
        trd_write_sector(sector_buff, track, sector);
        sector++;
        if(!(sector&=0x0F))
            track++;
    }
    disk_info.free_trk_next = track;
    disk_info.free_sec_next = sector;
}

void put_file_to_cat(TRD_FILE_t *file)
{
    memcpy(&cat[disk_info.num_files++], file, 16);
}

u8 _delete_file(u8 i)
{
    // set end of catalogue
    cat[i].name[0] = 0x00;
    disk_info.free_trk_next = cat[i].track;
    disk_info.free_sec_next = cat[i].sector;
    disk_info.num_free_sec += cat[i].num_sec;
    disk_info.num_files--;
    // if number of file not 0 and previous file is deleted
    if((i) && (cat[i-1].name[0] == 0x01))
    {
        disk_info.num_del_files--;
        i = _delete_file(i-1);
    }
    return i;
}

void delete_files()
{
    u8 i = disk_info.num_files;

    while(i != 255)
    {
        // if marked for deletion and if not already deleted
        if(mark[i] && cat[i].name[0] != 0x01)
        {
            // if deleted file is the last
            if(disk_info.num_files == i+1)
            {
                i = _delete_file(i);
            }
            else
            {
                // set delete
                cat[i].name[0] = 0x01;
                disk_info.num_del_files++;
            }
        }
        i--;
    }
}

void skip_sectors(u8 *track, u8 *sector, u8 secnum)
{
    for(u8 i=0; i<secnum; i++)
    {
        sector[0]++;
        if(!(sector[0]&=0x0F))
            track[0]++;
    }
}

void copy_file(u8 dst, u8 src)
{
    u8 src_trk, src_sec;
    u8 dst_trk, dst_sec;
    static u8 track;
    static u8 sector;

    if(dst)
    {
        track = cat[dst-1].track;
        sector = cat[dst-1].sector;
        skip_sectors(&track, &sector, cat[dst-1].num_sec);
    } else
    {
        track = 1;
        sector = 0;
    }
    cat[dst].track = track;
    cat[dst].sector = sector;

    src_trk = cat[src].track;
    src_sec = cat[src].sector;
    dst_trk = cat[dst].track;
    dst_sec = cat[dst].sector;

    for(u8 i=0; i<cat[src].num_sec; i++)
    {
        trd_read_sector(sector_buff, src_trk, src_sec);
        trd_write_sector(sector_buff, dst_trk, dst_sec);
        src_sec++;
        if(!(src_sec&=0x0F))
            src_trk++;
        dst_sec++;
        if(!(dst_sec&=0x0F))
            dst_trk++;
    }

    memcpy(&cat[dst], &cat[src], 14);
}

bool _move_file(u8 filenum)
{
    bool is_end = true;

    for(u8 i=filenum; i<disk_info.num_files; i++)
    {
        // if file not deleted
        if(cat[i].name[0] != 0x01)
        {
            copy_file(filenum, i);
            cat[i].name[0] = 0x01;

            // if last file in catalogue
            if(disk_info.num_files == i+1)
            {
                disk_info.num_files--;
                is_end = true;
            } else
            {
                is_end = false;
            }
            break;
        }
    }
    return is_end;
}

void move()
{
    bool is_end = false;
    static u8 track;
    static u8 sector;

    while(!is_end)
    {
        for(u8 i=0; i<disk_info.num_files; i++)
        {
            // if file deleted
            if(cat[i].name[0] == 0x01)
            {
                is_end = _move_file(i);
                break;
            }
        }
    }

    // calculate number of files
    u8 i = 0;
    while((cat[i].name[0] >= 0x20) && (i!=128))
        i++;

    cat[i].name[0] = 0;

    disk_info.num_files = i;
    disk_info.num_del_files = 0;

    // calculate first free track/sector
    track = cat[i-1].track;
    sector = cat[i-1].sector;
    skip_sectors(&track, &sector, cat[i-1].num_sec);

    disk_info.free_trk_next = track;
    disk_info.free_sec_next = sector;

    // calculate free sectors
    u16 count_sectors = 0;
    for(u8 i=0; i<disk_info.num_files; i++)
        count_sectors += cat[i].num_sec;

    disk_info.num_free_sec = total_sectors - count_sectors - 16;
}

void put_files()
{
    bool is_hobeta;
    u8 track, sector;
    u8 trd_secnum;
    u16 free_sec;
    u32 fsize;
    u16 marked_files;
    HOBETA_t hobeta_header;
    TRD_FILE_t trd_file;

    win_filecopy.wnd_txt = txt_copy;
    wc_print_window(&win_filecopy);

    marked_files = wc_get_marked_file(0,NULL);

    for(u16 i=1; i<=marked_files; i++)
    {
        if(wc_api__bool(_ESC))
        {
            messagebox(ALERT_NOTICE, "Breaked");
            break;
        }

        wc_get_marked_file(i, &((u8)filename)+1);
        *((u8*)filename) = 0;
        fsize = wc_ffind(filename);
        wc_api_u8(_GFILE);

        if(fsize > 65535)
        {
            messagebox(ALERT_ERROR, "File too big! Press any key");
            continue;
        }

        trd_secnum = (fsize>>8) ? fsize>>8 : (fsize>>8) + 1;

        fseek_begin();
        is_hobeta = false;

        // is HOBETA?
        if(find_ext(&((u8)filename)+1)[1]=='$')
        {
            // get HOBETA header
            fget_bytes(&(u8)hobeta_header, sizeof(hobeta_header));

            if((hobeta_checksum((u8*)hobeta_header) == hobeta_header.checksum) && (hobeta_header.zero == 0x00))
            {
                is_hobeta = true;

                // preparing TRDOS filename
                memcpy(trd_file.name, hobeta_header.filename, 8);
                trd_file.type = hobeta_header.type;
                trd_file.start = hobeta_header.start;
                trd_file.length = hobeta_header.length;
                trd_file.num_sec = hobeta_header.secsize;
            }
        }
        else
        {
            is_hobeta = false;

            // preparing TRDOS filename
            memcpy(trd_file.name, &((u8)filename)+1, 8);
            trd_file.type = 'C';
            trd_file.start = 0x0000;
            trd_file.length = fsize;
            trd_file.num_sec = trd_secnum;
        }

        // if not HOBETA then fseek to begin
        if(!is_hobeta)
        {
            fseek_begin();
            wc_api_u8(_GFILE);
        }

        track = disk_info.free_trk_next;
        sector = disk_info.free_sec_next;

        trd_file.track = track;
        trd_file.sector = sector;

        free_sec = disk_info.num_free_sec;

        if(trd_secnum > free_sec)
        {
            messagebox(ALERT_ERROR, "No disk space!");
            break;
        }

        if(disk_info.num_files == 128)
        {
            messagebox(ALERT_ERROR, "Directory full!");
            break;
        }

        wc_print(&win_filecopy, 15, 5, &((u8)filename)+1);

        put_file_to_cat(&trd_file);
        put_file_to_trd(track, sector, trd_secnum);
        disk_info.num_free_sec -= trd_secnum;
    }

    wc_restore_window(&win_filecopy);
}

// -------------------------------------------------------------

void read_trd()
{
    for(u8 i=0; i<64; i++)
    {
        wc_vpage3(i);
        wc_load512((u8*)0xC000, 32);
    }
    total_sectors = wc_filesize>>8;
}

void update_catalogue()
{
    trd_save_cat(cat, &disk_info);
}

void renew_trd()
{
    *((u8*)filename) = 0;
    strcpy(filename+1, trd_filename);
    wc_ffind(filename);

    wc_api_u8(_GFILE);
    fseek_begin();

    update_catalogue();

    for(u8 i=0; i<64; i++)
    {
        wc_vpage3(i);
        wc_save512((u8*)0xC000, 32);
    }
}

// -------------------------------------------------------------
void print_disk_info()
{
    wc_print_w(&win_info, 32-8, 1, disk_info.disk_title, 8); print_num(&win_info, total_sectors-16, 2);
    wc_print_w(&win_info, 32-3, 3, "  ", 3); print_num(&win_info, disk_info.num_free_sec, 3);
    wc_print_w(&win_info, 32-3, 4, "  ", 3); print_num(&win_info, disk_info.num_files, 4);
    wc_print_w(&win_info, 32-3, 5, "  ", 3); print_num(&win_info, disk_info.num_del_files, 5);
    wc_print_w(&win_info, 32-3, 6, "  ", 3); print_num(&win_info, disk_info.free_trk_next, 6);
    wc_print_w(&win_info, 32-3, 7, "  ", 3); print_num(&win_info, disk_info.free_sec_next, 7);
}

void print_file_info()
{
    u8 i = win_files.cur_pos+pos_offset-1;

    wc_print_w(&win_fileinfo, 32-5, 1, "     ", 5); print_num(&win_fileinfo, cat[i].start, 1);
    wc_print_w(&win_fileinfo, 32-5, 2, "     ", 5); print_num(&win_fileinfo, cat[i].length, 2);
    wc_print_w(&win_fileinfo, 32-3, 3, "  ", 3); print_num(&win_fileinfo, cat[i].num_sec, 3);
    wc_print_w(&win_fileinfo, 32-3, 4, "  ", 3); print_num(&win_fileinfo, cat[i].track, 4);
    wc_print_w(&win_fileinfo, 32-3, 5, "  ", 3); print_num(&win_fileinfo, cat[i].sector, 5);
}

void print_file(u8 file, u8 y)
{
    u8 a, b;

    wc_print_w(&win_files, 1, y+1, (!mark[file])?" ":"\x07", 1);

    wc_print_w(&win_files, 2, y+1, cat[file].name, 8);
    wc_print_w(&win_files, 10, y+1, " ", 1);

    a = (u8)(cat[file].start)&0xFF;
    b = (u8)((cat[file].start)>>8)&0xFF;

    if(((a>=0x20 && a<0x80) && (b>=0x20 && b<0x80)) && (a+b!=0x40))
    {
        wc_print_w(&win_files, 11, y+1, cat[file].ext, 3);
    }
    else
    {
        wc_print_w(&win_files, 11, y+1, "<", 1);
        wc_print_w(&win_files, 12, y+1, (char*)&cat[file].type, 1);
        wc_print_w(&win_files, 13, y+1, ">", 1);
    }
}

void print_cat()
{
    u8 i = 0;
    u8 j;

    while(i<FILES_IN_WINDOW)
    {
        j = i + pos_offset;
        if(j >= disk_info.num_files)
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

void cursor_dn()
{
    if(win_files.cur_pos<FILES_IN_WINDOW && (win_files.cur_pos+pos_offset)<disk_info.num_files)
    {
        wc_restore_cursor(&win_files);
        win_files.cur_pos++;
        wc_print_cursor(&win_files);
    }
    else if(pos_offset<(disk_info.num_files-FILES_IN_WINDOW))
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
    }
    else if(pos_offset>0)
    {
        pos_offset--;
        print_cat();
    }
    print_file_info();
}

void cursor_pgup()
{
    if(!(win_files.cur_pos+pos_offset-1)) return;
    wc_restore_cursor(&win_files);
    for(u8 i=0; i<FILES_IN_WINDOW; i++)
    {
        if(win_files.cur_pos>1)
            win_files.cur_pos--;
        else if(pos_offset>0)
            pos_offset--;
    }
    print_cat();
    wc_print_cursor(&win_files);
    print_file_info();
}

void cursor_pgdn()
{
    wc_restore_cursor(&win_files);
    for(u8 i=0; i<FILES_IN_WINDOW; i++)
    {
        if(win_files.cur_pos<FILES_IN_WINDOW && (win_files.cur_pos+pos_offset)<disk_info.num_files)
            win_files.cur_pos++;
        else if(pos_offset<(disk_info.num_files-FILES_IN_WINDOW))
            pos_offset++;
    }
    print_cat();
    wc_print_cursor(&win_files);
    print_file_info();
}

void init_variables()
{
    pos_offset = 0;
    win_files.cur_pos = 1;
    buff_index = 0;
    memset(&mark[0], false, sizeof mark);
}

void browse()
{
    u8 file_cur;

    if(!disk_info.num_files)
    {
        messagebox(ALERT_NOTICE, "No files!");
        return;
    }

    while(1)
    {
        EIHALT

        // if esc key pressed pressed
        if(wc_api__bool(_ESC))
            break;

        file_cur = win_files.cur_pos+pos_offset-1;

        // if space key pressed
        if(wc_api__bool(_SPKE) && disk_info.num_files)
        {
            mark[file_cur] = !mark[file_cur];
            print_file(file_cur, win_files.cur_pos-1);
            cursor_dn();
        }

        // if cursor down pressed
        if(wc_api__bool(_DWWW))
            cursor_dn();

        // if cursor up pressed
        if(wc_api__bool(_UPPP))
            cursor_up();

        // if pgdn pressed
        if(wc_api__bool(_PGD))
            cursor_pgdn();

        // if pgdn pressed
        if(wc_api__bool(_PGU))
            cursor_pgup();

        switch (wc_keyscan())
        {
            case 'x':
            {
                if (!disk_info.num_files)
                    break;
                if (!is_marked())
                    mark[file_cur] = true;
                print_cat();
                extract_to_hobeta();
                memset(mark, false, 128);
                print_cat();
                break;
            }
            case 'd':
            {
                if (!is_marked())
                    mark[file_cur] = true;
                print_cat();
                wc_restore_cursor(&win_files);
                delete_files();
                memset(&mark[0], false, sizeof mark);
                init_variables();
                print_cat();
                wc_print_cursor(&win_files);
                print_disk_info();
                print_file_info();
                break;
            }
            case 'm':
            {
                if(disk_info.num_del_files)
                {
                    if(sure())
                    {
                        infobox("Moving files...");
                        move();
                        infobox_close();
                        wc_restore_cursor(&win_files);
                        init_variables();
                        print_cat();
                        wc_print_cursor(&win_files);
                        print_disk_info();
                        print_file_info();
                    }
                }
                break;
            }
            case 's':
            {
                if(sure())
                {
                    infobox("Updating TRD file...");
                    renew_trd();
                    infobox_close();
                }
                break;
            }
            default:
                break;
        }
    }
}

void browse_entry()
{
    strcpy(trd_filename, (char*)wc_filename);
    read_trd();
    trd_read_cat(cat, &disk_info);

    if(!is_trd(&disk_info))
    {
        messagebox(ALERT_ERROR, "Wrong TRD!");
        return;
    }

    wc_print_window(&win_main);
    wc_print_window(&win_files);
    wc_print_window(&win_info);
    wc_print_window(&win_fileinfo);

    init_variables();

    print_cat();
    wc_print_cursor(&win_files);
    print_disk_info();
    print_file_info();

    wc_print(&win_main, 19, 20, "\x06\x06""M""\x07\x07"" - MOVE");
    wc_print(&win_main, 19, 21, "\x06\x06""S""\x07\x07"" - save TRD");
    wc_print(&win_main, 19, 22, "\x06\x06""D""\x07\x07"" - delete file(s)");
    wc_print(&win_main, 19, 23, "\x06\x06""X""\x07\x07"" - extract file(s)");

    wc_marked_cnt = wc_get_marked_file(0, 0);

    if(!wc_marked_cnt)
    {
        browse();
        wc_restore_window(&win_main);
    }
    else
    {
        put_files();
        init_variables();
        print_cat();
        wc_print_cursor(&win_files);
        print_disk_info();
        print_file_info();
        browse();
        wc_restore_window(&win_main);
    }
}

// -------------------------------------------------------------
void create_trd()
{
    infobox("Please wait while TRD creating...");

    memset(cat, 0, sizeof(cat));
    memset(&disk_info, 0, sizeof(disk_info));
    memset(&(u8)disk_info.space9, 0x20, 9);
    memcpy(&(u8)disk_info.disk_title, "DISK 000", 8);
    disk_info.ident = TRD_SIGN;
    disk_info.type = DS_80;
    disk_info.num_free_sec = 2544;
    disk_info.free_trk_next = 1;

    for(u8 i=0; i<64; i++)
    {
        wc_vpage3(i);
        memset((u8*)0xC000, 0, 16384);
    }

    trd_save_cat(cat, &disk_info);

    if (mkfile("TRD.trd", (u32)256*16*2*80))
    {
        messagebox(ALERT_ERROR, "Error has occured!");
        infobox_close();
        return;
    }

    for(u8 i=0; i<64; i++)
    {
        wc_vpage3(i);
        wc_save512((u8*)0xC000, 32);
    }

    infobox_close();
}

// -------------------------------------------------------------
void main()
{
    switch (wc_call_type)
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
        create_trd();
        wc_exit(WC_REREAD_DIR);
        break;

    default:
        wc_exit(WC_REREAD_DIR);
    }
}
