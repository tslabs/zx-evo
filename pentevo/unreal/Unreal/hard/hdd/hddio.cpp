#include "std.h"
#include "emul.h"
#include "vars.h"
#include "hddio.h"

#include <Poco/Util/Application.h>

#include "util.h"

typedef int(__cdecl* get_aspi32_support_info_t)();
typedef int(__cdecl* send_aspi32_command_t)(void* srb);
constexpr int atapi_cdb_size = 12; // sizeof(CDB) == 16
constexpr int max_info_len = 48;

get_aspi32_support_info_t get_aspi32_support_info = nullptr;
send_aspi32_command_t send_aspi32_command = nullptr;
HMODULE h_aspi_dll = nullptr;
HANDLE h_aspi_completion_event;


DWORD ata_passer_t::open(phys_device_t* phy_dev)
{
	close();
	this->dev = phy_dev;

	h_device = CreateFile(phy_dev->filename,
		GENERIC_READ | GENERIC_WRITE, // R/W required!
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (h_device == INVALID_HANDLE_VALUE)
	{
		const ULONG le = GetLastError();
		printf("can't open: `%s', %u\n", phy_dev->filename, le);
		return le;
	}

	if (phy_dev->type == ata_devtype_t::nthdd && phy_dev->usage == use)
	{
		memset(vols, 0, sizeof(vols));

		// lock & dismount all volumes on disk
		char vol_name[256];
		const HANDLE vol_enum = FindFirstVolume(vol_name, _countof(vol_name));
		if (vol_enum == INVALID_HANDLE_VALUE)
		{
			const ULONG le = GetLastError();
			printf("can't enumerate volumes: %u\n", le);
			return le;
		}

		BOOL next_vol = TRUE;
		unsigned vol_idx = 0;
		unsigned i;
		for (; next_vol; next_vol = FindNextVolume(vol_enum, vol_name, _countof(vol_name)))
		{
			const int l = strlen(vol_name);
			if (vol_name[l - 1] == '\\')
				vol_name[l - 1] = 0;

			const HANDLE vol = CreateFile(vol_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			if (vol == INVALID_HANDLE_VALUE)
			{
				printf("can't open volume `%s'\n", vol_name);
				continue;
			}

			UCHAR buf[sizeof(VOLUME_DISK_EXTENTS) + 100 * sizeof(DISK_EXTENT)];
			const auto disk_ext = PVOLUME_DISK_EXTENTS(buf);

			ULONG junk;
			if (!DeviceIoControl(vol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, buf, sizeof(buf), &junk, nullptr))
			{
				junk = GetLastError();
				CloseHandle(vol);
				printf("can't get volume extents: `%s', %lu\n", vol_name, junk);
				continue;
			}

			if (disk_ext->NumberOfDiskExtents == 0)
			{
				// bad volume
				CloseHandle(vol);
				return ERROR_ACCESS_DENIED;
			}

			if (disk_ext->NumberOfDiskExtents > 1)
			{
				for (i = 0; i < disk_ext->NumberOfDiskExtents; i++)
				{
					if (disk_ext->Extents[i].DiskNumber == phy_dev->spti_id)
					{
						// complex volume (volume split over several disks)
						CloseHandle(vol);
						return ERROR_ACCESS_DENIED;
					}
				}
			}

			if (disk_ext->Extents[0].DiskNumber != phy_dev->spti_id)
			{
				CloseHandle(vol);
				continue;
			}

			vols[vol_idx++] = vol;
		}
		FindVolumeClose(vol_enum);

		for (i = 0; i < vol_idx; i++)
		{
			ULONG junk;
			if (!DeviceIoControl(vols[i], FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0, &junk, nullptr))
			{
				junk = GetLastError();
				printf("can't lock volume: %u\n", junk);
				return junk;
			}
			if (!DeviceIoControl(vols[i], FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0, &junk, nullptr))
			{
				junk = GetLastError();
				printf("can't dismount volume: %u\n", junk);
				return junk;
			}
		}
	}
	return NO_ERROR;
}

void ata_passer_t::close()
{
	if (h_device != INVALID_HANDLE_VALUE)
	{
		if (dev->type == ata_devtype_t::nthdd && dev->usage == use)
		{
			// unlock all volumes on disk
			for (unsigned i = 0; i < _countof(vols) && vols[i]; i++)
			{
				ULONG junk;
				DeviceIoControl(vols[i], FSCTL_UNLOCK_VOLUME, nullptr, 0, nullptr, 0, &junk, nullptr);
				CloseHandle(vols[i]);
				vols[i] = nullptr;
			}
		}

		CloseHandle(h_device);
	}
	h_device = INVALID_HANDLE_VALUE;
	dev = nullptr;
}

unsigned ata_passer_t::identify(phys_device_t* outlist, const unsigned max)
{
	unsigned res = 0;
	ata_passer_t ata;

	const unsigned hdd_count = get_hdd_count();

	for (unsigned drive = 0; drive < max_phys_hd_drives && res < max; drive++)
	{

		phys_device_t* dev = outlist + res;
		dev->type = ata_devtype_t::nthdd;
		dev->spti_id = drive;
		dev->usage = enum_only;
		sprintf(dev->filename, R"(\\.\PhysicalDrive%d)", dev->spti_id);

		if (drive >= hdd_count)
			continue;

		const DWORD errcode = ata.open(dev);
		if (errcode == ERROR_FILE_NOT_FOUND)
			continue;

		color(CONSCLR_HARDITEM);
		printf("hd%d: ", drive);

		if (errcode != NO_ERROR)
		{
			color(CONSCLR_ERROR);
			printf("access failed\n");
			err_win32(errcode);
			continue;
		}

		SENDCMDINPARAMS in{};
		in.cBufferSize = 512;
		in.irDriveRegs.bCommandReg = ID_CMD;

		struct
		{
			SENDCMDOUTPARAMS out;
			char xx[512];
		} res_buffer{};

		res_buffer.out.cBufferSize = 512;
		DWORD sz;

		DISK_GEOMETRY geo = { 0 };
		const int res1 = DeviceIoControl(ata.h_device, SMART_RCV_DRIVE_DATA, &in, sizeof in, &res_buffer, sizeof res_buffer, &sz, nullptr);
		if (!res1)
		{
			printf("cant get hdd info, %u\n", GetLastError());
		}
		int res2 = DeviceIoControl(ata.h_device, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &geo, sizeof geo, &sz, nullptr);
		if (geo.BytesPerSector != 512)
		{
			color(CONSCLR_ERROR);
			printf("unsupported sector size (%lu bytes)\n", geo.BytesPerSector);
			continue;
		}

		ata.close();

		if (!res1)
		{
			color(CONSCLR_ERROR);
			printf("identification failed\n");
			continue;
		}

		memcpy(dev->idsector, res_buffer.out.bBuffer, 512);
		char model[42], serial[22];
		swap_bytes(model, res_buffer.out.bBuffer + 54, 20);
		swap_bytes(serial, res_buffer.out.bBuffer + 20, 10);

		dev->hdd_size = geo.Cylinders.LowPart * geo.SectorsPerTrack * geo.TracksPerCylinder;
		unsigned shortsize = dev->hdd_size / 2; char mult = 'K';
		if (shortsize >= 100000)
		{
			shortsize /= 1024;
			mult = 'M';
			if (shortsize >= 100000) {
				shortsize /= 1024;
				mult = 'G';
			}
		}

		color(CONSCLR_HARDINFO);
		printf("%-40s %-20s ", model, serial);
		color(CONSCLR_HARDITEM);
		printf("%8d %cb\n", shortsize, mult);
		if (dev->hdd_size > 0xFFFFFFF)
		{
			color(CONSCLR_WARNING);
			printf("     drive %d warning! LBA48 is not supported. only first 128GB visible\n", drive); //Alone Coder 0.36.7
		}

		print_device_name(dev->viewname, dev);
		res++;
	}

	return res;
}

unsigned ata_passer_t::get_hdd_count() // static
{
	// create a HDEVINFO with all present devices
	const HDEVINFO device_info_set = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, nullptr, nullptr,
	                                                   DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (device_info_set == INVALID_HANDLE_VALUE)
	{
		assert(FALSE);
		return 0;
	}

	// enumerate through all devices in the set
	ULONG member_index = 0;
	while (true)
	{
		// get device interfaces
		SP_DEVICE_INTERFACE_DATA device_interface_data;
		device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(device_info_set, nullptr, &GUID_DEVINTERFACE_DISK, member_index, &device_interface_data))
		{
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				// error
				assert(FALSE);
			}

			// ok, reached end of the device enumeration
			break;
		}

		// process the next device next time
		member_index++;
	}

	// destroy device info list
	SetupDiDestroyDeviceInfoList(device_info_set);

	return member_index;
}


bool ata_passer_t::seek(const unsigned nsector) const
{
	LARGE_INTEGER offset;
	offset.QuadPart = static_cast<LONGLONG>(nsector) << 9;
	const DWORD code = SetFilePointer(h_device, offset.LowPart, &offset.HighPart, FILE_BEGIN);
	return (code != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
}

bool ata_passer_t::read_sector(u8* dst) const
{
	DWORD sz = 0;
	if (!ReadFile(h_device, dst, 512, &sz, nullptr))
		return false;
	if (sz < 512)
		memset(dst + sz, 0, 512 - sz); // on EOF, or truncated file, read 00
	return true;
}

bool ata_passer_t::write_sector(const u8* src) const
{
	DWORD sz = 0;
	return (WriteFile(h_device, src, 512, &sz, nullptr) && sz == 512);
}

DWORD atapi_passer_t::open(phys_device_t* phy_dev)
{
	close();
	this->dev = phy_dev;
	if (phy_dev->type == ata_devtype_t::aspi_cd)
		return NO_ERROR;

	h_device = CreateFile(phy_dev->filename,
		GENERIC_READ | GENERIC_WRITE, // R/W required!
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (h_device != INVALID_HANDLE_VALUE)
		return NO_ERROR;
	return GetLastError();
}

void atapi_passer_t::close()
{
	if (!dev || dev->type == ata_devtype_t::aspi_cd)
		return;
	if (h_device != INVALID_HANDLE_VALUE)
		CloseHandle(h_device);
	h_device = INVALID_HANDLE_VALUE;
	dev = nullptr;
}

unsigned atapi_passer_t::identify(phys_device_t* outlist, int max)
{
	int res = 0;
	atapi_passer_t atapi;

	if (conf.cd_aspi)
	{

		init_aspi();


		for (int adapterid = 0; ; adapterid++)
		{

			SRB_HAInquiry srb = { 0 };
			srb.SRB_Cmd = SC_HA_INQUIRY;
			srb.SRB_HaId = static_cast<u8>(adapterid);
			DWORD aspi_status = send_aspi32_command(&srb);

			if (aspi_status != SS_COMP) break;

			char b1[20], b2[20];
			memcpy(b1, srb.HA_ManagerId, 16); b1[16] = 0;
			memcpy(b2, srb.HA_Identifier, 16); b2[16] = 0;

			if (adapterid == 0) {
				color(CONSCLR_HARDITEM); printf("using ");
				color(CONSCLR_WARNING); printf("%s", b1);
				color(CONSCLR_HARDITEM); printf(" %s\n", b2);
			}
			if (adapterid >= static_cast<int>(srb.HA_Count)) break;
			// int maxTargets = (int)SRB.HA_Unique[3]; // always 8 (?)

			for (int targetid = 0; targetid < 8; targetid++) {

				phys_device_t* dev = outlist + res;
				dev->type = ata_devtype_t::aspi_cd;
				dev->adapterid = adapterid; // (int)SRB.HA_SCSI_ID; // does not work with Nero ASPI
				dev->targetid = targetid;

				DWORD errcode = atapi.open(dev);
				if (errcode != NO_ERROR) continue;

				int ok = atapi.read_atapi_id(dev->idsector, 1);
				atapi.close();
				if (ok != 2) continue; // not a CD-ROM

				print_device_name(dev->viewname, dev);
				res++;
			}
		}


		return res;
	}

	// spti
	for (int drive = 0; drive < max_phys_cd_drives && res < max; drive++)
	{

		phys_device_t* dev = outlist + res;
		dev->type = ata_devtype_t::spti_cd;
		dev->spti_id = drive;
		dev->usage = enum_only;
		sprintf(dev->filename, R"(\\.\CdRom%d)", dev->spti_id);

		DWORD errcode = atapi.open(dev);
		if (errcode == ERROR_FILE_NOT_FOUND)
			continue;

		color(CONSCLR_HARDITEM);
		printf("cd%d: ", drive);
		if (errcode != NO_ERROR)
		{
			color(CONSCLR_ERROR);
			printf("access failed\n");
			err_win32(errcode);
			continue;
		}


		int ok = atapi.read_atapi_id(dev->idsector, 0);
		atapi.close();
		if (!ok)
		{
			color(CONSCLR_ERROR);
			printf("identification failed\n");
			continue;
		}
		if (ok < 2)
			continue; // not a CD-ROM

		print_device_name(dev->viewname, dev);
		res++;
	}

	return res;
}

int atapi_passer_t::pass_through(void* buf, const int buf_sz)
{
	const int res = (conf.cd_aspi) ? send_aspi_cmd(buf, buf_sz) : send_spti_cmd(buf, buf_sz);
	return res;
}

int atapi_passer_t::send_spti_cmd(void* buf, const int buf_sz)
{
	memset(buf, 0, buf_sz);

	struct {
		SCSI_PASS_THROUGH_DIRECT p;
		u8 sense[max_sense_len];
	} srb = { 0 }, dst;

	srb.p.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	*reinterpret_cast<CDB*>(&srb.p.Cdb) = cdb;
	srb.p.CdbLength = sizeof(CDB);
	srb.p.DataIn = SCSI_IOCTL_DATA_IN;
	srb.p.TimeOutValue = 10;
	srb.p.DataBuffer = buf;
	srb.p.DataTransferLength = buf_sz;
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);

	DWORD outsize;
	int r = DeviceIoControl(h_device, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&srb.p, sizeof(srb.p),
		&dst, sizeof(dst),
		&outsize, nullptr);

	if (!r) { return 0; }

	passed_length = dst.p.DataTransferLength;
	if (senselen = dst.p.SenseInfoLength) memcpy(sense, dst.sense, senselen);

#ifdef DUMP_HDD_IO
	printf("sense=%d, data=%d, srbsz=%d/%d, dir=%d. ok%d\n", senselen, passed_length, outsize, sizeof(srb.p), dst.p.DataIn, res);
	printf("srb:"); dump1((BYTE*)&dst, outsize);
	printf("data:"); dump1((BYTE*)databuf, 0x40);
#endif

	return 1;
}

int atapi_passer_t::read_atapi_id(u8* idsector, const char prefix)
{
	memset(&cdb, 0, sizeof(CDB));
	memset(idsector, 0, 512);

	INQUIRYDATA inq;
	cdb.CDB6INQUIRY.OperationCode = 0x12; // INQUIRY
	cdb.CDB6INQUIRY.AllocationLength = sizeof(inq);
	if (!pass_through(&inq, sizeof(inq))) return 0;

	char vendor[10], product[18], revision[6], id[22], ata_name[26];

	memcpy(vendor, inq.VendorId, sizeof(inq.VendorId)); vendor[sizeof(inq.VendorId)] = 0;
	memcpy(product, inq.ProductId, sizeof(inq.ProductId)); product[sizeof(inq.ProductId)] = 0;
	memcpy(revision, inq.ProductRevisionLevel, sizeof(inq.ProductRevisionLevel)); revision[sizeof(inq.ProductRevisionLevel)] = 0;
	memcpy(id, inq.VendorSpecific, sizeof(inq.VendorSpecific)); id[sizeof(inq.VendorSpecific)] = 0;

	if (prefix) {
		color(CONSCLR_HARDITEM);
		if (dev->type == ata_devtype_t::aspi_cd) printf("%d.%d: ", dev->adapterid, dev->targetid);
		if (dev->type == ata_devtype_t::spti_cd) printf("cd%d: ", dev->spti_id);
	}

	trim(vendor); trim(product); trim(revision); trim(id);
	sprintf(ata_name, "%s %s", vendor, product);

	idsector[0] = 0xC0; // removable, accelerated DRQ, 12-byte packet
	idsector[1] = 0x85; // protocol: ATAPI, device type: CD-ROM

	make_ata_string(idsector + 54, 20, ata_name);
	make_ata_string(idsector + 20, 10, id);
	make_ata_string(idsector + 46, 4, revision);

	idsector[0x63] = 0x0B; // caps: IORDY,LBA,DMA
	idsector[0x67] = 4; // PIO timing
	idsector[0x69] = 2; // DMA timing

	if (inq.DeviceType == 5) color(CONSCLR_HARDINFO);
	printf("%-40s %-20s  ", ata_name, id);
	color(CONSCLR_HARDITEM);
	printf("rev.%-4s\n", revision);

	return 1 + (inq.DeviceType == 5);
}

bool atapi_passer_t::read_sector(u8* dst) const
{
	DWORD sz = 0;
	if (!ReadFile(h_device, dst, 2048, &sz, nullptr))
		return false;
	if (sz < 2048)
		memset(dst + sz, 0, 2048 - sz); // on EOF, or truncated file, read 00
	return true;
}

bool atapi_passer_t::seek(const unsigned nsector) const
{
	LARGE_INTEGER offset;
	offset.QuadPart = i64(nsector) * 2048;
	const DWORD code = SetFilePointer(h_device, offset.LowPart, &offset.HighPart, FILE_BEGIN);
	return (code != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
}

void make_ata_string(u8* dst, const unsigned n_words, const char* src)
{
	unsigned i; //Alone Coder 0.36.7
	for (/*unsigned*/ i = 0; i < n_words * 2 && src[i]; i++) dst[i] = src[i];
	while (i < n_words * 2) dst[i++] = ' ';
	for (i = 0; i < n_words * 2; i += 2)
	{
		const u8 tmp = dst[i];
		dst[i] = dst[i + 1];
		dst[i + 1] = tmp;
	}
		
}

void swap_bytes(char* dst, const BYTE* src, const unsigned n_words)
{
	unsigned i; //Alone Coder 0.36.7
	for (/*unsigned*/ i = 0; i < n_words; i++)
	{
		const char c1 = src[2 * i];
		const char c2 = src[2 * i + 1];
		dst[2 * i] = c2;
		dst[2 * i + 1] = c1;
	}
	dst[2 * i] = 0;
	trim(dst);
}

void print_device_name(char* dst, const phys_device_t* dev)
{
	char model[42], serial[22];
	swap_bytes(model, dev->idsector + 54, 20);
	swap_bytes(serial, dev->idsector + 20, 10);
	sprintf(dst, "<%s,%s>", model, serial);
}

void init_aspi()
{
	if (send_aspi32_command)
		return;
	h_aspi_dll = LoadLibrary("WNASPI32.DLL");
	if (!h_aspi_dll)
	{
		errmsg("failed to load WNASPI32.DLL");
		err_win32();
		terminate();
	}
	get_aspi32_support_info = reinterpret_cast<get_aspi32_support_info_t>(GetProcAddress(h_aspi_dll, "GetASPI32SupportInfo"));
	send_aspi32_command = reinterpret_cast<send_aspi32_command_t>(GetProcAddress(h_aspi_dll, "SendASPI32Command"));
	if (!get_aspi32_support_info || !send_aspi32_command) errexit("invalid ASPI32 library");
	const DWORD init = get_aspi32_support_info();
	if (((init >> 8) & 0xFF) != SS_COMP)
		errexit("error in ASPI32 initialization");
	h_aspi_completion_event = CreateEvent(nullptr, 0, 0, nullptr);
}

int atapi_passer_t::send_aspi_cmd(void* buf, int buf_sz)
{
	SRB_ExecSCSICmd srb = { 0 };
	srb.SRB_Cmd = SC_EXEC_SCSI_CMD;
	srb.SRB_HaId = (u8)dev->adapterid;
	srb.SRB_Flags = SRB_DIR_IN | SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
	srb.SRB_Target = (u8)dev->targetid;
	srb.SRB_BufPointer = (u8*)buf;
	srb.SRB_BufLen = buf_sz;
	srb.SRB_SenseLen = sizeof(srb.SenseArea);
	srb.SRB_CDBLen = atapi_cdb_size;
	srb.SRB_PostProc = h_aspi_completion_event;
	memcpy(srb.CDBByte, &cdb, atapi_cdb_size);

	/* DWORD ASPIStatus = */ send_aspi32_command(&srb);
	passed_length = srb.SRB_BufLen;

	if (srb.SRB_Status == SS_PENDING)
	{
		const DWORD aspi_event_status = WaitForSingleObject(h_aspi_completion_event, 10000); // timeout 10sec
		if (aspi_event_status == WAIT_OBJECT_0)
			ResetEvent(h_aspi_completion_event);
	}
	if ((senselen = srb.SRB_SenseLen))
		memcpy(sense, srb.SenseArea, senselen);
	if (passed_length >= 0xffff)
		passed_length = 2048; //Alone Coder //was >=65535 in win9x //makes possible to work in win9x (HDDoct, WDC, Time Gal) //XP fails too

#ifdef DUMP_HDD_IO
	printf("sense=%d, data=%d/%d, ok%d\n", senselen, passed_length, buf_sz, SRB.SRB_Status);
	printf("srb:"); dump1((BYTE*)&SRB, sizeof(SRB));
	printf("data:"); dump1((BYTE*)buf, 0x40);
#endif

	return (srb.SRB_Status == SS_COMP);
}

void done_aspi()
{
	if (!h_aspi_dll)
		return;
	FreeLibrary(h_aspi_dll);
	CloseHandle(h_aspi_completion_event);
}
