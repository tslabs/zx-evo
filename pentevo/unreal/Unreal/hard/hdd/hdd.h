#pragma once
#include "sysdefs.h"

struct ata_device
{
	enum class atapi_int_reason : u8
	{
		none = 0x00,
		cod = 0x01,
		io = 0x02,
		release = 0x04
	};

	enum class hd_status : u8
	{
		bsy = 0x80,
		drdy = 0x40,
		df = 0x20,
		dsc = 0x10,
		drq = 0x08,
		corr = 0x04,
		idx = 0x02,
		err = 0x01,
		none = 0x00,
	};

	enum class hd_error : u8
	{
		bbk = 0x80,
		unc = 0x40,
		mc = 0x20,
		idnf = 0x10,
		mcr = 0x08,
		abrt = 0x04,
		tk0_nf = 0x02,
		amnf = 0x01,
		none = 0x00,
	};

	enum class hd_control : u8
	{
		none = 0x00,
		srst = 0x04,
		n_ien = 0x02
	};

	enum class hd_state
	{
		idle = 0,
		read_id,
		read_sectors,
		verify_sectors,
		write_sectors,
		format_track,
		recv_packet,
		read_atapi,
		mode_select
	};


	unsigned c{}, h{}, s{}, lba{};
	union
	{
		u8 regs[12]{};
		struct
		{
			u8 data;
			hd_error err;             // for write, features
			union
			{
				u8 count;
				atapi_int_reason intreason;
			};
			u8 sec;
			union
			{
				u16 cyl;
				u16 atapi_count;
				struct
				{
					u8 cyl_l;
					u8 cyl_h;
				};
			};
			u8 devhead;
			::ata_device::hd_status status;          // for write, cmd
			/*                  */
			hd_control control;         // reg8 - control (CS1,DA=6)
			u8 feat;
			u8 cmd;
			u8 reserved;        // reserved
		} reg;
	};
	u8 intrq{};
	u8 readonly{};
	u8 device_id{};             // 0x00 - master, 0x10 - slave
	u8 atapi{};                 // flag for CD-ROM device

	u8 read(unsigned n_reg);
	void write(unsigned n_reg, u8 data);
	u16 read_data();
	void write_data(u16 data);
	u8 read_intrq();

	char exec_ata_cmd(u8 cmd);
	char exec_atapi_cmd(u8 cmd);

	enum class reset_type { reset_hard, reset_soft, reset_srst };
	void reset_signature(reset_type mode = reset_type::reset_soft);

	void reset(reset_type mode);
	char seek();
	void recalibrate();
	void configure(IDE_CONFIG* cfg);
	void prepare_id();
	void command_ok();
	void next_sector();
	void read_sectors();
	void verify_sectors();
	void write_sectors();
	void format_track();

	

	

	hd_state state;
	unsigned transptr{}, transcount{};
	int phys_dev{};
	u8 transbf[0xFFFF]{}; // ATAPI is able to tranfer 0xFFFF bytes. passing more leads to error

	void handle_atapi_packet();
	void handle_atapi_packet_emulate();

	ata_passer_t ata_p;
	atapi_passer_t atapi_p;
	bool loaded() const;
	//was crashed at atapi_p.loaded() if no master or slave device!!! see fix in ATAPI_PASSER //Alone Coder
};

struct ata_port
{
	ata_device dev[2];
	u8 read_high{}, write_high{};

	ata_port()
	{
		dev[0].device_id = 0;
		dev[1].device_id = 0x10;
		reset();
	}

	u8 read(unsigned n_reg);
	void write(unsigned n_reg, u8 data);
	u16 read_data();
	void write_data(u16 data);
	u8 read_intrq();

	void reset();
};

extern phys_device_t phys[];
extern unsigned n_phys;

unsigned find_hdd_device(const char* name);
void init_hdd_cd();
