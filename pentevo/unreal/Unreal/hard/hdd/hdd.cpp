#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

//#define DUMP_HDD_IO 1

constexpr unsigned max_devices = max_phys_hd_drives + 2 * max_phys_cd_drives;

ata_device::hd_status operator|(ata_device::hd_status lhs, ata_device::hd_status rhs)
{
	return static_cast<ata_device::hd_status>(static_cast<u8>(lhs) | static_cast<u8>(rhs));
}

ata_device::hd_error operator|(ata_device::hd_error lhs, ata_device::hd_error rhs)
{
	return static_cast<ata_device::hd_error>(static_cast<u8>(lhs) | static_cast<u8>(rhs));
}

bool operator&(ata_device::hd_control lhs, ata_device::hd_control rhs)
{
	return static_cast<u8>(lhs) & static_cast<u8>(rhs);
}

bool operator&(ata_device::hd_status lhs, ata_device::hd_status rhs)
{
	return static_cast<u8>(lhs) & static_cast<u8>(rhs);
}

phys_device_t phys[max_devices] = {};
unsigned n_phys = 0;

void init_hdd_cd()
{
	memset(&phys, 0, sizeof phys);
	if (conf.ide_skip_real)
		return;

	n_phys = 0;
	n_phys = ata_passer_t::identify(&phys[n_phys], max_devices - n_phys);
	n_phys += atapi_passer_t::identify(&phys[n_phys], max_devices - n_phys);

	if (!n_phys)
		errmsg("HDD/CD emulator can't access physical drives");
}

void delstr_spaces(char* dst, const char* src)
{
	for (; *src; src++)
		if (*src != ' ') *dst++ = *src;
	*dst = 0;
}

unsigned find_hdd_device(const char* name)
{
	char s2[512];
	delstr_spaces(s2, name);
	for (int drive = 0; drive < n_phys; drive++)
	{
		char s1[512];
		delstr_spaces(s1, phys[drive].viewname);
		if (!stricmp(s1, s2))
			return drive;
	}
	return -1;
}

void ata_device::configure(IDE_CONFIG* cfg)
{
	atapi_p.close(); ata_p.close();

	c = cfg->c;
	h = cfg->h;
	s = cfg->s;
	lba = cfg->lba;
	readonly = cfg->readonly;

	memset(regs, 0, sizeof(regs)); // Очищаем регистры
	command_ok(); // Сбрасываем состояние и позицию передачи данных

	phys_dev = -1;
	if (!*cfg->image)
		return;

	phys_device_t filedev{}, * dev;
	phys_dev = find_hdd_device(cfg->image);
	if (phys_dev == -1)
	{
		if (cfg->image[0] == '<')
		{
			errmsg("no physical device %s", cfg->image);
			*cfg->image = 0;
			return;
		}
		strcpy(filedev.filename, cfg->image);
		filedev.type = cfg->cd ? ata_devtype_t::filecd : ata_devtype_t::filehdd;
		dev = &filedev;
	}
	else
	{
		dev = &phys[phys_dev];
		if (dev->type == ata_devtype_t::nthdd)
		{
			// read geometry from id sector
			c = *reinterpret_cast<u16*>(phys[phys_dev].idsector + 2);
			h = *reinterpret_cast<u16*>(phys[phys_dev].idsector + 6);
			s = *reinterpret_cast<u16*>(phys[phys_dev].idsector + 12);
			lba = *reinterpret_cast<unsigned*>(phys[phys_dev].idsector + 0x78);
			if (!lba)
				lba = c * h * s;
		}
	}
	DWORD errcode = 0;
	if (dev->type == ata_devtype_t::nthdd || dev->type == ata_devtype_t::filehdd)
	{
		dev->usage = use;
		errcode = ata_p.open(dev);
		atapi = 0;
	}

	if (dev->type == ata_devtype_t::spti_cd || dev->type == ata_devtype_t::aspi_cd || dev->type == ata_devtype_t::filecd)
	{
		dev->usage = use;
		errcode = atapi_p.open(dev);
		atapi = 1;
	}

	if (errcode == NO_ERROR)
		return;
	errmsg("failed to open %s", cfg->image);
	//err_win32(errcode);
	*cfg->image = 0;
}

void ata_port::reset()
{
	dev[0].reset(ata_device::reset_type::reset_hard);
	dev[1].reset(ata_device::reset_type::reset_hard);
}

bool ata_device::loaded() const
{
	return ata_p.loaded() || atapi_p.loaded();
}

u8 ata_port::read(unsigned n_reg)
{
#ifdef DUMP_HDD_IO
	u8 val = dev[0].read(n_reg) & dev[1].read(n_reg); printf("R%X:%02X ", n_reg, val); return val;
#endif
	return dev[0].read(n_reg) & dev[1].read(n_reg);
}

u16 ata_port::read_data()
{
#ifdef DUMP_HDD_IO
	unsigned val = dev[0].read_data() & dev[1].read_data(); printf("r%04X ", val & 0xFFFF); return val;
#endif
	return dev[0].read_data() & dev[1].read_data();
}

void ata_port::write(unsigned n_reg, u8 data)
{
#ifdef DUMP_HDD_IO
	printf("R%X=%02X ", n_reg, data);
#endif
	dev[0].write(n_reg, data);
	dev[1].write(n_reg, data);
}

void ata_port::write_data(u16 data)
{
#ifdef DUMP_HDD_IO
	printf("w%04X ", data & 0xFFFF);
#endif
	dev[0].write_data(data);
	dev[1].write_data(data);
}

u8 ata_port::read_intrq()
{
#ifdef DUMP_HDD_IO
	u8 i = dev[0].read_intrq() & dev[1].read_intrq(); printf("i%d ", !!i); return i;
#endif
	return dev[0].read_intrq() & dev[1].read_intrq();
}

void ata_device::reset_signature(const reset_type mode)
{
	reg.count = reg.sec = 1;
	reg.err = hd_error::amnf;

	reg.cyl = atapi ? 0xEB14 : 0;
	reg.devhead &= (atapi && mode == reset_type::reset_soft) ? 0x10 : 0;
	reg.status = (mode == reset_type::reset_soft || !atapi) ? hd_status::drdy | hd_status::dsc : hd_status::none;
}

void ata_device::reset(const reset_type mode)
{
	reg.control = hd_control::none; // clear SRST
	intrq = 0;

	command_ok();
	reset_signature(mode);
}

void ata_device::command_ok()
{
	state = hd_state::idle;
	transptr = -1;
	reg.err = hd_error::none;
	reg.status = hd_status::drdy | hd_status::dsc;
}

u8 ata_device::read_intrq()
{
	if (!loaded() || ((reg.devhead ^ device_id) & 0x10) || (reg.control & hd_control::n_ien)) return 0xFF;
	return intrq ? 0xFF : 0x00;
}

u8 ata_device::read(unsigned n_reg)
{
	if (!loaded())
		return 0xFF;
	if ((reg.devhead ^ device_id) & 0x10)
	{
		return 0xFF;
	}

	if (n_reg == 7)
		intrq = 0;
	if (n_reg == 8)
		n_reg = 7; // read alt.status -> read status
	if (n_reg == 7 || (reg.status & hd_status::bsy))
	{
		//	   printf("state=%d\n",state); //Alone Coder
		return static_cast<u8>(reg.status);
	} // BSY=1 or read status
	// BSY = 0
	//// if (reg.status & STATUS_DRQ) return 0xFF;    // DRQ.  ATA-5: registers should not be queried while DRQ=1, but programs do this!
	// DRQ = 0
	return regs[n_reg];
}

u16 ata_device::read_data()
{
	if (!loaded())
		return 0xFFFF;
	if ((reg.devhead ^ device_id) & 0x10)
		return 0xFFFF;
	if (transptr >= transcount)
		return 0xFFFF;

	// DRQ=1, BSY=0, data present
	const auto result = *reinterpret_cast<u16*>(transbf + transptr * 2);
	transptr++;

	if (transptr < transcount)
		return result;
	// look to state, prepare next block
	if (state == hd_state::read_id || state == hd_state::read_atapi)
		command_ok();
	if (state == hd_state::read_sectors)
	{
		if (!--reg.count)
			command_ok();
		else
		{
			next_sector();
			read_sectors();
		}
	}

	return result;
}



char ata_device::exec_ata_cmd(const u8 cmd)
{
	//   printf(__FUNCTION__" cmd=%02X\n", cmd);
	   // EXECUTE DEVICE DIAGNOSTIC for both ATA and ATAPI
	if (cmd == 0x90)
	{
		reset_signature(reset_type::reset_soft);
		return 1;
	}

	if (atapi)
		return 0;

	// INITIALIZE DEVICE PARAMETERS
	if (cmd == 0x91)
	{
		// pos = (reg.cyl * h + (reg.devhead & 0x0F)) * s + reg.sec - 1;
		h = (reg.devhead & 0xF) + 1;
		s = reg.count;
		if (s == 0)
		{
			reg.status = hd_status::drdy | hd_status::df | hd_status::dsc | hd_status::err;
			return 1;
		}

		c = lba / s / h;

		reg.status = hd_status::drdy | hd_status::dsc;
		return 1;
	}

	if ((cmd & 0xFE) == 0x20) // ATA-3 (mandatory), read sectors
	{ // cmd #21 obsolette, rqd for is-dos
 //       printf(__FUNCTION__" sec_cnt=%d\n", reg.count);
		read_sectors();
		return 1;
	}

	if ((cmd & 0xFE) == 0x40) // ATA-3 (mandatory),  verify sectors
	{ //rqd for is-dos
		verify_sectors();
		return 1;
	}

	if ((cmd & 0xFE) == 0x30 && !readonly) // ATA-3 (mandatory), write sectors
	{
		if (seek())
		{
			state = hd_state::write_sectors;
			reg.status = hd_status::drq | hd_status::dsc;
			transptr = 0;
			transcount = 0x100;
		}
		return 1;
	}

	if (cmd == 0x50) // format track (данная реализация - ничего не делает)
	{
		reg.sec = 1;
		if (seek())
		{
			state = hd_state::format_track;
			reg.status = hd_status::drq | hd_status::dsc;
			transptr = 0;
			transcount = 0x100;
		}
		return 1;
	}

	if (cmd == 0xEC)
	{
		prepare_id();
		return 1;
	}

	if (cmd == 0xE7)
	{ // FLUSH CACHE
		if (ata_p.flush())
		{
			command_ok();
			intrq = 1;
		}
		else
			reg.status = hd_status::drdy | hd_status::df | hd_status::dsc | hd_status::err; // 0x71
		return 1;
	}

	if (cmd == 0x10)
	{
		recalibrate();
		command_ok();
		intrq = 1;
		return 1;
	}

	if (cmd == 0x08)		// reset
	{
		recalibrate();
		command_ok();
		intrq = 1;
		return 1;
	}

	if (cmd == 0x70)
	{ // seek
		if (!seek())
			return 1;
		command_ok();
		intrq = 1;
		return 1;
	}

	printf("*** unknown ata cmd %02X ***\n", cmd);

	return 0;
}

char ata_device::exec_atapi_cmd(const u8 cmd)
{
	if (!atapi)
		return 0;

	// soft reset
	if (cmd == 0x08)
	{
		reset(reset_type::reset_soft);
		return 1;
	}
	if (cmd == 0xA1) // IDENTIFY PACKET DEVICE
	{
		prepare_id();
		return 1;
	}

	if (cmd == 0xA0)
	{ // packet
		state = hd_state::recv_packet;
		reg.status = hd_status::drq;
		reg.intreason = atapi_int_reason::cod;
		transptr = 0;
		transcount = 6;
		return 1;
	}

	if (cmd == 0xEC)
	{
		reg.count = 1;
		reg.sec = 1;
		reg.cyl = 0xEB14;

		reg.status = hd_status::dsc | hd_status::drdy | hd_status::err;
		reg.err = hd_error::abrt;
		state = hd_state::idle;
		intrq = 1;
		return 1;
	}

	printf("*** unknown atapi cmd %02X ***\n", cmd);
	// "command aborted" with ATAPI signature
	reg.count = 1;
	reg.sec = 1;
	reg.cyl = 0xEB14;
	return 0;
}

void ata_device::write(const unsigned n_reg, const u8 data)
{
	//   printf("dev=%d, reg=%d, data=%02X\n", device_id, n_reg, data);
	if (!loaded())
		return;
	if (n_reg == 1)
	{
		reg.feat = data;
		return;
	}

	if (n_reg != 7)
	{
		regs[n_reg] = data;
		if (reg.control & hd_control::srst)
		{
			//          printf("dev=%d, reset\n", device_id);
			reset(reset_type::reset_srst);
		}
		return;
	}

	// execute command!
	if (((reg.devhead ^ device_id) & 0x10) && data != 0x90)
		return;
	if (!(reg.status & hd_status::drdy) && !atapi)
	{
		printf("warning: hdd not ready cmd = %02X (ignored)\n", data);
		return;
	}

	reg.err = hd_error::none; intrq = 0;

	//{printf(" [");for (int q=1;q<9;q++) printf("-%02X",regs[q]);printf("]\n");}
	if (exec_atapi_cmd(data))
		return;
	if (exec_ata_cmd(data))
		return;
	reg.status = hd_status::dsc | hd_status::drdy | hd_status::err;
	reg.err = hd_error::abrt;
	state = hd_state::idle; intrq = 1;
}

void ata_device::write_data(const u16 data)
{
	if (!loaded()) return;
	if ((reg.devhead ^ device_id) & 0x10)
		return;
	if (transptr >= transcount)
		return;
	*reinterpret_cast<u16*>(transbf + transptr * 2) = data;
	transptr++;
	if (transptr < transcount)
		return;
	// look to state, prepare next block
	if (state == hd_state::write_sectors)
	{
		write_sectors();
		return;
	}

	if (state == hd_state::format_track)
	{
		format_track();
		return;
	}

	if (state == hd_state::recv_packet)
	{
		handle_atapi_packet();
		return;
	}
}

char ata_device::seek()
{
	unsigned pos;
	if (reg.devhead & 0x40)
	{
		pos = *reinterpret_cast<unsigned*>(regs + 3) & 0x0FFFFFFF;
		if (pos >= lba)
		{
		seek_err:
			//          printf("seek error: lba %d:%d\n", lba, pos);
			reg.status = hd_status::drdy | hd_status::df | hd_status::err;
			reg.err = hd_error::idnf | hd_error::abrt;
			intrq = 1;
			return 0;
		}
		//      printf("lba %d:%d\n", lba, pos);
	}
	else
	{
		if (reg.cyl >= c || static_cast<unsigned>(reg.devhead & 0x0F) >= h || reg.sec > s || !reg.sec)
		{
			//          printf("seek error: chs %4d/%02d/%02d\n", *(u16*)(regs+4), (reg.devhead & 0x0F), reg.sec);
			goto seek_err;
		}
		pos = (reg.cyl * h + (reg.devhead & 0x0F)) * s + reg.sec - 1;
		//      printf("chs %4d/%02d/%02d: %8d\n", *(u16*)(regs+4), (reg.devhead & 0x0F), reg.sec, pos);
	}
	//printf("[seek %I64d]", ((__int64)pos) << 9);
	if (!ata_p.seek(pos))
	{
		reg.status = hd_status::drdy | hd_status::df | hd_status::err;
		reg.err = hd_error::idnf | hd_error::abrt;
		intrq = 1;
		return 0;
	}
	return 1;
}

void ata_device::format_track()
{
	intrq = 1;
	if (!seek())
		return;

	command_ok();
	return;
}

void ata_device::write_sectors()
{
	intrq = 1;
	//printf(" [write] ");
	if (!seek())
		return;

	if (!ata_p.write_sector(transbf))
	{
		reg.status = hd_status::drdy | hd_status::dsc | hd_status::err;
		reg.err = hd_error::unc;
		state = hd_state::idle;
		return;
	}

	if (!--reg.count)
	{
		command_ok();
		return;
	}
	next_sector();

	transptr = 0, transcount = 0x100;
	state = hd_state::write_sectors;
	reg.err = hd_error::none;
	reg.status = hd_status::drq | hd_status::dsc;
}

void ata_device::read_sectors()
{
	//   __debugbreak();
	intrq = 1;
	if (!seek())
		return;

	if (!ata_p.read_sector(transbf))
	{
		reg.status = hd_status::drdy | hd_status::dsc | hd_status::err;
		reg.err = hd_error::unc | hd_error::idnf;
		state = hd_state::idle;
		return;
	}
	transptr = 0;
	transcount = 0x100;
	state = hd_state::read_sectors;
	reg.err = hd_error::none;
	reg.status = hd_status::drdy | hd_status::drq | hd_status::dsc;

	/*
	   if (reg.devhead & 0x40)
		   printf("dev=%d lba=%d\n", device_id, *(unsigned*)(regs+3) & 0x0FFFFFFF);
	   else
		   printf("dev=%d c/h/s=%d/%d/%d\n", device_id, reg.cyl, (reg.devhead & 0xF), reg.sec);
	*/
}

void ata_device::verify_sectors()
{
	intrq = 1;
	//   __debugbreak();

	do
	{
		--reg.count;

		if (!seek())
			return;

		if (reg.count)
			next_sector();
	} while (reg.count);
	command_ok();
}

void ata_device::next_sector()
{
	if (reg.devhead & 0x40)
	{ // LBA
		*reinterpret_cast<unsigned*>(&reg.sec) = (*reinterpret_cast<unsigned*>(&reg.sec) & 0xF0000000) + ((*reinterpret_cast<unsigned*>(&reg.sec) + 1) & 0x0FFFFFFF);
		return;
	}
	// need to recalc CHS for every sector, coz ATA registers
	// should contain current position on failure
	if (reg.sec < s)
	{
		reg.sec++;
		return;
	}
	reg.sec = 1;
	const u8 head = (reg.devhead & 0x0F) + 1;
	if (head < h)
	{
		reg.devhead = (reg.devhead & 0xF0) | head;
		return;
	}
	reg.devhead &= 0xF0;
	reg.cyl++;
}

void ata_device::recalibrate()
{
	reg.cyl = 0;
	reg.devhead &= 0xF0;

	if (reg.devhead & 0x40) // LBA
	{
		reg.sec = 0;
		return;
	}

	reg.sec = 1;
}

constexpr u8 toc_data_track = 0x04;

// [vv] Работа с файлом - образом диска напрямую
void ata_device::handle_atapi_packet_emulate()
{
	//    printf("%s\n", __FUNCTION__);
	memcpy(&atapi_p.cdb, transbf, 12);

	switch (atapi_p.cdb.CDB12.OperationCode)
	{
	case SCSIOP_TEST_UNIT_READY:; // 6
		command_ok();
		return;

	case SCSIOP_READ:; // 10
	{
		const unsigned cnt = (u32(atapi_p.cdb.CDB10.TransferBlocksMsb) << 8) | atapi_p.cdb.CDB10.TransferBlocksLsb;
		unsigned pos = (u32(atapi_p.cdb.CDB10.LogicalBlockByte0) << 24) |
			(u32(atapi_p.cdb.CDB10.LogicalBlockByte1) << 16) |
			(u32(atapi_p.cdb.CDB10.LogicalBlockByte2) << 8) |
			atapi_p.cdb.CDB10.LogicalBlockByte3;

		if (cnt * 2048 > sizeof(transbf))
		{
			reg.status = hd_status::drdy | hd_status::dsc | hd_status::err;
			reg.err = hd_error::unc | hd_error::idnf;
			state = hd_state::idle;
			return;
		}

		for (unsigned i = 0; i < cnt; i++, pos++)
		{
			if (!atapi_p.seek(pos))
			{
				reg.status = hd_status::drdy | hd_status::dsc | hd_status::err;
				reg.err = hd_error::unc | hd_error::idnf;
				state = hd_state::idle;
				return;
			}

			if (!atapi_p.read_sector(transbf + i * 2048))
			{
				reg.status = hd_status::drdy | hd_status::dsc | hd_status::err;
				reg.err = hd_error::unc | hd_error::idnf;
				state = hd_state::idle;
				return;
			}
		}
		intrq = 1;
		reg.atapi_count = cnt * 2048;
		reg.intreason = atapi_int_reason::io;
		reg.status = hd_status::drq;
		transcount = (cnt * 2048) / 2;
		transptr = 0;
		state = hd_state::read_atapi;
		return;
	}

	case SCSIOP_READ_TOC:; // 10
	{
		constexpr u8 toc_data[] =
		{
		  0, 4 + 8 * 2 - 2, 1, 0xAA,
		  0, toc_data_track, 1, 0, 0, 0, 0, 0,
		  0, toc_data_track, 0xAA, 0, 0, 0, 0, 0,
		};
		constexpr unsigned len = sizeof(toc_data);
		memcpy(transbf, toc_data, len);
		reg.atapi_count = len;
		reg.intreason = atapi_int_reason::io;
		reg.status = hd_status::drq;
		transcount = (len + 1) / 2;
		transptr = 0;
		state = hd_state::read_atapi;
		return;
	}
	case SCSIOP_START_STOP_UNIT:; // 10
		command_ok();
		return;

	case SCSIOP_SET_CD_SPEED:; // 12
		command_ok();
		return;
	default:;
	}

	printf("*** unknown scsi cmd %02X ***\n", atapi_p.cdb.CDB12.OperationCode);

	reg.err = hd_error::none;
	state = hd_state::idle;
	reg.status = hd_status::dsc | hd_status::err | hd_status::drdy;
}

void ata_device::handle_atapi_packet()
{
#if defined(DUMP_HDD_IO)
	{
		printf(" [packet");
		for (int i = 0; i < 12; i++)
			printf("-%02X", transbf[i]);
		printf("]\n");
	}
#endif
	if (phys_dev == -1)
		return handle_atapi_packet_emulate();

	memcpy(&atapi_p.cdb, transbf, 12);

	intrq = 1;

	if (atapi_p.cdb.MODE_SELECT10.OperationCode == 0x55)
	{ // MODE SELECT requires additional data from host

		state = hd_state::mode_select;
		reg.status = hd_status::drq;
		reg.intreason = atapi_int_reason::none;
		transptr = 0;
		transcount = atapi_p.cdb.MODE_SELECT10.ParameterListLength[0] * 0x100 + atapi_p.cdb.MODE_SELECT10.ParameterListLength[1];
		return;
	}

	if (atapi_p.cdb.CDB6READWRITE.OperationCode == 0x03 && atapi_p.senselen)
	{ // REQ.SENSE - read cached
		memcpy(transbf, atapi_p.sense, atapi_p.senselen);
		atapi_p.passed_length = atapi_p.senselen; atapi_p.senselen = 0; // next time read from device
		goto ok;
	}

	if (atapi_p.pass_through(transbf, sizeof transbf))
	{
		if (atapi_p.senselen)
		{
			reg.err = static_cast<hd_error>(atapi_p.sense[2] << 4);
			goto err;
		} // err = sense key //win9x hangs on drq after atapi packet when emulator does goto err (see walkaround in SEND_ASPI_CMD)
	ok:
		if (!atapi_p.cdb.CDB6READWRITE.OperationCode)
			atapi_p.passed_length = 0; // bugfix in cdrom driver: TEST UNIT READY has no data
		if (!atapi_p.passed_length /* || atapi_p.passed_length == sizeof transbf */)
		{
			command_ok();
			return;
		}
		reg.atapi_count = atapi_p.passed_length;
		reg.intreason = atapi_int_reason::io;
		reg.status = hd_status::drq;
		transcount = (atapi_p.passed_length + 1) / 2;
		transptr = 0;
		state = hd_state::read_atapi;
	}
	else
	{ // bus error
		reg.err = hd_error::none;
	err:
		state = hd_state::idle;
		reg.status = hd_status::dsc | hd_status::err | hd_status::drdy;
	}
}

void ata_device::prepare_id()
{
	if (phys_dev == -1)
	{
		memset(transbf, 0, 512);
		make_ata_string(transbf + 54, 20, "UNREAL SPECCY HARD DRIVE IMAGE");
		make_ata_string(transbf + 20, 10, "0000");
		*reinterpret_cast<u16*>(transbf) = 0x045A;
		reinterpret_cast<u16*>(transbf)[1] = static_cast<u16>(c);
		reinterpret_cast<u16*>(transbf)[3] = static_cast<u16>(h);
		reinterpret_cast<u16*>(transbf)[6] = static_cast<u16>(s);
		*reinterpret_cast<unsigned*>(transbf + 60 * 2) = lba;
		reinterpret_cast<u16*>(transbf)[20] = 3; // a dual ported multi-sector buffer capable of simultaneous transfers with a read caching capability
		reinterpret_cast<u16*>(transbf)[21] = 512; // cache size=256k
		reinterpret_cast<u16*>(transbf)[22] = 4; // ECC bytes
		reinterpret_cast<u16*>(transbf)[49] = 0x200; // LBA supported
		reinterpret_cast<u16*>(transbf)[80] = 0x3E; // support specifications up to ATA-5
		reinterpret_cast<u16*>(transbf)[81] = 0x13; // ATA/ATAPI-5 T13 1321D revision 3
		reinterpret_cast<u16*>(transbf)[82] = 0x60; // supported look-ahead and write cache

		// make checksum
		transbf[510] = 0xA5;
		u8 cs = 0;
		for (unsigned i = 0; i < 511; i++)
			cs += transbf[i];
		transbf[511] = 0 - cs;
	}
	else
	{ // copy as is...
		memcpy(transbf, phys[phys_dev].idsector, 512);
	}

	state = hd_state::read_id;
	transptr = 0;
	transcount = 0x100;
	intrq = 1;
	reg.status = hd_status::drdy | hd_status::drq | hd_status::dsc;
	reg.err = hd_error::none;
}
