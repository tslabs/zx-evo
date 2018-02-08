/*----------------------------------------------------------------------------/
/  Petit FatFs - FAT file system module  R0.03                (C)ChaN, 2014
/-----------------------------------------------------------------------------/
/ Petit FatFs module is a generic FAT file system module for small embedded
/ systems. This is a free software that opened for education, research and
/ commercial developments under license policy of following trems.
/
/  Copyright (C) 2014, ChaN, all right reserved.
/
/ * The Petit FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Jun 15,'09  R0.01a  First release.
/
/ Dec 14,'09  R0.02   Added multiple code page support.
/                     Added write funciton.
/                     Changed stream read mode interface.
/ Dec 07,'10  R0.02a  Added some configuration options.
/                     Fixed fails to open objects with DBCS character.

/ Jun 10,'14  R0.03   Separated out configuration options to pffconf.h.
/                     Added _FS_FAT16 option.
/----------------------------------------------------------------------------*/

#include "defs.h"
#include "pff.h"    /* Petit FatFs configurations and declarations */
#include "diskio.h"    /* Declarations of low level disk I/O functions */

FATFS *FatFs;  /* Pointer to the file system object (logical drive) */

/*--------------------------------------------------------------------------

   Private Functions

---------------------------------------------------------------------------*/

/* Fill memory */
void mem_set (void* dst, int val, int cnt) {
  char *d = (char*)dst;
  while (cnt--) *d++ = (char)val;
}

/* Compare memory to memory */
int mem_cmp (const void* dst, const void* src, int cnt) {
  const char *d = (const char *)dst, *s = (const char *)src;
  int r = 0;
  while (cnt-- && (r = *d++ - *s++) == 0) ;
  return r;
}

/*-----------------------------------------------------------------------*/
/* FAT access - Read value of a FAT entry                                */
/*-----------------------------------------------------------------------*/
CLUST get_fat (  /* 1:IO error, Else:Cluster status */
  CLUST clst  /* Cluster# to get the link information */
)
{
  BYTE buf[4];
  FATFS *fs = FatFs;

  if (clst < 2 || clst >= fs->n_fatent)  /* Range check */
    return 1;

  switch (fs->fs_type) {
#if _FS_FAT12
  case FS_FAT12 : {
    UINT wc, bc, ofs;

    bc = (UINT)clst; bc += bc / 2;
    ofs = bc & 511; bc >> 9;
    if (ofs != 511) {
      if (disk_readp(buf, fs->fatbase + bc, ofs, 2)) break;
    } else {
      if (disk_readp(buf, fs->fatbase + bc, 511, 1)) break;
      if (disk_readp(buf+1, fs->fatbase + bc + 1, 0, 1)) break;
    }
    wc = LD_WORD(buf);
    return (clst & 1) ? (wc >> 4) : (wc & 0xFFF);
  }
#endif
#if _FS_FAT16
  case FS_FAT16 :
    if (disk_readp(buf, fs->fatbase + (clst >> 8), ((UINT)clst & 255) * 2, 2)) break;
    return LD_WORD(buf);
#endif
#if _FS_FAT32
  case FS_FAT32 :
    if (disk_readp(buf, fs->fatbase + (clst >> 7), ((UINT)clst & 127) * 4, 4)) break;
    return LD_DWORD(buf) & 0x0FFFFFFF;
#endif
  }

  return 1;  /* An error occured at the disk I/O layer */
}

/*-----------------------------------------------------------------------*/
/* Get sector# from cluster# / Get cluster field from directory entry    */
/*-----------------------------------------------------------------------*/
DWORD clust2sect (  /* !=0: Sector number, 0: Failed - invalid cluster# */
  CLUST clst    /* Cluster# to be converted */
)
{
  FATFS *fs = FatFs;


  clst -= 2;
  if (clst >= (fs->n_fatent - 2)) return 0;    /* Invalid cluster# */
  return (DWORD)clst * fs->csize + fs->database;
}

CLUST get_clust (
  BYTE* dir    /* Pointer to directory entry */
)
{
  FATFS *fs = FatFs;
  CLUST clst = 0;


  if (_FS_32ONLY || (_FS_FAT32 && (fs->fs_type == FS_FAT32))) {
    clst = LD_WORD(dir+DIR_FstClusHI);
    clst <<= 16;
  }
  clst |= LD_WORD(dir+DIR_FstClusLO);

  return clst;
}

/*-----------------------------------------------------------------------*/
/* Directory handling - Rewind directory index                           */
/*-----------------------------------------------------------------------*/
FRESULT dir_rewind (
  DIR *dj      /* Pointer to directory object */
)
{
  CLUST clst;
  FATFS *fs = FatFs;


  dj->index = 0;
  clst = dj->sclust;
  if (clst == 1 || clst >= fs->n_fatent)  /* Check start cluster range */
    return FR_DISK_ERR;
  if (_FS_FAT32 && !clst && (_FS_32ONLY || fs->fs_type == FS_FAT32))  /* Replace cluster# 0 with root cluster# if in FAT32 */
    clst = (CLUST)fs->dirbase;
  dj->clust = clst;            /* Current cluster */
  dj->sect = (_FS_32ONLY || clst) ? clust2sect(clst) : fs->dirbase;  /* Current sector */

  return FR_OK;  /* Seek succeeded */
}

/*-----------------------------------------------------------------------*/
/* Directory handling - Move directory index next                        */
/*-----------------------------------------------------------------------*/
FRESULT dir_next (  /* FR_OK:Succeeded, FR_NO_FILE:End of table */
  DIR *dj      /* Pointer to directory object */
)
{
  CLUST clst;
  WORD i;
  FATFS *fs = FatFs;


  i = dj->index + 1;
  if (!i || !dj->sect)  /* Report EOT when index has reached 65535 */
    return FR_NO_FILE;

  if (!(i % 16)) {    /* Sector changed? */
    dj->sect++;      /* Next sector */

    if (dj->clust == 0) {  /* Static table */
      if (i >= fs->n_rootdir)  /* Report EOT when end of table */
        return FR_NO_FILE;
    }
    else {          /* Dynamic table */
      if (((i / 16) & (fs->csize - 1)) == 0) {  /* Cluster changed? */
        clst = get_fat(dj->clust);    /* Get next cluster */
        if (clst <= 1) return FR_DISK_ERR;
        if (clst >= fs->n_fatent)    /* When it reached end of dynamic table */
          return FR_NO_FILE;      /* Report EOT */
        dj->clust = clst;        /* Initialize data for new cluster */
        dj->sect = clust2sect(clst);
      }
    }
  }

  dj->index = i;

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Directory handling - Find an object in the directory                  */
/*-----------------------------------------------------------------------*/
FRESULT dir_find (
  DIR *dj,    /* Pointer to the directory object linked to the file name */
  BYTE *dir    /* 32-byte working buffer */
)
{
  FRESULT res;
  BYTE c;


  res = dir_rewind(dj);      /* Rewind directory object */
  if (res != FR_OK) return res;

  do {
    res = disk_readp(dir, dj->sect, (dj->index % 16) * 32, 32)  /* Read an entry */
      ? FR_DISK_ERR : FR_OK;
    if (res != FR_OK) break;
    c = dir[DIR_Name];  /* First character */
    if (c == 0) { res = FR_NO_FILE; break; }  /* Reached to end of table */
    if (!(dir[DIR_Attr] & AM_VOL) && !mem_cmp(dir, dj->fn, 11)) /* Is it a valid entry? */
      break;
    res = dir_next(dj);          /* Next entry */
  } while (res == FR_OK);

  return res;
}

/*-----------------------------------------------------------------------*/
/* Read an object from the directory                                     */
/*-----------------------------------------------------------------------*/
FRESULT dir_read (
  DIR *dj,    /* Pointer to the directory object to store read object name */
  BYTE *dir    /* 32-byte working buffer */
)
{
  FRESULT res;
  BYTE a, c;


  res = FR_NO_FILE;
  while (dj->sect) {
    res = disk_readp(dir, dj->sect, (dj->index % 16) * 32, 32)  /* Read an entry */
      ? FR_DISK_ERR : FR_OK;
    if (res != FR_OK) break;
    c = dir[DIR_Name];
    if (c == 0) { res = FR_NO_FILE; break; }  /* Reached to end of table */
    a = dir[DIR_Attr] & AM_MASK;
    if (c != 0xE5 && c != '.' && !(a & AM_VOL))  /* Is it a valid entry? */
      break;
    res = dir_next(dj);      /* Next entry */
    if (res != FR_OK) break;
  }

  if (res != FR_OK) dj->sect = 0;

  return res;
}

/*-----------------------------------------------------------------------*/
/* Pick a segment and create the object name in directory form           */
/*-----------------------------------------------------------------------*/
FRESULT create_name (
  DIR *dj,      /* Pointer to the directory object */
  const char **path  /* Pointer to pointer to the segment in the path string */
)
{
  BYTE c, ni, si, i, *sfn;
  const char *p;

  /* Create file name in directory form */
  sfn = dj->fn;
  mem_set(sfn, ' ', 11);
  si = i = 0; ni = 8;
  p = *path;
  for (;;)
  {
    c = p[si++];
    if (c <= ' ' || c == '/') break;  /* Break on end of segment */
    if (c == '.' || i >= ni)
    {
      if (ni != 8 || c != '.') break;
      i = 8; ni = 11;
      continue;
    }
    sfn[i++] = c;
  }
  *path = &p[si];            /* Rerurn pointer to the next segment */

  sfn[11] = (c <= ' ') ? 1 : 0;    /* Set last segment flag if end of path */

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Get file information from directory entry                             */
/*-----------------------------------------------------------------------*/
void get_fileinfo (    /* No return code */
  DIR *dj,      /* Pointer to the directory object */
  BYTE *dir,      /* 32-byte working buffer */
  FILINFO *fno     /* Pointer to store the file information */
)
{
  BYTE i, c;
  char *p;


  p = fno->fname;
  if (dj->sect) {
    for (i = 0; i < 8; i++) {  /* Copy file name body */
      c = dir[i];
      if (c == ' ') break;
      if (c == 0x05) c = 0xE5;
      *p++ = c;
    }
    if (dir[8] != ' ') {    /* Copy file name extension */
      *p++ = '.';
      for (i = 8; i < 11; i++) {
        c = dir[i];
        if (c == ' ') break;
        *p++ = c;
      }
    }
    fno->fattrib = dir[DIR_Attr];        /* Attribute */
    fno->fsize = LD_DWORD(dir+DIR_FileSize);  /* Size */
    fno->fdate = LD_WORD(dir+DIR_WrtDate);    /* Date */
    fno->ftime = LD_WORD(dir+DIR_WrtTime);    /* Time */
  }

  *p = 0;
}

/*-----------------------------------------------------------------------*/
/* Follow a file path                                                    */
/*-----------------------------------------------------------------------*/
FRESULT follow_path (  /* FR_OK(0): successful, !=0: error code */
  DIR *dj,      /* Directory object to return last directory and found object */
  BYTE *dir,      /* 32-byte working buffer */
  const char *path  /* Full-path string to find a file or directory */
)
{
  FRESULT res;


  while (*path == ' ') path++;    /* Strip leading spaces */
  if (*path == '/') path++;      /* Strip heading separator if exist */
  dj->sclust = 0;            /* Set start directory (always root dir) */

  if ((BYTE)*path < ' ') {      /* Null path means the root directory */
    res = dir_rewind(dj);
    dir[0] = 0;

  } else {              /* Follow path */
    for (;;) {
      res = create_name(dj, &path);  /* Get a segment */
      if (res != FR_OK) break;
      res = dir_find(dj, dir);    /* Find it */
      if (res != FR_OK) break;    /* Could not find the object */
      if (dj->fn[11]) break;      /* Last segment match. Function completed. */
      if (!(dir[DIR_Attr] & AM_DIR)) { /* Cannot follow path because it is a file */
        res = FR_NO_FILE; break;
      }
      dj->sclust = get_clust(dir);  /* Follow next */
    }
  }

  return res;
}

/*-----------------------------------------------------------------------*/
/* Check a sector if it is an FAT boot record                            */
/*-----------------------------------------------------------------------*/
BYTE check_fs (  /* 0:The FAT boot record, 1:Valid boot record but not an FAT, 2:Not a boot record, 3:Error */
  BYTE *buf,  /* Working buffer */
  DWORD sect  /* Sector# (lba) to check if it is an FAT boot record or not */
)
{
  if (disk_readp(buf, sect, 510, 2))    /* Read the boot record */
    return 3;
  if (LD_WORD(buf) != 0xAA55)        /* Check record signature */
    return 2;

  if (!_FS_32ONLY && !disk_readp(buf, sect, BS_FilSysType, 2) && LD_WORD(buf) == 0x4146)  /* Check FAT12/16 */
    return 0;
  if (_FS_FAT32 && !disk_readp(buf, sect, BS_FilSysType32, 2) && LD_WORD(buf) == 0x4146)  /* Check FAT32 */
    return 0;
  return 1;
}

/*--------------------------------------------------------------------------

   Public Functions

--------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Mount/Unmount a Locical Drive                                         */
/*-----------------------------------------------------------------------*/
FRESULT pf_mount (
  FATFS *fs    /* Pointer to new file system object */
)
{
  BYTE fmt, buf[36];
  DWORD bsect, fsize, tsect, mclst;


  FatFs = 0;

  if (disk_initialize())  /* Check if the drive is ready or not */
    return FR_NOT_READY;

  /* Search FAT partition on the drive */
  bsect = 0;
  fmt = check_fs(buf, bsect);      /* Check sector 0 as an SFD format */
  if (fmt == 1) {            /* Not an FAT boot record, it may be FDISK format */
    /* Check a partition listed in top of the partition table */
    if (disk_readp(buf, bsect, MBR_Table, 16)) {  /* 1st partition entry */
      fmt = 3;
    } else {
      if (buf[4]) {          /* Is the partition existing? */
        bsect = LD_DWORD(&buf[8]);  /* Partition offset in LBA */
        fmt = check_fs(buf, bsect);  /* Check the partition */
      }
    }
  }
  if (fmt == 3) return FR_DISK_ERR;
  if (fmt) return FR_NO_FILESYSTEM;  /* No valid FAT patition is found */

  /* Initialize the file system object */
  if (disk_readp(buf, bsect, 13, sizeof (buf))) return FR_DISK_ERR;

  fsize = LD_WORD(buf+BPB_FATSz16-13);        /* Number of sectors per FAT */
  if (!fsize) fsize = LD_DWORD(buf+BPB_FATSz32-13);

  fsize *= buf[BPB_NumFATs-13];            /* Number of sectors in FAT area */
  fs->fatbase = bsect + LD_WORD(buf+BPB_RsvdSecCnt-13); /* FAT start sector (lba) */
  fs->csize = buf[BPB_SecPerClus-13];          /* Number of sectors per cluster */
  fs->n_rootdir = LD_WORD(buf+BPB_RootEntCnt-13);    /* Nmuber of root directory entries */
  tsect = LD_WORD(buf+BPB_TotSec16-13);        /* Number of sectors on the file system */
  if (!tsect) tsect = LD_DWORD(buf+BPB_TotSec32-13);
  mclst = (tsect            /* Last cluster# + 1 */
    - LD_WORD(buf+BPB_RsvdSecCnt-13) - fsize - fs->n_rootdir / 16
    ) / fs->csize + 2;
  fs->n_fatent = (CLUST)mclst;

  fmt = 0;              /* Determine the FAT sub type */
  if (_FS_FAT12 && mclst < 0xFF7)
    fmt = FS_FAT12;
  if (_FS_FAT16 && mclst >= 0xFF8 && mclst < 0xFFF7)
    fmt = FS_FAT16;
  if (_FS_FAT32 && mclst >= 0xFFF7)
    fmt = FS_FAT32;
  if (!fmt) return FR_NO_FILESYSTEM;
  fs->fs_type = fmt;

  if (_FS_32ONLY || (_FS_FAT32 && fmt == FS_FAT32))
    fs->dirbase = LD_DWORD(buf+(BPB_RootClus-13));  /* Root directory start cluster */
  else
    fs->dirbase = fs->fatbase + fsize;        /* Root directory start sector (lba) */
  fs->database = fs->fatbase + fsize + fs->n_rootdir / 16;  /* Data start sector (lba) */

  fs->flag = 0;
  FatFs = fs;

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Open or Create a File                                                 */
/*-----------------------------------------------------------------------*/
FRESULT pf_open (
  const char *path  /* Pointer to the file name */
)
{
  FRESULT res;
  DIR dj;
  BYTE sp[12], dir[32];
  FATFS *fs = FatFs;


  if (!fs) return FR_NOT_ENABLED;    /* Check file system */

  fs->flag = 0;
  dj.fn = sp;
  res = follow_path(&dj, dir, path);  /* Follow the file path */
  if (res != FR_OK) return res;    /* Follow failed */
  if (!dir[0] || (dir[DIR_Attr] & AM_DIR))  /* It is a directory */
    return FR_NO_FILE;

  fs->org_clust = get_clust(dir);    /* File start cluster */
  fs->fsize = LD_DWORD(dir+DIR_FileSize);  /* File size */
  fs->fptr = 0;            /* File pointer */
  fs->flag = FA_OPENED;

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Read File                                                             */
/*-----------------------------------------------------------------------*/
FRESULT pf_read (
  void* buff,    /* Pointer to the read buffer (NULL:Forward data to the stream)*/
  UINT btr,    /* Number of bytes to read */
  UINT* br    /* Pointer to number of bytes read */
)
{
  DRESULT dr;
  CLUST clst;
  DWORD sect, remain;
  UINT rcnt;
  BYTE cs, *rbuff = buff;
  FATFS *fs = FatFs;

  *br = 0;
  if (!fs) return FR_NOT_ENABLED;    /* Check file system */

  if (!(fs->flag & FA_OPENED))    /* Check if opened */
    return FR_NOT_OPENED;

  remain = fs->fsize - fs->fptr;
  if (btr > remain) btr = (UINT)remain;      /* Truncate btr by remaining bytes */

  while (btr)  /* Repeat until all data transferred */
  {
    if (!(fs->fptr & 511))  /* On the sector boundary? */
    {
      cs = (BYTE)((fs->fptr >> 9) & (fs->csize - 1));  /* Sector offset in the cluster */
      if (!cs)  /* On the cluster boundary? */
      {
        if (fs->fptr == 0)          /* On the top of the file? */
          clst = fs->org_clust;
        else
          clst = get_fat(fs->curr_clust);

        if (clst <= 1) ABORT(FR_DISK_ERR);

        fs->curr_clust = clst;        /* Update current cluster */
      }

      sect = clust2sect(fs->curr_clust);    /* Get current sector */
      if (!sect) ABORT(FR_DISK_ERR);
      fs->dsect = sect + cs;
    }

    {
      u16 csz = min(fs->csize, 64) << 9;        // cluster size, but not more than 32kB
      u16 crz = csz - (fs->fptr & (csz - 1));   // remainder to the end of cluster
      rcnt = min(btr, crz);                     // size to read from bulk
    }

    // rcnt = min (btr, 512 - ((UINT)fs->fptr & 511));  // uncomment this for 1 sector per read (and comment code above)

    dr = disk_readp(!buff ? 0 : rbuff, fs->dsect, (UINT)fs->fptr & 511, rcnt);
    if (dr) ABORT(FR_DISK_ERR);

    /* Update pointers and counters */
    fs->fptr += rcnt;
    rbuff += rcnt;
    btr -= rcnt;
    *br += rcnt;
  }

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Write File                                                            */
/*-----------------------------------------------------------------------*/
FRESULT pf_write (
  const void* buff,  /* Pointer to the data to be written */
  UINT btw,      /* Number of bytes to write (0:Finalize the current write operation) */
  UINT* bw      /* Pointer to number of bytes written */
)
{
  CLUST clst;
  DWORD sect, remain;
  const BYTE *p = buff;
  BYTE cs;
  UINT wcnt;
  FATFS *fs = FatFs;


  *bw = 0;
  if (!fs) return FR_NOT_ENABLED;    /* Check file system */
  if (!(fs->flag & FA_OPENED))    /* Check if opened */
    return FR_NOT_OPENED;

  if (!btw) {    /* Finalize request */
    if ((fs->flag & FA__WIP) && disk_writep(0, 0)) ABORT(FR_DISK_ERR);
    fs->flag &= ~FA__WIP;
    return FR_OK;
  } else {    /* Write data request */
    if (!(fs->flag & FA__WIP))    /* Round-down fptr to the sector boundary */
      fs->fptr &= 0xFFFFFE00;
  }
  remain = fs->fsize - fs->fptr;
  if (btw > remain) btw = (UINT)remain;      /* Truncate btw by remaining bytes */

  while (btw)  {                  /* Repeat until all data transferred */
    if (!((UINT)fs->fptr % 511)) {      /* On the sector boundary? */
      cs = ((BYTE)(fs->fptr >> 9) & (fs->csize - 1));  /* Sector offset in the cluster */
      if (!cs) {                /* On the cluster boundary? */
        if (fs->fptr == 0)          /* On the top of the file? */
          clst = fs->org_clust;
        else
          clst = get_fat(fs->curr_clust);
        if (clst <= 1) ABORT(FR_DISK_ERR);
        fs->curr_clust = clst;        /* Update current cluster */
      }
      sect = clust2sect(fs->curr_clust);    /* Get current sector */
      if (!sect) ABORT(FR_DISK_ERR);
      fs->dsect = sect + cs;
      if (disk_writep(0, fs->dsect)) ABORT(FR_DISK_ERR);  /* Initiate a sector write operation */
      fs->flag |= FA__WIP;
    }
    wcnt = 512 - ((UINT)fs->fptr & 511);      /* Number of bytes to write to the sector */
    if (wcnt > btw) wcnt = btw;
    if (disk_writep(p, wcnt)) ABORT(FR_DISK_ERR);  /* Send data to the sector */
    fs->fptr += wcnt; p += wcnt;        /* Update pointers and counters */
    btw -= wcnt; *bw += wcnt;
    if (!((UINT)fs->fptr & 511)) {
      if (disk_writep(0, 0)) ABORT(FR_DISK_ERR);  /* Finalize the currtent secter write operation */
      fs->flag &= ~FA__WIP;
    }
  }

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Seek File R/W Pointer                                                 */
/*-----------------------------------------------------------------------*/
FRESULT pf_lseek (
  DWORD ofs    /* File pointer from top of file */
)
{
  CLUST clst;
  DWORD bcs, sect, ifptr;
  FATFS *fs = FatFs;


  if (!fs) return FR_NOT_ENABLED;    /* Check file system */
  if (!(fs->flag & FA_OPENED))    /* Check if opened */
      return FR_NOT_OPENED;

  if (ofs > fs->fsize) ofs = fs->fsize;  /* Clip offset with the file size */
  ifptr = fs->fptr;
  fs->fptr = 0;
  if (ofs > 0) {
    bcs = (DWORD)fs->csize << 9;  /* Cluster size (byte) */
    if (ifptr > 0 &&
      (ofs - 1) / bcs >= (ifptr - 1) / bcs) {  /* When seek to same or following cluster, */
      fs->fptr = (ifptr - 1) & ~(bcs - 1);  /* start from the current cluster */
      ofs -= fs->fptr;
      clst = fs->curr_clust;
    } else {              /* When seek to back cluster, */
      clst = fs->org_clust;      /* start from the first cluster */
      fs->curr_clust = clst;
    }
    while (ofs > bcs) {        /* Cluster following loop */
      clst = get_fat(clst);    /* Follow cluster chain */
      if (clst <= 1 || clst >= fs->n_fatent) ABORT(FR_DISK_ERR);
      fs->curr_clust = clst;
      fs->fptr += bcs;
      ofs -= bcs;
    }
    fs->fptr += ofs;
    sect = clust2sect(clst);    /* Current sector */
    if (!sect) ABORT(FR_DISK_ERR);
    fs->dsect = sect + ((fs->fptr >> 9) & (fs->csize - 1));
  }

  return FR_OK;
}

/*-----------------------------------------------------------------------*/
/* Create a Directroy Object                                             */
/*-----------------------------------------------------------------------*/
FRESULT pf_opendir (
  DIR *dj,      /* Pointer to directory object to create */
  const char *path  /* Pointer to the directory path */
)
{
  FRESULT res;
  BYTE sp[12], dir[32];
  FATFS *fs = FatFs;


  if (!fs) {        /* Check file system */
    res = FR_NOT_ENABLED;
  } else {
    dj->fn = sp;
    res = follow_path(dj, dir, path);    /* Follow the path to the directory */
    if (res == FR_OK) {            /* Follow completed */
      if (dir[0]) {            /* It is not the root dir */
        if (dir[DIR_Attr] & AM_DIR)    /* The object is a directory */
          dj->sclust = get_clust(dir);
        else              /* The object is not a directory */
          res = FR_NO_FILE;
      }
      if (res == FR_OK)
        res = dir_rewind(dj);      /* Rewind dir */
    }
  }

  return res;
}

/*-----------------------------------------------------------------------*/
/* Read Directory Entry in Sequense                                      */
/*-----------------------------------------------------------------------*/
FRESULT pf_readdir (
  DIR *dj,      /* Pointer to the open directory object */
  FILINFO *fno    /* Pointer to file information to return */
)
{
  FRESULT res;
  BYTE sp[12], dir[32];
  FATFS *fs = FatFs;


  if (!fs) {        /* Check file system */
    res = FR_NOT_ENABLED;
  } else {
    dj->fn = sp;
    if (!fno) {
      res = dir_rewind(dj);
    } else {
      res = dir_read(dj, dir);  /* Get current directory item */
      if (res == FR_NO_FILE) res = FR_OK;
      if (res == FR_OK) {        /* A valid entry is found */
        get_fileinfo(dj, dir, fno);  /* Get the object information */
        res = dir_next(dj);      /* Increment read index for next */
        if (res == FR_NO_FILE) res = FR_OK;
      }
    }
  }

  return res;
}
