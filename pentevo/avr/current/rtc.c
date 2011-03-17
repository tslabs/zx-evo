#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "pins.h"
#include "mytypes.h"
#include "main.h"
#include "zx.h"
#include "rtc.h"
#include "ps2.h"
#include "version.h"
#include "rs232.h"

//if want Log than comment next string
#undef LOGENABLE

volatile UBYTE gluk_regs[14];

//stop transmit
#define tw_send_stop() {TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);}

static UBYTE tw_send_start(void)
{
	//start transmit
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for flag
	while (!(TWCR & (1<<TWINT)));

//#ifdef LOGENABLE
//	char log_reset_type[] = "TWS..\r\n";
//	UBYTE b = TWSR;
//	log_reset_type[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_reset_type[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	to_log(log_reset_type);
//#endif
	//return status
   return TWSR&0xF8;
}

static UBYTE tw_send_addr(UBYTE addr)
{
	//set address
	TWDR = addr;

	//enable transmit
	TWCR = (1<<TWINT)|(1<<TWEN);

	//wait for end transmit
	while (!(TWCR & (1<<TWINT)));

#ifdef LOGENABLE
	char log_tw[] = "TWA.. ..\r\n";
	UBYTE b = TWSR;
	log_tw[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[6] = ((addr >> 4) <= 9 )?'0'+(addr >> 4):'A'+(addr >> 4)-10;
	log_tw[7] = ((addr & 0x0F) <= 9 )?'0'+(addr & 0x0F):'A'+(addr & 0x0F)-10;
	to_log(log_tw);
#endif
	//return status
   return TWSR&0xF8;
}

static UBYTE tw_send_data(UBYTE data)
{
	//set data
	TWDR = data;

	//enable transmit
	TWCR = (1<<TWINT)|(1<<TWEN);

	//wait for end transmit
	while (!(TWCR & (1<<TWINT)));

#ifdef LOGENABLE
	char log_tw[] = "TWD.. ..\r\n";
	UBYTE b = TWSR;
	log_tw[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[6] = ((data >> 4) <= 9 )?'0'+(data >> 4):'A'+(data >> 4)-10;
	log_tw[7] = ((data & 0x0F) <= 9 )?'0'+(data & 0x0F):'A'+(data & 0x0F)-10;
	to_log(log_tw);
#endif
	//return status
   return TWSR&0xF8;
}

static UBYTE tw_read_data(UBYTE* data)
{
	//enable
	TWCR = (1<<TWINT)|(1<<TWEN);

	//wait for flag set
	while (!(TWCR & (1<<TWINT)));

#ifdef LOGENABLE
	char log_tw[] = "TWR.. ..\r\n";
	UBYTE b = TWSR;
	log_tw[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[6] = ((TWDR >> 4) <= 9 )?'0'+(TWDR >> 4):'A'+(TWDR >> 4)-10;
	log_tw[7] = ((TWDR & 0x0F) <= 9 )?'0'+(TWDR & 0x0F):'A'+(TWDR & 0x0F)-10;
	to_log(log_tw);
#endif
	//get data
	*data = TWDR;

	//return status
   return TWSR & 0xF8;
}

static UBYTE bcd_to_hex(UBYTE data)
{
	//convert BCD to HEX
	return  (data>>4)*10 + (data&0x0F);
}

static UBYTE hex_to_bcd(UBYTE data)
{
	//convert HEX to BCD
	return  ((data/10)<<4) + (data%10);
}

static UBYTE days_of_months()
{
	//return number of days in month
	static const UBYTE days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	UBYTE tmp = gluk_regs[GLUK_REG_MONTH]-1;

   	if ( tmp > sizeof(days)-1 ) tmp = 0; //check range

	tmp = days[tmp];

	//check leap-year
	if ( (tmp == 28) && ( ( gluk_regs[GLUK_REG_YEAR]&0x03 ) == 0 ) ) tmp++;

	return tmp;
}

void rtc_init(void)
{
	//SCL frequency = CPU clk/ ( 16 + 2* (TWBR) * 4^(TWPS) )
	// 11052000 / (16 + 2*48 ) = 98678,5Hz (100000Hz recommended for PCF8583)
	TWBR = 48;
	TWSR = 0;

	//reset RTC
	//write 0 to control/status register [0] on PCF8583
	rtc_write(0, 0);

	//set Gluk clock registers
	gluk_init();
	if ( gluk_regs[GLUK_REG_SEC] == 0 ) gluk_init();

	//restore mode register from NVRAM
	modes_register = rtc_read(RTC_COMMON_MODE_REG);

	//set modes on fpga
	//zx_spi_send(SPI_CONFIG_REG, modes_register&MODE_VGA, 0);
	zx_set_config(0);
}

void rtc_write(UBYTE addr, UBYTE data)
{
	//set address
	if ( tw_send_start() & (TW_START|TW_REP_START) )
	{
		if ( tw_send_addr(RTC_ADDRESS) == TW_MT_SLA_ACK )
		{
			if ( tw_send_data(addr) == TW_MT_DATA_ACK )
			{
				//write data
				tw_send_data(data);
			}
		}
	}
	tw_send_stop();
}

UBYTE rtc_read(UBYTE addr)
{
	UBYTE ret = 0;
	//set address
	if ( tw_send_start() & (TW_START|TW_REP_START) )
	{
		if ( tw_send_addr(RTC_ADDRESS) == TW_MT_SLA_ACK )
		{
			if ( tw_send_data(addr) == TW_MT_DATA_ACK )
			{
				//read data
				if ( tw_send_start() == TW_REP_START )
				{
					if ( tw_send_addr(RTC_ADDRESS|0x01) == TW_MR_SLA_ACK )
					{
						tw_read_data(&ret);
					}
				}
			}
		}
	}
	tw_send_stop();
	return ret;
}

void gluk_init(void)
{
	UBYTE tmp;
	//default values
	gluk_regs[GLUK_REG_A] = 0x00;
	gluk_regs[GLUK_REG_B] = 0x02;
	gluk_regs[GLUK_REG_C] = 0x00;
	gluk_regs[GLUK_REG_D] = 0x80;

	//setup

	//read month and day of week
	tmp = rtc_read(6);
	gluk_regs[GLUK_REG_MONTH] = bcd_to_hex(0x1F&tmp);
	tmp = (tmp>>5);
	//PC8583 dayweek 0..6 => DS12788 dayweek 1..7
	gluk_regs[GLUK_REG_DAY_WEEK] = (tmp>6)?1:tmp+1;

	//read year and day of month
	tmp = rtc_read(5);
	gluk_regs[GLUK_REG_DAY_MONTH] = bcd_to_hex(0x3F&tmp);
	gluk_regs[GLUK_REG_YEAR] = tmp>>6;
	tmp = rtc_read(RTC_YEAR_ADD_REG);
	if ( (tmp&0x03) > gluk_regs[GLUK_REG_YEAR] )
	{
		//count of year over - correct year
		tmp += 4;
		if ( tmp >= 100 ) tmp = 0;
	}
	gluk_regs[GLUK_REG_YEAR] += tmp&0xFC;
	rtc_write(RTC_YEAR_ADD_REG,gluk_regs[GLUK_REG_YEAR]); //save year

	//read time
	gluk_regs[GLUK_REG_HOUR] = bcd_to_hex(0x3F&rtc_read(4)); //TODO 12/24 format
	gluk_regs[GLUK_REG_MIN] = bcd_to_hex(rtc_read(3));
	gluk_regs[GLUK_REG_SEC] = bcd_to_hex(rtc_read(2));
}

void gluk_inc(void)
{
	if ( ++gluk_regs[GLUK_REG_SEC] >= 60 )
	{
		gluk_regs[GLUK_REG_SEC] = 0;
		if ( ++gluk_regs[GLUK_REG_MIN] >= 60 )
		{
			gluk_regs[GLUK_REG_MIN] = 0;
			if ( ++gluk_regs[GLUK_REG_HOUR] >= 24 )
			{
				gluk_regs[GLUK_REG_HOUR] = 0;
				if ( ++gluk_regs[GLUK_REG_DAY_WEEK] > 7  )
				{
					gluk_regs[GLUK_REG_DAY_WEEK] = 1;
				}
				if ( ++gluk_regs[GLUK_REG_DAY_MONTH] > days_of_months() )
				{
					gluk_regs[GLUK_REG_DAY_MONTH] = 1;
					if ( ++gluk_regs[GLUK_REG_MONTH] > 12 )
					{
						gluk_regs[GLUK_REG_MONTH] = 1;
						if( ++gluk_regs[GLUK_REG_YEAR] >= 100 )
						{
							gluk_regs[GLUK_REG_YEAR] = 0;
						}
					}
				}
			}
		}
	}

	//set update flag
	gluk_regs[GLUK_REG_C] |= GLUK_C_UPDATE_FLAG;

//#ifdef LOGENABLE
//{
//	char log_int_rtc[] = "00.00.00\r\n";
//	log_int_rtc[0] = '0' + gluk_regs[GLUK_REG_HOUR]/10;
//	log_int_rtc[1] = '0' + gluk_regs[GLUK_REG_HOUR]%10;
//	log_int_rtc[3] = '0' + gluk_regs[GLUK_REG_MIN]/10;
//	log_int_rtc[4] = '0' + gluk_regs[GLUK_REG_MIN]%10;
//	log_int_rtc[6] = '0' + gluk_regs[GLUK_REG_SEC]/10;
//	log_int_rtc[7] = '0' + gluk_regs[GLUK_REG_SEC]%10;
//	to_log(log_int_rtc);
//}
//#endif
}

UBYTE gluk_get_reg(UBYTE index)
{
	if( index < sizeof(gluk_regs)/sizeof(gluk_regs[0]) )
	{
		//clock registers from array
		UBYTE tmp = gluk_regs[index];
		if ( ( index<10 ) && ( (gluk_regs[GLUK_REG_B]&GLUK_B_DATA_MODE) == 0 ) )
		{
			//clock registers mast be in BCD if HEX-bit not set in reg B
			tmp = hex_to_bcd(tmp);
		}

		if ( index == GLUK_REG_C )
		{
			//clear update flag
			gluk_regs[GLUK_REG_C] &= ~GLUK_C_UPDATE_FLAG;
		}

		return tmp;
	}
	else
	{
		if ( index >= 0xF0 )
		{
			//read version
			return GetVersionByte( index&0x0F );
		}

		//other from nvram
		//- on PCF8583 nvram started from #10
		//- on 512vi1[DS12887] nvram started from #0E
		return rtc_read( (index&0x3F)+2 );
	}
}

void gluk_set_reg(UBYTE index, UBYTE data)
{
	if( index < sizeof(gluk_regs)/sizeof(gluk_regs[0]) )
	{
		if ( index<10 )
		{
			//write to clock registers
			if ( (gluk_regs[GLUK_REG_B]&GLUK_B_DATA_MODE) == 0 )
			{
				//array of registers must be in Hex, but data in BCD if HEX-bit not set in reg B
				data = bcd_to_hex(data);
			}
			gluk_regs[index] = data;

			//write to nvram if need
			switch( index )
			{
				case GLUK_REG_SEC:
					rtc_write(2, hex_to_bcd(gluk_regs[GLUK_REG_SEC]));
					break;
				case GLUK_REG_MIN:
					rtc_write(3, hex_to_bcd(gluk_regs[GLUK_REG_MIN]));
					break;
				case GLUK_REG_HOUR:
					rtc_write(4, 0x3F&hex_to_bcd(gluk_regs[GLUK_REG_HOUR]));
					break;
				case GLUK_REG_MONTH:
				case GLUK_REG_DAY_WEEK:
					//DS12788 dayweek 1..7 => PC8583 dayweek 0..6
					rtc_write(6, ((gluk_regs[GLUK_REG_DAY_WEEK]-1)<<5)+(0x1F&hex_to_bcd(gluk_regs[GLUK_REG_MONTH])));
					break;
				case GLUK_REG_YEAR:
					rtc_write(RTC_YEAR_ADD_REG, gluk_regs[GLUK_REG_YEAR]);
				case GLUK_REG_DAY_MONTH:
					rtc_write(5, (gluk_regs[GLUK_REG_YEAR]<<6)+(0x3F&hex_to_bcd(gluk_regs[GLUK_REG_DAY_MONTH])));
					break;
			}
		}
	}
	else
	{
		if ( index >= 0xF0 )
		{
			//set version data type
			SetVersionType( data );
		}
		else
		{
			//write to nvram
			//- on PCF8583 nvram started from #10
			//- on 512vi1[DS12887] nvram started from #0E
			rtc_write( (index&0x3F)+2, data);
		}
	}
}
