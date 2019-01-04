#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgtrace.h"
#include "util.h"

/*
	 dialogs design

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from TR-DOS file  ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³file:  12345678 C      ³
³start: E000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from TR-DOS sector³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³trk (00-9F): 00        ³
³sec (00-0F): 08        ³
³start: E000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read RAW sectors       ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³cyl (00-4F): 00 side: 0³
³sec (00-0F): 08 num: 01³
³start: E000            ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from host file    ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³file: 12345678.bin     ³
³start: C000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

*/

enum FILEDLG_MODE { FDM_LOAD = 0, FDM_SAVE, FDM_DISASM };

unsigned addr = 0, end = 0xFFFF;
u8 *memdata;
unsigned rw_drive, rw_trk, rw_cyl, rw_tsec, rw_rsec, rw_side;
char fname[20] = "", trdname[9] = "12345678", trdext[2] = "C";

const int file_dlg_x = 6;
const int file_dlg_y = 10;
const int file_dlg_dx = 25;

static void rw_err(const char *msg)
{
	MessageBox(wnd, msg, "Error", MB_OK | MB_ICONERROR);
}

char query_file_addr(const FILEDLG_MODE mode)
{
	filledframe(file_dlg_x, file_dlg_y, file_dlg_dx, 5);
	char ln[64];
	static const char *titles[] = { " Read from binary file   ",
									" Write to binary file    ",
									" Disasm to text file     " };
	tprint(file_dlg_x, file_dlg_y, titles[mode], frm_header);
	tprint(file_dlg_x + 1, file_dlg_y + 2, "file:", fframe_inside);
	sprintf(ln, (mode != FDM_LOAD) ? "start: %04X end: %04X" : "start: %04X", addr, end);
	tprint(file_dlg_x + 1, file_dlg_y + 3, ln, fframe_inside);
	strcpy(str, fname);
	for (;;)
	{
		if (!inputhex(file_dlg_x + 7, file_dlg_y + 2, 16, false))
			return 0;
		if (mode != FDM_LOAD)
			break;
		if (GetFileAttributes(str) != INVALID_FILE_ATTRIBUTES)
			break;
	}
	strcpy(fname, str);
	sprintf(ln, "%-16s", fname);
	fillattr(file_dlg_x + 7, file_dlg_y + 2, 16);
	const unsigned a1 = input4(file_dlg_x + 8, file_dlg_y + 3, addr);
	if (a1 == UINT_MAX)
		return 0;
	addr = a1;
	fillattr(file_dlg_x + 8, file_dlg_y + 3, 4);
	if (mode == FDM_LOAD)
		return 1;
	for (;;)
	{
		const unsigned e1 = input4(file_dlg_x + 18, file_dlg_y + 3, end);
		if (e1 == UINT_MAX)
			return 0;
		if (e1 < addr)
			continue;
		end = e1;
		return 1;
	}
}

void write_mem()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	auto ptr = memdata;
	for (auto a1 = addr; a1 <= end; a1++)
		*cpu.DirectMem(a1) = *ptr++;
}

void read_mem()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	auto ptr = memdata;
	for (auto a1 = addr; a1 <= end; a1++)
		*ptr++ = cpu.DirectRm(a1);
}

char rw_select_drive()
{
	tprint(file_dlg_x + 1, file_dlg_y + 2, "drive:", fframe_inside);
	for (;;) {
		*reinterpret_cast<unsigned*>(str) = 'A' + rw_drive;
		if (!inputhex(file_dlg_x + 8, file_dlg_y + 2, 1, true)) return 0;
		fillattr(file_dlg_x + 8, file_dlg_y + 2, 1);
		const unsigned disk = *str - 'A';
		if (disk > 3) continue;
		if (!comp.wd.fdd[disk].rawdata) continue;
		rw_drive = disk; return 1;
	}
}

char rw_trdos_sectors(FILEDLG_MODE mode)
{
	filledframe(file_dlg_x, file_dlg_y, file_dlg_dx, 7);
	const char *title = (mode == FDM_LOAD) ? " Read from TR-DOS sectors" :
		" Write to TR-DOS sectors ";
	tprint(file_dlg_x, file_dlg_y, title, frm_header);

	char ln[64];

	sprintf(ln, "trk (00-9F): %02X", rw_trk);
	tprint(file_dlg_x + 1, file_dlg_y + 3, ln, fframe_inside);

	sprintf(ln, "sec (00-0F): %02X", rw_tsec);
	tprint(file_dlg_x + 1, file_dlg_y + 4, ln, fframe_inside);

	sprintf(ln, "start: %04X end: %04X", addr, end);
	tprint(file_dlg_x + 1, file_dlg_y + 5, ln, fframe_inside);

	if (!rw_select_drive()) return 0;
	FDD *fdd = &comp.wd.fdd[rw_drive];
	// if (fdd->sides != 2) { rw_err("single-side TR-DOS disks are not supported"); return 0; }

	unsigned t = input2(file_dlg_x + 14, file_dlg_y + 3, rw_trk);
	if (t == UINT_MAX) return 0; else rw_trk = t;
	fillattr(file_dlg_x + 14, file_dlg_y + 3, 2);

	t = input2(file_dlg_x + 14, file_dlg_y + 4, rw_tsec);
	if (t == UINT_MAX) return 0; else rw_tsec = t;
	fillattr(file_dlg_x + 14, file_dlg_y + 4, 2);

	t = input4(file_dlg_x + 8, file_dlg_y + 5, addr);
	if (t == UINT_MAX) return 0; else addr = t;
	fillattr(file_dlg_x + 8, file_dlg_y + 5, 4);

	for (;;) {
		t = input4(file_dlg_x + 18, file_dlg_y + 5, end);
		fillattr(file_dlg_x + 18, file_dlg_y + 5, 4);
		if (t == UINT_MAX) return 0;
		if (t < addr) continue;
		end = t; break;
	}

	unsigned offset = 0;
	if (mode == FDM_SAVE) read_mem();

	unsigned trk = rw_trk, sec = rw_tsec;

	TRKCACHE tc; tc.clear();
	for (;;) {
		int left = end + 1 - (addr + offset);
		if (left <= 0) break;
		if (left > 0x100) left = 0x100;

		tc.seek(fdd, trk / 2, trk & 1, LOAD_SECTORS);
		if (!tc.trkd) { sprintf(ln, "track #%02X not found", trk); rw_err(ln); break; }
		const SECHDR *hdr = tc.get_sector(sec + 1);
		if (!hdr || !hdr->data) { sprintf(ln, "track #%02X, sector #%02X not found", trk, sec); rw_err(ln); break; }
		if (hdr->l != 1) { sprintf(ln, "track #%02X, sector #%02X is not 256 bytes", trk, sec); rw_err(ln); break; }

		if (mode == FDM_LOAD) {
			memcpy(memdata + offset, hdr->data, left);
		}
		else {
			tc.write_sector(sec + 1, memdata + offset);
			fdd->optype |= 1;
		}

		offset += left;
		if (++sec == 0x10) trk++, sec = 0;
	}

	end = addr + offset - 1;
	if (mode == FDM_LOAD) write_mem();
	return 1;
}

char wr_trdos_file()
{
	filledframe(file_dlg_x, file_dlg_y, file_dlg_dx, 6);
	const char *title = " Write to TR-DOS file    ";
	tprint(file_dlg_x, file_dlg_y, title, frm_header);

	char ln[64];

	sprintf(ln, "file:  %-8s %s", trdname, trdext);
	tprint(file_dlg_x + 1, file_dlg_y + 3, ln, fframe_inside);

	sprintf(ln, "start: %04X end: %04X", addr, end);
	tprint(file_dlg_x + 1, file_dlg_y + 4, ln, fframe_inside);

	if (!rw_select_drive()) return 0;
	auto fdd = &comp.wd.fdd[rw_drive];
	// if (fdd->sides != 2) { rw_err("single-side TR-DOS disks are not supported"); return 0; }

	strcpy(str, trdname);
	if (!inputhex(file_dlg_x + 8, file_dlg_y + 3, 8, false)) return 0;
	fillattr(file_dlg_x + 8, file_dlg_y + 3, 8);
	strcpy(trdname, str);
	for (int ptr = strlen(trdname); ptr < 8; trdname[ptr++] = ' ') {}
	trdname[8] = 0;

	strcpy(str, trdext);
	if (!inputhex(file_dlg_x + 17, file_dlg_y + 3, 1, false)) return 0;
	fillattr(file_dlg_x + 17, file_dlg_y + 3, 1);
	trdext[0] = str[0]; trdext[1] = 0;

	unsigned t = input4(file_dlg_x + 8, file_dlg_y + 4, addr);
	if (t == UINT_MAX) return 0; else addr = t;
	fillattr(file_dlg_x + 8, file_dlg_y + 4, 4);

	for (;;) {
		t = input4(file_dlg_x + 18, file_dlg_y + 4, end);
		fillattr(file_dlg_x + 18, file_dlg_y + 4, 4);
		if (t == UINT_MAX) return 0;
		if (t < addr) continue;
		end = t; break;
	}

	read_mem();

	u8 hdr[16];
	memcpy(hdr, trdname, 8);
	hdr[8] = *trdext;

	const unsigned sz = end - addr + 1;
	*reinterpret_cast<u16*>(hdr + 9) = addr;
	*reinterpret_cast<u16*>(hdr + 11) = sz;
	hdr[13] = u8(align_by(sz, 0x100) / 0x100); // sector size

	fdd->optype |= 1;
	if (!fdd->addfile(hdr, memdata)) { rw_err("write error"); return 0; }
	return 1;
}

void mon_load()
{
	static menuitem items[] =
	{ { "from binary file", menuitem::left },
	  { "from TR-DOS file", menuitem::left },
	  { "from TR-DOS sectors", menuitem::left },
	  { "from raw sectors of FDD image", menuitem::left } };
	static menudef menu = { items, 3, "Load data to memory...", 0 };

	if (!handle_menu(&menu))
		return;

	u8 bf[0x10000];
	memdata = bf;

	switch (menu.pos)
	{
	case 0:
	{
		if (!query_file_addr(FDM_LOAD))
			return;
		const auto ff = fopen(fname, "rb");
		if (!ff)
			return;
		const auto sz = fread(bf, 1, sizeof(bf), ff);
		fclose(ff);
		end = addr + sz - 1;
		end &= 0xFFFF;
		write_mem();
		return;
	}

	case 1:
	{
		rw_err("file selector\r\nis not implemented");
		return;
	}

	case 2:
	{
		rw_trdos_sectors(FDM_LOAD);
		return;
	}

	case 3:
	{
		return;
	}
	default: ;
	}
}

void mon_save()
{
	static menuitem items[] =
	{ { "to binary file", menuitem::left },
	  { "to TR-DOS file", menuitem::left },
	  { "to TR-DOS sectors", menuitem::left },
	  { "as Z80 disassembly", menuitem::left },
	  { "to raw sectors of FDD image", menuitem::flags_t(menuitem::left | menuitem::disabled) } };
	static menudef menu = { items, 4, "Save data from memory...", 0 };

	if (!handle_menu(&menu)) return;

	u8 bf[0x10000]; memdata = bf;

	switch (menu.pos)
	{
	case 0:
	{
		if (!query_file_addr(FDM_SAVE)) return;
		read_mem();
		const auto ff = fopen(fname, "wb");
		if (!ff) return;
		fwrite(bf, 1, end + 1 - addr, ff);
		fclose(ff);
		return;
	}

	case 1:
	{
		wr_trdos_file();
		return;
	}

	case 2:
	{
		rw_trdos_sectors(FDM_SAVE);
		return;
	}

	case 3:
	{
		if (!query_file_addr(FDM_DISASM)) return;
		const auto ff = fopen(fname, "wt");
		if (!ff) return;
		for (auto a = addr; a <= end; ) {
			//            char line[64]; //Alone Coder 0.36.7
			char line[16 + 129]; //Alone Coder 0.36.7
			a += disasm_line(a, line);
			fprintf(ff, "%s\n", line);
		}
		fclose(ff);
		return;
	}

	case 4:
	{
		return;
	}
	default: ;
	}
}
