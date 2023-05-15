#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "snapshot.h"
#include "tape.h"
#include "hard/memory.h"
#include "emulator/ui/opendlg.h"
#include "draw.h"
#include "config.h"
#include "hard/cpu/z80main.h"
#include "util.h"
#include "depack.h"
#include "hard/tsconf.h"
#include "emulator/debugger/dbglabls.h"
#include <ctime>

using namespace Gdiplus;
ULONG_PTR gdiplus_token = 0;
CLSID clsid_png_encoder = GUID_NULL;
CLSID clsid_gif_encoder = GUID_NULL;

int read_sp();
int read_sna48();
int read_sna128();
int read_spg();
int read_z80();
int write_sna(FILE*);
int load_arc(char* fname);

bool gdiplus_startup()
{
	const GdiplusStartupInput gdiplus_startup_input;
	if (GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr) == Ok)
	{
		UINT num, size;
		GetImageEncodersSize(&num, &size);
		if (num)
		{
			const auto info = static_cast<ImageCodecInfo*>(malloc(size));
			if (GetImageEncoders(num, size, info) == Ok)
			{
				for (UINT i = 0; i < num; i++)
				{
					if (info[i].FormatID == ImageFormatPNG)
						clsid_png_encoder = info[i].Clsid;
					else if (info[i].FormatID == ImageFormatGIF)
						clsid_gif_encoder = info[i].Clsid;
				}
				free(info);
				return true;
			}
			free(info);
		}
	}
	return false;
}

void gdiplus_shutdown()
{
	clsid_png_encoder = clsid_gif_encoder = GUID_NULL;
	if (gdiplus_token) GdiplusShutdown(gdiplus_token);
}

u8 what_is(char* filename)
{
	FILE* ff = fopen(filename, "rb");
	if (!ff) return snNOFILE;
	snapsize = fread(snbuf, 1, sizeof snbuf, ff);
	fclose(ff);
	if (snapsize == sizeof snbuf) return snTOOLARGE;
	u8 type = snUNKNOWN;
	char* ptr = strrchr(filename, '.');
	unsigned ext = ptr ? (*(int*)(ptr + 1) | 0x20202020) : 0;
	if (snapsize < 32) return type;

	if (snapsize == 131103 || snapsize == 147487) type = snSNA_128;
	if (snapsize == 49179) type = snSNA_48;
	if (ext == WORD4('t', 'a', 'p', ' ')) type = snTAP;
	if (ext == WORD4('z', '8', '0', ' ')) type = snZ80;
	if (ext == WORD4('s', 'p', 'g', ' ')) type = snSPG;
	if (conf.trdos_present)
	{
		if (!snbuf[13] && snbuf[14] && (int)snapsize == snbuf[14] * 256 + 17) type = snHOB;
		if (snapsize >= 8192 && !(snapsize & 0xFF) && ext == WORD4('t', 'r', 'd', ' ')) type = snTRD;

		if (snapsize >= 8192 && ext == WORD4('i', 's', 'd', ' '))
			type = snISD;

		if (snapsize >= 8192 && ext == WORD4('p', 'r', 'o', ' '))
			type = snPRO;

		if (!memcmp(snbuf, "SINCLAIR", 8))
		{
			const unsigned nfiles = snbuf[8];
			unsigned nsec = 0;
			for (unsigned i = 0; i < nfiles; i++)
			{
				nsec += snbuf[9 + 14 * i + 13];
			}

			if (snapsize >= 9 + nfiles * 14 + nsec * 0x100)
				type = snSCL;
		}
		if (!memcmp(snbuf, "FDI", 3) && *(u16*)(snbuf + 4) <= MAX_CYLS && *(u16*)(snbuf + 6) <= 2) type = snFDI;
		if (((*(short*)snbuf | 0x2020) == WORD2('t', 'd')) && snbuf[4] >= 10 && snbuf[4] <= 21 && snbuf[9] <= 2) type = snTD0;
		if (*(unsigned*)snbuf == WORD4('U', 'D', 'I', '!') && *(unsigned*)(snbuf + 4) == snapsize - 4 && snbuf[9] < MAX_CYLS && snbuf[10] < 2 && !snbuf[8]) type = snUDI;
	}
	if (snapsize > 10 && !memcmp(snbuf, "ZXTape!\x1A", 8)) type = snTZX;
	if (snapsize > 0x22 && !memcmp(snbuf, "Compressed Square Wave\x1A", 23)) type = snCSW;
	if (*(u16*)snbuf == WORD2('S', 'P') && *(u16*)(snbuf + 2) + 0x26 == static_cast<int>(snapsize)) type = snSP;
	return type;
}

int loadsnap(char* filename)
{
	if (load_arc(filename))
		return 1;

	invalidate_ts_cache();

	const u8 type = what_is(filename);

	if (type >= snHOB)
	{

		if (trd_toload == -1)
		{
			int last = -1;
			for (int i = 0; i < 4; i++) if (trd_loaded[i]) last = i;
			trd_toload = (last == -1) ? 0 : ((type == snHOB) ? last : (last + 1) & 3);
		}

		for (unsigned k = 0; k < 4; k++)
		{
			if (k != trd_toload && !stricmp(comp.wd.fdd[k].name, filename))
			{
				static char err[] = "This disk image is already loaded to drive X:\n"
					"Do you still want to load it to drive Y:?";
				err[43] = k + 'A';
				err[84] = trd_toload + 'A';
				if (MessageBox(GetForegroundWindow(), err, "Warning", MB_YESNO | MB_ICONWARNING) == IDNO) return 1;
			}
		}

		FDD* drive = comp.wd.fdd + trd_toload;
		if (!drive->test())
			return 0;
		int ok = drive->read(type);
		if (ok)
		{
			if (*conf.appendboot)
				drive->addboot();
			strcpy(drive->name, filename);
			if (GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY)
				conf.trdos_wp[trd_toload] = 1;
			drive->snaptype = (type == snHOB || type == snSCL) ? snTRD : type;
			trd_loaded[trd_toload] = 1;

			SetWindowText(wnd, (strrchr(filename, '\\') + 1));
		}
		return ok;
	}

	int loadStatus = 0;
	switch (type) {
	case snSP: loadStatus = read_sp(); break;
	case snSNA_48: loadStatus = read_sna48(); break;
	case snSNA_128: loadStatus = read_sna128(); break;
	case snSPG: loadStatus = read_spg(); break;
	case snZ80: loadStatus = read_z80(); break;
	case snTAP: loadStatus = readTAP(); break;
	case snTZX: loadStatus = readTZX(); break;
	case snCSW: loadStatus = readCSW(); break;
	default: break;
	}

	// reload labels if success (network/VM shared folders fix)
	if ((loadStatus == 1) && (trace_labels)) mon_labels.import_file();

	return loadStatus;
}

int read_spg()
{
	hdrSPG1_0* hdr10 = (hdrSPG1_0*)snbuf;
	hdrSPG0_2* hdr02 = (hdrSPG0_2*)snbuf;

	if (memcmp(&hdr10->sign, "SpectrumProg", 12))
		return 0;
	u8 type = hdr10->ver;
	if ((type != 0) && (type != 1) && (type != 2) && (type != 0x10))
		return 0;

	reset(rom_mode::nochange);
	load_spec_colors();
	reset_sound();

	switch (conf.spg_mem_init)
	{
	default:
		break;
	case 1: // random memory initialization
		for (u32 i = 0; i < PAGE * MAX_RAM_PAGES; i++)
		{
			srand((unsigned)time(nullptr));
			u8 byte = rand();
			RAM_BASE_M[i] = byte;
		}
		break;
	case 2: // zero memory initialization
		memset(RAM_BASE_M, 0, PAGE * MAX_RAM_PAGES);
		break;
	}

	cpu.iy = 0x5C3A;
	cpu.alt.hl = 0x2758;
	cpu.i = 63;
	cpu.im = 1;
	comp.p7FFD = 16;

	/* SPG ver.1.0 */
	if (type == 0x10)
	{
		cpu.sp = hdr10->sp;
		cpu.pc = hdr10->pc;
		cpu.iff1 = (hdr10->clk & 4) ? 1 : 0;
		comp.ts.zclk = hdr10->clk & 3;
		comp.ts.page[3] = hdr10->win3_pg;

		u8* data = &hdr10->data;
		for (u8 i = 0; i < hdr10->n_blk; i++)
		{
			u16 size = ((hdr10->blocks[i].size & 0x1F) + 1) * 512;
			u8 page = hdr10->blocks[i].page;
			u16 offs = (hdr10->blocks[i].addr & 0x1F) * 512;
			u8* zxram = page_ram(page) + offs;
			switch ((hdr10->blocks[i].size & 0xC0) >> 6)
			{
			case 0x00:
				memcpy(zxram, data, size);
				break;

			case 0x01:
				demlz(zxram, data, size);
				break;

			case 0x02:
				dehrust(zxram, data, size);
				break;
			}

			data += size;

			if (hdr10->blocks[i].addr & 0x80)
				break;
		}
	}

	/* SPG ver.0.x */
	else
	{
		cpu.sp = hdr02->sp;
		cpu.pc = hdr02->pc;
		cpu.iff1 = 0;
		comp.ts.zclk = 0;
		comp.ts.page[3] = hdr02->win3_pg & ((type == 2) ? 15 : 7);

		if (hdr02->vars_len)
		{
			u16 addr;

			if (!hdr02->vars_addr)
				addr = 0x5B00;
			else
				addr = hdr02->vars_addr;
			addr -= 0x4000;		// fucking crotch for only loading to the lower memory

			memcpy(page_ram(5) + addr, hdr02->vars, hdr02->vars_len);
		}

		u8* data = &hdr02->data;
		for (u8 i = 0; i < 15; i++)
		{
			u16 addr = hdr02->blocks[i].addr;
			if (addr < 0x9A00)
				break;
			int size = hdr02->blocks[i].size * 2048;
			if (!i)
				size += 1536;
			u8 page = hdr02->blocks[i].page & ((type == 2) ? 15 : 7);
			int offs = (addr < 0xC000) ? (addr - 0x8000) : (addr - 0xC000);

			if (addr < 0xC000)
			{
				int sz = ((size + offs) > 0x4000) ? (0x4000 - offs) : size;
				memcpy(page_ram(2) + offs, data, sz);
				offs = 0x0000;
				size -= sz;
				data += sz;
			}

			if (size)
			{
				memcpy(page_ram(page) + offs, data, size);
				data += size;
			}

			if (type != 2)		// for ver. 0.0 and 0.1 there are 4 dummy bytes in every block descriptor
				i++;
		}

		if (hdr02->pgmgr_addr)
		{
			// const u8 mgr[] = {0xC5, 0x01, 0xAF, 0x13, 0xED, 0x79, 0xC1, 0xC9};	// TS-conf
			const u8 mgr[] = { 0xC5, 0x4F, 0xE6, 0xF8, 0x79, 0x28, 0x04, 0xE6, 0x07, 0xF6, 0x40, 0xF6, 0x10, 0x01, 0xFD, 0x7F, 0xED, 0x79, 0xC1, 0xC9 };	// Pentagon
			memcpy(page_ram(5) + hdr02->pgmgr_addr - 0x4000, mgr, sizeof(mgr));
		}

	}

	set_clk();
	set_banks();

	snbuf[0x20] = 0;	// to avoid garbage in header
	SetWindowText(wnd, (char*)snbuf);
	return 1;
}

int read_sna128()
{
	// conf.mem_model = MM_PENTAGON; conf.ramsize = 128;
	reset(rom_mode::nochange);
	hdrSNA128* hdr = (hdrSNA128*)snbuf;
	// reset(hdr->trdos? RM_DOS : RM_SOS);
	cpu.alt.af = hdr->altaf; cpu.alt.bc = hdr->altbc;
	cpu.alt.de = hdr->altde; cpu.alt.hl = hdr->althl;
	cpu.af = hdr->af; cpu.bc = hdr->bc; cpu.de = hdr->de; cpu.hl = hdr->hl;
	cpu.ix = hdr->ix; cpu.iy = hdr->iy; cpu.sp = hdr->sp; cpu.pc = hdr->pc;
	cpu.i = hdr->i; cpu.r_low = hdr->r; cpu.r_hi = hdr->r & 0x80; cpu.im = hdr->im;
	cpu.iff1 = hdr->iff1 ? 1 : 0; comp.p7FFD = hdr->p7FFD;
	comp.pFE = hdr->pFE; comp.border_attr = comp.pFE & 7;
	memcpy(page_ram(5), hdr->page5, PAGE);
	memcpy(page_ram(2), hdr->page2, PAGE);
	memcpy(page_ram((hdr->p7FFD & 7)), hdr->active_page, PAGE);
	u8* newpage = snbuf + 0xC01F;
	u8 mapped = 0x24 | (1 << (hdr->p7FFD & 7));
	for (u8 i = 0; i < 8; i++)
		if (!(mapped & (1 << i))) {
			memcpy(memory + PAGE * i, newpage, PAGE); newpage += PAGE;
		}
	set_banks(); return 1;
}

int read_sna48()
{
	//conf.mem_model = MM_PENTAGON; conf.ramsize = 128;  // molodcov_alex
	reset(rom_mode::sos);
	hdrSNA128* hdr = (hdrSNA128*)snbuf;
	cpu.alt.af = hdr->altaf; cpu.alt.bc = hdr->altbc;
	cpu.alt.de = hdr->altde; cpu.alt.hl = hdr->althl;
	cpu.af = hdr->af; cpu.bc = hdr->bc; cpu.de = hdr->de; cpu.hl = hdr->hl;
	cpu.ix = hdr->ix; cpu.iy = hdr->iy; cpu.sp = hdr->sp;
	cpu.i = hdr->i; cpu.r_low = hdr->r; cpu.r_hi = hdr->r & 0x80; cpu.im = hdr->im;
	cpu.iff1 = hdr->iff1 ? 1 : 0; comp.p7FFD = 0x30;
	comp.pEFF7 |= EFF7_LOCKMEM; //Alone Coder
	comp.pFE = hdr->pFE; comp.border_attr = comp.pFE & 7;
	memcpy(page_ram(5), hdr->page5, PAGE);
	memcpy(page_ram(2), hdr->page2, PAGE);
	memcpy(page_ram(0), hdr->active_page, PAGE);
	cpu.pc = cpu.direct_rm(cpu.sp) + 0x100 * cpu.direct_rm(cpu.sp + 1); cpu.sp += 2;
	set_banks(); return 1;
}

int read_sp()
{
	//conf.mem_model = MM_PENTAGON; conf.ramsize = 128;  // molodcov_alex
	reset(rom_mode::sos);
	hdrSP* hdr = (hdrSP*)snbuf;
	cpu.alt.af = hdr->altaf; cpu.alt.bc = hdr->altbc;
	cpu.alt.de = hdr->altde; cpu.alt.hl = hdr->althl;
	cpu.af = hdr->af; cpu.bc = hdr->bc; cpu.de = hdr->de; cpu.hl = hdr->hl;
	cpu.ix = hdr->ix; cpu.iy = hdr->iy; cpu.sp = hdr->sp; cpu.pc = hdr->pc;
	cpu.i = hdr->i; cpu.r_low = hdr->r; cpu.r_hi = hdr->r & 0x80;
	cpu.iff1 = (hdr->flags & 1);
	cpu.im = 1 + ((hdr->flags >> 1) & 1);
	cpu.iff2 = (hdr->flags >> 2) & 1;
	comp.p7FFD = 0x30;
	comp.pEFF7 |= EFF7_LOCKMEM; //Alone Coder
	comp.pFE = hdr->pFE; comp.border_attr = comp.pFE & 7;
	for (unsigned i = 0; i < hdr->len; i++)
		cpu.direct_wm(hdr->start + i, snbuf[i + 0x26]);
	set_banks(); return 1;
}

int write_sna(FILE* ff)
{
	/*   if (conf.ramsize != 128) {
		  MessageBox(GetForegroundWindow(), "SNA format can hold only\r\n128kb memory models", "Save", MB_ICONERROR);
		  return 0;
	   }*/ //Alone Coder
	hdrSNA128* hdr = (hdrSNA128*)snbuf;
	hdr->trdos = (comp.flags & CF_TRDOS) ? 1 : 0;
	hdr->altaf = cpu.alt.af; hdr->altbc = cpu.alt.bc;
	hdr->altde = cpu.alt.de; hdr->althl = cpu.alt.hl;
	hdr->af = cpu.af; hdr->bc = cpu.bc; hdr->de = cpu.de; hdr->hl = cpu.hl;
	hdr->ix = cpu.ix; hdr->iy = cpu.iy; hdr->sp = cpu.sp; hdr->pc = cpu.pc;
	hdr->i = cpu.i; hdr->r = (cpu.r_low & 0x7F) + cpu.r_hi; hdr->im = cpu.im;
	hdr->iff1 = cpu.iff1 ? 0xFF : 0;
	hdr->p7FFD = comp.p7FFD;
	hdr->pFE = comp.pFE; comp.border_attr = comp.pFE & 7;
	unsigned savesize = sizeof(hdrSNA128);
	u8 mapped = 0x24 | (1 << (comp.p7FFD & 7));
	if (comp.p7FFD == 0x30)
	{ // save 48k
		mapped = 0xFF;
		savesize = 0xC01B;
		hdr->sp -= 2;
		cpu.direct_wm(hdr->sp, cpu.pcl);
		cpu.direct_wm(hdr->sp + 1, cpu.pch);
	}
	memcpy(hdr->page5, memory + PAGE * 5, PAGE);
	memcpy(hdr->page2, memory + PAGE * 2, PAGE);
	memcpy(hdr->active_page, memory + PAGE * (comp.p7FFD & 7), PAGE);
	if (fwrite(hdr, 1, savesize, ff) != savesize) return 0;
	for (u8 i = 0; i < 8; i++)
		if (!(mapped & (1 << i))) {
			if (fwrite(memory + PAGE * i, 1, PAGE, ff) != PAGE) return 0;
		}
	return 1;
}

void unpack_page(u8* dst, int dstlen, u8* src, int srclen)
{
	memset(dst, 0, dstlen);
	while (srclen > 0 && dstlen > 0) {
		if (srclen >= 4 && *(u16*)src == WORD2(0xED, 0xED)) {
			for (u8 i = src[2]; i; i--)
				*dst++ = src[3], dstlen--;
			srclen -= 4; src += 4;
		}
		else *dst++ = *src++, dstlen--, srclen--;
	}
}

int read_z80()
{
	//conf.mem_model = MM_PENTAGON; conf.ramsize = 128;  // molodcov_alex
	auto* hdr = reinterpret_cast<hdrZ80*>(snbuf);
	u8* ptr = snbuf + 30;
	u8 model48k = (hdr->model < 3);
	reset((model48k | (hdr->p7FFD & 0x10)) ? rom_mode::sos : rom_mode::s128);
	if (hdr->flags == 0xFF)
		hdr->flags = 1;
	if (hdr->pc == 0)
	{ // 2.01
		ptr += 2 + hdr->len;
		hdr->pc = hdr->newpc;
		memset(RAM_BASE_M, 0, PAGE * 8); // clear 128k - first 8 pages

		while (ptr < snbuf + snapsize)
		{
			u8* p48[] =
			{
				   base_sos_rom, nullptr, nullptr, nullptr,
				   page_ram(2), page_ram(0), nullptr, nullptr,
				   page_ram(5), nullptr, nullptr, nullptr
			};
			u8* p128[] =
			{
				   base_sos_rom, base_dos_rom, base_128_rom, page_ram(0),
				   page_ram(1), page_ram(2), page_ram(3), page_ram(4),
				   page_ram(5), page_ram(6), page_ram(7), nullptr
			};
			unsigned len = *(u16*)ptr;
			if (ptr[2] > 11)
				return 0;
			u8* dstpage = model48k ? p48[ptr[2]] : p128[ptr[2]];
			if (!dstpage)
				return 0;
			ptr += 3;
			if (len == 0xFFFF)
				memcpy(dstpage, ptr, len = PAGE);
			else
				unpack_page(dstpage, PAGE, ptr, len);
			ptr += len;
		}
	}
	else
	{
		int len = snapsize - 30;
		u8* mem48 = ptr;
		if (hdr->flags & 0x20)
			unpack_page(mem48 = snbuf + 4 * PAGE, 3 * PAGE, ptr, len);
		memcpy(memory + PAGE * 5, mem48, PAGE);
		memcpy(memory + PAGE * 2, mem48 + PAGE, PAGE);
		memcpy(memory + PAGE * 0, mem48 + 2 * PAGE, PAGE);
		model48k = 1;
	}
	cpu.a = hdr->a, cpu.f = hdr->f;
	cpu.bc = hdr->bc, cpu.de = hdr->de, cpu.hl = hdr->hl;
	cpu.alt.bc = hdr->bc1, cpu.alt.de = hdr->de1, cpu.alt.hl = hdr->hl1;
	cpu.alt.a = hdr->a1, cpu.alt.f = hdr->f1;
	cpu.pc = hdr->pc, cpu.sp = hdr->sp; cpu.ix = hdr->ix, cpu.iy = hdr->iy;
	cpu.i = hdr->i, cpu.r_low = hdr->r & 0x7F;
	cpu.r_hi = ((hdr->flags & 1) << 7);
	comp.pFE = (hdr->flags >> 1) & 7;
	comp.border_attr = comp.pFE;
	cpu.iff1 = hdr->iff1, cpu.iff2 = hdr->iff2; cpu.im = (hdr->im & 3);
	comp.p7FFD = (model48k) ? 0x30 : hdr->p7FFD;

	if (model48k)
		comp.pEFF7 |= EFF7_LOCKMEM; //Alone Coder
	set_banks();

	return 1;
}

#define arctmp ((char*)rbuf)
char* arc_fname;
INT_PTR CALLBACK ArcDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_INITDIALOG) {
		for (char* dst = arctmp; *dst; dst += strlen(dst) + 1)
			SendDlgItemMessage(dlg, IDC_ARCLIST, LB_ADDSTRING, 0, (LPARAM)dst);
		SendDlgItemMessage(dlg, IDC_ARCLIST, LB_SETCURSEL, 0, 0);
		return 1;
	}
	if ((msg == WM_COMMAND && wp == IDCANCEL) ||
		(msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE)) EndDialog(dlg, 0);
	if (msg == WM_COMMAND && (LOWORD(wp) == IDOK || (HIWORD(wp) == LBN_DBLCLK && LOWORD(wp) == IDC_ARCLIST)))
	{
		int n = SendDlgItemMessage(dlg, IDC_ARCLIST, LB_GETCURSEL, 0, 0);
		char* dst = arctmp;
		for (int q = 0; q < n; q++) dst += strlen(dst) + 1;
		arc_fname = dst;
		EndDialog(dlg, 0);
	}
	return 0;
}

bool filename_ok(char* fname)
{
	for (char* wc = skiparc; *wc; wc += strlen(wc) + 1)
		if (wcmatch(fname, wc)) return 0;
	return 1;
}

int load_arc(char* fname)
{
	char* ext = strrchr(fname, '.'); if (!ext) return 0;
	ext++;
	char* cmdtmp, done = 0;
	for (char* x = arcbuffer; *x; x = cmdtmp + strlen(cmdtmp) + 1) {
		cmdtmp = x + strlen(x) + 1;
		if (stricmp(ext, x)) continue;

		char dir[0x200]; GetCurrentDirectory(sizeof dir, dir);
		char tmp[0x200]; GetTempPath(sizeof tmp, tmp);
		char d1[0x20]; sprintf(d1, "us%08X", GetTickCount());
		SetCurrentDirectory(tmp);
		CreateDirectory(d1, nullptr);
		SetCurrentDirectory(d1);

		color();
		char cmdln[0x200]; sprintf(cmdln, cmdtmp, fname);
		STARTUPINFO si = { sizeof si };
		si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi;
		unsigned flags = CREATE_NEW_CONSOLE;
		HANDLE hc = CreateFile("CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hc != INVALID_HANDLE_VALUE) CloseHandle(hc), flags = 0;
		if (CreateProcess(nullptr, cmdln, nullptr, nullptr, 0, flags, nullptr, nullptr, &si, &pi)) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			DWORD code; GetExitCodeProcess(pi.hProcess, &code);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			if (!code || MessageBox(GetForegroundWindow(), "Broken archive", nullptr, MB_ICONERROR | MB_OKCANCEL) == IDOK) {
				WIN32_FIND_DATA fd; HANDLE h;
				char* dst = arctmp; unsigned nfiles = 0;
				if ((h = FindFirstFile("*.*", &fd)) != INVALID_HANDLE_VALUE) {
					do {
						if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && filename_ok(fd.cFileName)) {
							strcpy(dst, fd.cFileName); dst += strlen(dst) + 1;
							nfiles++;
						}
					} while (FindNextFile(h, &fd));
					FindClose(h);
				}
				*dst = 0; arc_fname = nullptr;
				if (nfiles == 1) arc_fname = arctmp;
				if (nfiles > 1)
					DialogBox(hIn, MAKEINTRESOURCE(IDD_ARC), GetForegroundWindow(), ArcDlg);
				if (!nfiles) MessageBox(GetForegroundWindow(), "Empty archive!", nullptr, MB_ICONERROR | MB_OK);
				char buf[0x200]; strcpy(buf, tmp); strcat(buf, "\\");
				strcat(buf, d1); strcat(buf, "\\"); if (arc_fname) strcat(buf, arc_fname), arc_fname = buf;
				if (arc_fname && !(done = loadsnap(arc_fname))) MessageBox(GetForegroundWindow(), "loading error", arc_fname, MB_ICONERROR);
				if (!done) done = -1;
			}
			// delete junk
			SetCurrentDirectory(tmp);
			SetCurrentDirectory(d1);
			WIN32_FIND_DATA fd; HANDLE h;
			if ((h = FindFirstFile("*.*", &fd)) != INVALID_HANDLE_VALUE) {
				do { DeleteFile(fd.cFileName); } while (FindNextFile(h, &fd));
				FindClose(h);
			}
		}
		SetCurrentDirectory(tmp);
		RemoveDirectory(d1);
		SetCurrentDirectory(dir);
		eat(); if (done) return done > 0 ? 1 : 0;
	}
	eat(); return 0;
}

void opensnap(int index)
{
	char mask[0x200]; *mask = 0;
	for (char* x = arcbuffer; *x; )
		strcat(mask, ";*."), strcat(mask, x),
		x += strlen(x) + 1, x += strlen(x) + 1;

	char fline[0x400];
	const char* src = "all (sna,spg,z80,sp,tap,tzx,csw,trd,scl,fdi,td0,udi,isd,pro,hobeta)\0*.sna;*.spg;*.z80;*.sp;*.tap;*.tzx;*.csw;*.trd;*.scl;*.td0;*.udi;*.fdi;*.isd;*.pro;*.$?;*.!?<\0"
		"Disk B (trd,scl,fdi,td0,udi,isd,pro,hobeta)\0*.trd;*.scl;*.fdi;*.udi;*.td0;*.isd;*.pro;*.$?<\0"
		"Disk C (trd,scl,fdi,td0,udi,isd,pro,hobeta)\0*.trd;*.scl;*.fdi;*.udi;*.td0;*.isd;*.pro;*.$?<\0"
		"Disk D (trd,scl,fdi,td0,udi,isd,pro,hobeta)\0*.trd;*.scl;*.fdi;*.udi;*.td0;*.isd;*.pro;*.$?<\0\0>";
	if (!conf.trdos_present)
		src = "ZX files (sna,spg,z80,tap,tzx,csw)\0*.sna;*.spg;*.z80;*.tap;*.tzx;*.csw<\0\0>";
	for (char* dst = fline; *src != '>'; src++)
		if (*src == '<') strcpy(dst, mask), dst += strlen(dst);
		else *dst++ = *src;

	OPENFILENAME ofn = { 0 };
	char fname[0x200]; *fname = 0;
	char dir[0x200]; GetCurrentDirectory(sizeof dir, dir);

	ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
	ofn.hwndOwner = GetForegroundWindow();
	ofn.lpstrFilter = fline;
	ofn.lpstrFile = fname; ofn.nMaxFile = sizeof fname;
	ofn.lpstrTitle = "Load Snapshot / Disk / Tape";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFilterIndex = index;
	ofn.lpstrInitialDir = dir;
	//   __debugbreak();
	if (GetSnapshotFileName(&ofn, 0))
	{
		trd_toload = ofn.nFilterIndex - 1;
		if (!loadsnap(fname))
			MessageBox(GetForegroundWindow(), fname, "loading error", MB_ICONERROR);
	}
	eat();
}

const int mx_typs = (1 + 4 * 6);
u8 snaps[mx_typs]; unsigned exts[mx_typs], drvs[mx_typs]; int snp;
static void addref(LPSTR& ptr, u8 sntype, const char* ref, unsigned drv, unsigned ext)
{
	strcpy(ptr, ref); ptr += strlen(ptr) + 1;
	strcpy(ptr, ref + strlen(ref) + 1); ptr += strlen(ptr) + 1;
	drvs[snp] = drv; exts[snp] = ext; snaps[snp++] = sntype;
}

void savesnap(int diskindex)
{
again:
	OPENFILENAME ofn = { 0 };
	char fname[0x200]; *fname = 0;
	if (diskindex >= 0) {
		strcpy(fname, comp.wd.fdd[diskindex].name);
		int ln = strlen(fname);
		if (ln > 4 && (*(unsigned*)(fname + ln - 4) | WORD4(0, 0x20, 0x20, 0x20)) == WORD4('.', 's', 'c', 'l'))
			*(unsigned*)(fname + ln - 4) = WORD4('.', 't', 'r', 'd');
	}

	snp = 1; char types[600], * ptr = types;

	if (diskindex < 0)
	{
		exts[snp] = WORD4('s', 'n', 'a', 0); snaps[snp] = snSNA_128; // default
		addref(ptr, snSNA_128, "ZX-Spectrum 128K snapshot (SNA)\0*.sna", -1, WORD4('s', 'n', 'a', 0));
	}

	ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
	ofn.nFilterIndex = 1;

	if (conf.trdos_present)
	{
		static char mask[] = "Disk A (TRD)\0*.trd";
		static const char ex[][3] = { {'T','R','D'}, {'F','D','I'},{'T','D','0'},{'U','D','I'},{'I','S','D'},{'P','R','O'} };
		static const unsigned ex2[] = { snTRD, snFDI, snTD0, snUDI, snISD, snPRO };

		for (int n = 0; n < 4; n++)
		{
			if (!comp.wd.fdd[n].rawdata)
				continue;
			if (diskindex >= 0 && diskindex != n)
				continue;
			mask[5] = 'A' + n;

			for (int i = 0; i < sizeof ex / sizeof(ex[0]); i++)
			{
				if (diskindex == n && ex2[i] == comp.wd.fdd[n].snaptype)
					ofn.nFilterIndex = snp;
				memcpy(mask + 8, ex[i], 3);
				memcpy(mask + 15, ex[i], 3);
				addref(ptr, ex2[i], mask, n, (*(unsigned*)ex[i] & 0xFFFFFF) | 0x202020);
			}
		}
	}
	ofn.lpstrFilter = types; *ptr = 0;
	ofn.lpstrFile = fname; ofn.nMaxFile = sizeof fname;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.hwndOwner = GetForegroundWindow();
	char* path = strrchr(fname, '\\');
	if (path)
	{ // check if directory exists (for files opened from archive)
		char x = *path; *path = 0;
		unsigned atr = GetFileAttributes(fname); *path = x;
		if (atr == -1 || !(atr & FILE_ATTRIBUTE_DIRECTORY)) *fname = 0;
	}
	else path = fname;
	path = strrchr(path, '.'); if (path) *path = 0; // delete extension

	if (GetSnapshotFileName(&ofn, 1))
	{
		char* fn = strrchr(ofn.lpstrFile, '\\');
		fn = fn ? fn + 1 : ofn.lpstrFile;
		char* extpos = strrchr(fn, '.');
		if (!extpos || stricmp(extpos + 1, (char*)&exts[ofn.nFilterIndex]))
		{
			char* dst = fn + strlen(fn); *dst++ = '.';
			*(unsigned*)dst = exts[ofn.nFilterIndex];
		}
		if (GetFileAttributes(ofn.lpstrFile) != -1 &&
			IDNO == MessageBox(GetForegroundWindow(), "File exists. Overwrite ?", "Save", MB_ICONQUESTION | MB_YESNO))
			goto again;

		FILE* ff = fopen(ofn.lpstrFile, "wb");
		if (ff)
		{
			int res = 0;
			FDD* saveto = comp.wd.fdd + drvs[ofn.nFilterIndex];
			switch (snaps[ofn.nFilterIndex])
			{
			case snSNA_128: res = write_sna(ff); break;
			case snTRD: res = saveto->write_trd(ff); break;
			case snUDI: res = saveto->write_udi(ff); break;
			case snFDI: res = saveto->write_fdi(ff); break;
			case snTD0: res = saveto->write_td0(ff); break;
			case snISD: res = saveto->write_isd(ff); break;
			case snPRO: res = saveto->write_pro(ff); break;
			}
			fclose(ff);
			if (!res)
				MessageBox(GetForegroundWindow(), "write error", "Save", MB_ICONERROR);
			else if (drvs[ofn.nFilterIndex] != -1)
			{
				comp.wd.fdd[drvs[ofn.nFilterIndex]].optype = 0;
				strcpy(comp.wd.fdd[drvs[ofn.nFilterIndex]].name, ofn.lpstrFile);

				//---------Alone Coder
				char* name = ofn.lpstrFile;
				for (char* x = name; *x; x++)
				{
					if (*x == '\\')
						name = x + 1;
				}
				char wintitle[0x200];
				strcpy(wintitle, name);
				strcat(wintitle, " - UnrealSpeccy");
				SetWindowText(wnd, wintitle);
				//~---------Alone Coder
			}
		}
		else
			MessageBox(GetForegroundWindow(), "Can't open file for writing", "Save", MB_ICONERROR);
	}
	eat();
}

void ConvPal8ToBgr24(u8* dst, u8* scrbuf, int dx)
{
	u8* ds = dst;
	for (unsigned i = 0; i < temp.oy; i++) // convert to BGR24 format
	{
		u8* src = scrbuf + i * dx;
		for (unsigned y = 0; y < temp.ox; y++)
		{
			ds[0] = pal0[src[y]].peBlue;
			ds[1] = pal0[src[y]].peGreen;
			ds[2] = pal0[src[y]].peRed;
			ds += 3;
		}
		ds = (PBYTE)(ULONG_PTR(ds + 3) & ~ULONG_PTR(3)); // каждая строка выравнена на 4
	}
}

void ConvRgb15ToBgr24(u8* dst, u8* scrbuf, int dx)
{
	u8* ds = dst;
	for (unsigned i = 0; i < temp.oy; i++) // convert to BGR24 format
	{
		u8* src = scrbuf + i * dx;
		for (unsigned y = 0; y < temp.ox; y++)
		{
			unsigned xx;
			xx = *(unsigned*)(src + y * 2);

			ds[0] = (xx & 0x1F) << 3;
			ds[1] = (xx & 0x03E0) >> 2;
			ds[2] = (xx & 0x7C00) >> 7;
			ds += 3;
		}
		ds = (PBYTE)(ULONG_PTR(ds + 3) & ~ULONG_PTR(3)); // каждая строка выравнена на 4
	}
}

void ConvRgb16ToBgr24(u8* dst, u8* scrbuf, int dx)
{
	u8* ds = dst;
	for (unsigned i = 0; i < temp.oy; i++) // convert to BGR24 format
	{
		u8* src = scrbuf + i * dx;
		for (unsigned y = 0; y < temp.ox; y++)
		{
			unsigned xx;
			xx = *(unsigned*)(src + y * 2);

			ds[0] = (xx & 0x1F) << 3;
			ds[1] = (xx & 0x07E0) >> 3;
			ds[2] = (xx & 0xF800) >> 8;
			ds += 3;
		}
		ds = (PBYTE)(ULONG_PTR(ds + 3) & ~ULONG_PTR(3)); // каждая строка выравнена на 4
	}
}

void ConvYuy2ToBgr24(u8* dst, u8* scrbuf, int dx)
{
	u8* ds = dst;
	for (unsigned i = 0; i < temp.oy; i++) // convert to BGR24 format
	{
		u8* src = scrbuf + i * dx;
		for (unsigned y = 0; y < temp.ox; y++)
		{
			unsigned xx;
			xx = *(unsigned*)(src + y * 2);

			int u = src[y / 2 * 4 + 1], v = src[y / 2 * 4 + 3], Y = src[y * 2];
			int r = (int)(.4732927654e-2 * u - 255.3076403 + 1.989858012 * v + .9803921569 * Y);
			int g = (int)(-.9756592292 * v + 186.0716700 - .4780256930 * u + .9803921569 * Y);
			int b = (int)(.9803921569 * Y + 2.004732928 * u - 255.3076403 - .1014198783e-1 * v); // mapple rulez!
			if (r < 0) r = 0; if (r > 255) r = 255;
			if (g < 0) g = 0; if (g > 255) g = 255;
			if (b < 0) b = 0; if (b > 255) b = 255;

			ds[0] = b;
			ds[1] = g;
			ds[2] = r;
			ds += 3;
		}
		ds = (PBYTE)(ULONG_PTR(ds + 3) & ~ULONG_PTR(3)); // каждая строка выравнена на 4
	}
}

void ConvBgr32ToBgr24(u8* dst, u8* scrbuf, int dx)
{
	u8* ds = dst;
	for (unsigned i = 0; i < temp.oy; i++) // convert to BGR24 format
	{
		u8* src = scrbuf + i * dx;
		for (unsigned x = 0; x < temp.ox; x++)
		{
			ds[0] = src[0];
			ds[1] = src[1];
			ds[2] = src[2];
			src += 4;
			ds += 3;
		}
		ds = (PBYTE)(ULONG_PTR(ds + 3) & ~ULONG_PTR(3)); // каждая строка выравнена на 4
	}
}

TColorConverter ConvBgr24 = nullptr;

bool CopyScreenshotToClipboard() {
	unsigned dx = temp.ox * temp.obpp / 8;
	u8* scrbuf_unaligned = (u8*)malloc(dx * temp.oy + CACHE_LINE);
	u8* scrbuf = (u8*)align_by(scrbuf_unaligned, CACHE_LINE);
	memset(scrbuf, 0, dx * temp.oy);
	renders[conf.render].func(scrbuf, dx); // render to memory buffer (PAL8, YUY2, RGB15, RGB16, RGB32)
	u32 dibSize = ((temp.ox * 3 + 3) & ~3) * temp.oy;
	u8* ds = (u8*)malloc(dibSize);
	ConvBgr24(ds, scrbuf, dx);
	free(scrbuf_unaligned);

	static u8 dibheader32[] = {
		// BITMAPINFOHEADER
		0x28,0x00,0x00,0x00, // Size
		0x80,0x02,0x00,0x00, // Width
		0xe0,0x01,0x00,0x00, // Height
		0x01,0x00,           // Planes
		24,0,                // BitCount
		0x00,0x00,0x00,0x00, // Compression
		0x00,0x10,0x0e,0x00, // SizeImage
		0x00,0x00,0x00,0x00, // XPixelsPerMeter
		0x00,0x00,0x00,0x00, // YPixelsPerMeter
		0x00,0x00,0x00,0x00, // ClrUsed
		0x00,0x00,0x00,0x00  // ClrImportant
	};

	*(unsigned*)(dibheader32 + 4) = temp.ox;
	*(unsigned*)(dibheader32 + 8) = temp.oy;

	// open clipboard
	if (!OpenClipboard(nullptr)) return false;
	if (!EmptyClipboard()) return false;

	// alloc global buffer
	HGLOBAL hbuf = GlobalAlloc(GMEM_MOVEABLE, sizeof(dibheader32) + dibSize);
	if (hbuf == nullptr) {
		CloseClipboard();
		return false;
	}

	u8* globBuf = (u8*)GlobalLock(hbuf);
	if (globBuf == nullptr) {
		CloseClipboard();
		GlobalFree(hbuf);
		return false;
	}
	memcpy(globBuf, dibheader32, sizeof(dibheader32));

	// flip image
	u8* p = globBuf + sizeof(dibheader32);
	for (int y = temp.oy - 1; y >= 0; y--)
	{
		memcpy(p, ds + ((y * temp.ox * 3 + 3) & ~3), temp.ox * 3);
		p += (temp.ox * 3 + 3) & ~3;
	}

	GlobalUnlock(hbuf);

	if (SetClipboardData(CF_DIB, hbuf) == nullptr) {
		CloseClipboard();
		GlobalFree(hbuf);
		return false;
	}

	CloseClipboard();
	GlobalFree(hbuf);
	return true;
}


char* SaveScreenshot(const char* prefix, unsigned counter)
{
	if (!(GetFileAttributes(conf.scrshot_path) & FILE_ATTRIBUTE_DIRECTORY))
	{
		return nullptr;
	}

	static char fname[FILENAME_MAX];
	strcpy(fname, conf.scrshot_path);

	sprintf(fname + strlen(fname), "\\%s%06u.%s", prefix, counter, sshot_ext[(int)conf.scrshot]);

	FILE* fileShot = nullptr;
	if (conf.scrshot == sshot_format::scr || conf.scrshot == sshot_format::bmp)
	{
		fileShot = fopen(fname, "wb");
		if (!fileShot) return nullptr;
	}

	if (conf.scrshot != sshot_format::scr)
	{
		unsigned dx = temp.ox * temp.obpp / 8;
		u8* scrbuf_unaligned = (u8*)malloc(dx * temp.oy + CACHE_LINE);
		u8* scrbuf = (u8*)align_by(scrbuf_unaligned, CACHE_LINE);
		memset(scrbuf, 0, dx * temp.oy);
		renders[conf.render].func(scrbuf, dx); // render to memory buffer (PAL8, YUY2, RGB15, RGB16, RGB32)
		u8* ds = (u8*)malloc(((temp.ox * 3 + 3) & ~3) * temp.oy);
		ConvBgr24(ds, scrbuf, dx);
		free(scrbuf_unaligned);

		if (conf.scrshot == sshot_format::bmp)
		{
			static u8 bmpheader32[] = {
				// BITMAPFILEHEADER
				'B','M',             // Type
				0x36,0x10,0x0e,0x00, // Size
				0x00,0x00,0x00,0x00, // Reserved1,Reserved2
				0x36,0x00,0x00,0x00, // OffBits
				// BITMAPINFOHEADER
				0x28,0x00,0x00,0x00, // Size
				0x80,0x02,0x00,0x00, // Width
				0xe0,0x01,0x00,0x00, // Height
				0x01,0x00,           // Planes
				24,0,                // BitCount
				0x00,0x00,0x00,0x00, // Compression
				0x00,0x10,0x0e,0x00, // SizeImage
				0x00,0x00,0x00,0x00, // XPixelsPerMeter
				0x00,0x00,0x00,0x00, // YPixelsPerMeter
				0x00,0x00,0x00,0x00, // ClrUsed
				0x00,0x00,0x00,0x00  // ClrImportant
			};

			*(unsigned*)(bmpheader32 + 2) = temp.ox * temp.oy * 3 + sizeof(bmpheader32); // filesize
			*(unsigned*)(bmpheader32 + 0x12) = temp.ox;
			*(unsigned*)(bmpheader32 + 0x16) = temp.oy;
			fwrite(bmpheader32, 1, sizeof(bmpheader32), fileShot);

			for (int y = temp.oy - 1; y >= 0; y--)
			{
				fwrite(ds + ((y * temp.ox * 3 + 3) & ~3), 1, temp.ox * 3, fileShot);
			}
		}
		else
		{
			//static png_color bkgColor = {127, 127, 127};
			Bitmap bmp(temp.ox, temp.oy, (temp.ox * 3 + 3) & ~3, PixelFormat24bppRGB, ds);

			size_t fname_len = strlen(fname);
			wchar_t* fnameW = new wchar_t[fname_len + 1];
			mbstowcs(fnameW, fname, fname_len); fnameW[fname_len] = 0;
			bmp.Save(fnameW, conf.scrshot == sshot_format::png ? &clsid_png_encoder : &clsid_gif_encoder, nullptr);
		}
		free(ds);
	}
	else
		fwrite(temp.base, 1, 6912, fileShot);

	if (fileShot) fclose(fileShot);
	return fname;
}

void main_scrshot()
{
	static unsigned counter = 0;
	char* fname = SaveScreenshot("sshot", counter);
	if (fname)
	{
		sprintf(statusline, "saved %s", strrchr(fname, '\\') + 1);
		statcnt = 30;
		counter++;
	}
}

void main_scrshot_clipboard() {
	if (CopyScreenshotToClipboard()) {
		sprintf(statusline, "copied to clipboard");
		statcnt = 30;
	}
}