#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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

volatile u8 gluk_regs[15];

//stop transmit
static void tw_send_stop(void)
{
	TWCR = _BV(TWINT)|_BV(TWEN)|_BV(TWSTO);
	//wait for flag
//	while ((TWCR & _BV(TWSTO))!=0);
//	_delay_us(20); //4mks for PCF8583
}

static u8 tw_send_start(void)
{
	//start transmit
	TWCR =_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);

	//wait for flag
	while ((TWCR & _BV(TWINT))==0);
//	while (TWCR & (1<<TWSTA));

//#ifdef LOGENABLE
//	char log_reset_type[] = "TWS..[..]..\r\n";
//	u8 b = TWSR;
//	log_reset_type[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_reset_type[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	b=TWCR;
//	log_reset_type[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_reset_type[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	b=TWDR;
//	log_reset_type[9] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_reset_type[10] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	to_log(log_reset_type);
//#endif
	//return status
   return TW_STATUS;
}

static u8 tw_send_addr(u8 addr)
{
	//set address
	TWDR = addr;

	//enable transmit
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for end transmit
	while ((TWCR & _BV(TWINT))==0);

//#ifdef LOGENABLE
//	char log_tw[] = "TWA..[..]..\r\n";
//	u8 b = TWSR;
//	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	b=TWCR;
//	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	log_tw[9] = ((addr >> 4) <= 9)?'0'+(addr >> 4):'A'+(addr >> 4)-10;
//	log_tw[10] = ((addr & 0x0F) <= 9)?'0'+(addr & 0x0F):'A'+(addr & 0x0F)-10;
//	to_log(log_tw);
//#endif
	//return status
   return TW_STATUS;
}

static u8 tw_send_data(u8 data)
{
	//set data
	TWDR = data;

	//enable transmit
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for end transmit
	while ((TWCR & _BV(TWINT))==0);

//#ifdef LOGENABLE
//	char log_tw[] = "TWW..[..]..\r\n";
//	u8 b = TWSR;
//	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	b=TWCR;
//	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	log_tw[9] = ((data >> 4) <= 9)?'0'+(data >> 4):'A'+(data >> 4)-10;
//	log_tw[10] = ((data & 0x0F) <= 9)?'0'+(data & 0x0F):'A'+(data & 0x0F)-10;
//	to_log(log_tw);
//#endif
	//return status
   return TW_STATUS;
}

static u8 tw_read_data(u8* data)
{
	//enable
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for flag set
	while ((TWCR & _BV(TWINT))==0);

//#ifdef LOGENABLE
//	char log_tw[] = "TWR..[..]..\r\n";
//	u8 b = TWSR;
//	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	b=TWCR;
//	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
//	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//	log_tw[9] = ((TWDR >> 4) <= 9)?'0'+(TWDR >> 4):'A'+(TWDR >> 4)-10;
//	log_tw[10] = ((TWDR & 0x0F) <= 9)?'0'+(TWDR & 0x0F):'A'+(TWDR & 0x0F)-10;
//	to_log(log_tw);
//#endif
	//get data
	*data = TWDR;

	//return status
   return TW_STATUS;
}

static u8 bcd_to_hex(u8 data)
{
	//convert BCD to HEX
	return  (u8)(data>>4)*10 + (u8)(data & 0x0F);
}

static u8 hex_to_bcd(u8 data)
{
	//convert HEX to BCD
	return  (u8)((data/10)<<4) + (u8)(data%10);
}

static u8 days_of_months()
{
	//return number of days in month
	static const u8 days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	u8 tmp = gluk_regs[GLUK_REG_MONTH]-1;

   	if (tmp > sizeof(days)-1) tmp = 0; //check range

	tmp = days[tmp];

	//check leap-year
	if ((tmp == 28) && ((gluk_regs[GLUK_REG_YEAR] & 0x03) == 0)) tmp++;

	return tmp;
}

static u8 read_eeprom(u8 offset)
{
	u16 ptr = gluk_regs[GLUK_REG_A];
	ptr = (ptr<<4) + (0x0F & offset);

	//wait for eeprom
	eeprom_busy_wait();

	return eeprom_read_byte((u8*)ptr);
}

static void write_eeprom(u8 offset, u8 data)
{
	u16 ptr = gluk_regs[GLUK_REG_A];
	ptr = (ptr<<4) + (0x0F & offset);

	//wait for eeprom
	eeprom_busy_wait();

	eeprom_write_byte ((u8*)ptr, data);
}

void rtc_init(void)
{
	//SCL frequency = CPU clk/ (16 + 2* (TWBR) * 4^(TWPS))
	// 11052000 / (16 + 2*48) = 98678,5Hz (100000Hz recommended for PCF8583)
	TWSR = 0;
	TWBR = 48;
	TWAR = 0; //disable address match unit

	//reset RTC
	//write 0 to control/status register [0] on PCF8583
	rtc_write(0, 0);

	//set Gluk clock registers
	gluk_init();
	if (gluk_regs[GLUK_REG_SEC] == 0) gluk_init();

	//restore mode register from NVRAM (CAPS LED off on init)
	modes_register = rtc_read(RTC_COMMON_MODE_REG) & ~(MODE_CAPSLED);
}

void rtc_write(u8 addr, u8 data)
{
	//set address
	if (tw_send_start() & (TW_START|TW_REP_START))
	{
		if (tw_send_addr(RTC_ADDRESS|TW_WRITE) == TW_MT_SLA_ACK)
		{
			if (tw_send_data(addr) == TW_MT_DATA_ACK)
			{
				//write data
				tw_send_data(data);
			}
		}
	}
	tw_send_stop();
}

u8 rtc_read(u8 addr)
{
	u8 ret = 0;

	//set address
	if (tw_send_start() & (TW_START|TW_REP_START))
	{
		if (tw_send_addr(RTC_ADDRESS|TW_WRITE) == TW_MT_SLA_ACK)
		{
			if (tw_send_data(addr) == TW_MT_DATA_ACK)
			{
				//read data
				if (tw_send_start() == TW_REP_START)
				{
					if (tw_send_addr(RTC_ADDRESS|TW_READ) == TW_MR_SLA_ACK)
					{
						tw_read_data( & ret);
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
	u8 tmp;
	//default values
	gluk_regs[GLUK_REG_A] = GLUK_A_INIT_VALUE;
	gluk_regs[GLUK_REG_B] = GLUK_B_INIT_VALUE;
	gluk_regs[GLUK_REG_C] = GLUK_C_INIT_VALUE;
	gluk_regs[GLUK_REG_D] = GLUK_D_INIT_VALUE;

	//setup

	//read month and day of week
	tmp = rtc_read(6);
	gluk_regs[GLUK_REG_MONTH] = bcd_to_hex(0x1F & tmp);
	tmp = (tmp>>5);
	//PC8583 dayweek 0..6 => DS12788 dayweek 1..7
	gluk_regs[GLUK_REG_DAY_WEEK] = (tmp>6)?1:tmp+1;

	//read year and day of month
	tmp = rtc_read(5);
	gluk_regs[GLUK_REG_DAY_MONTH] = bcd_to_hex(0x3F & tmp);
	gluk_regs[GLUK_REG_YEAR] = tmp>>6;
	tmp = rtc_read(RTC_YEAR_ADD_REG);
	if ((tmp & 0x03) > gluk_regs[GLUK_REG_YEAR])
	{
		//count of year over - correct year
		tmp += 4;
		if (tmp >= 100) tmp = 0;
	}
	gluk_regs[GLUK_REG_YEAR] += tmp & 0xFC;
	rtc_write(RTC_YEAR_ADD_REG,gluk_regs[GLUK_REG_YEAR]); //save year

	//read time
	gluk_regs[GLUK_REG_HOUR] = bcd_to_hex(0x3F & rtc_read(4)); //TODO 12/24 format
	gluk_regs[GLUK_REG_MIN] = bcd_to_hex(rtc_read(3));
	gluk_regs[GLUK_REG_SEC] = bcd_to_hex(rtc_read(2));
}

void gluk_inc(void)
{
	if (++gluk_regs[GLUK_REG_SEC] >= 60)
	{
		gluk_regs[GLUK_REG_SEC] = 0;
		if (++gluk_regs[GLUK_REG_MIN] >= 60)
		{
			gluk_regs[GLUK_REG_MIN] = 0;
			if (++gluk_regs[GLUK_REG_HOUR] >= 24)
			{
				gluk_regs[GLUK_REG_HOUR] = 0;
				if (++gluk_regs[GLUK_REG_DAY_WEEK] > 7 )
				{
					gluk_regs[GLUK_REG_DAY_WEEK] = 1;
				}
				if (++gluk_regs[GLUK_REG_DAY_MONTH] > days_of_months())
				{
					gluk_regs[GLUK_REG_DAY_MONTH] = 1;
					if (++gluk_regs[GLUK_REG_MONTH] > 12)
					{
						gluk_regs[GLUK_REG_MONTH] = 1;
						if (++gluk_regs[GLUK_REG_YEAR] >= 100)
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

u8 gluk_get_reg(u8 index)
{
	u8 tmp;

	if (index < sizeof(gluk_regs)/sizeof(gluk_regs[0]))
	{
		//clock registers from array
		tmp = gluk_regs[index];
		if ((index<10) && ((gluk_regs[GLUK_REG_B] & GLUK_B_DATA_MODE) == 0))
		{
			//clock registers mast be in BCD if HEX-bit not set in reg B
			tmp = hex_to_bcd(tmp);
		}

		if (index == GLUK_REG_C)
		{
			//clear update flag
			gluk_regs[GLUK_REG_C] &= ~GLUK_C_UPDATE_FLAG;

			//3 bit - SD card detect
			//2 bit - SD WRP detect
			tmp |= (((~SD_PIN) & ((1<<SDWRP)|(1<<SDDET)))>>2);

			//0 - bit numlock led status on read
			if ((PS2KEYBOARD_LED_NUMLOCK & modes_register)!=0)
			{
				tmp |= GLUK_C_NUM_LED_FLAG;
			}
			else
			{
				tmp &= ~GLUK_C_NUM_LED_FLAG;
			}
		}

		if (index == GLUK_REG_D)
		{
			//return keyboard statuses
			tmp &= ~(KB_LCTRL_MASK|KB_RCTRL_MASK|KB_LALT_MASK|KB_RALT_MASK|KB_LSHIFT_MASK|KB_RSHIFT_MASK|KB_F12_MASK);
			tmp |= (kb_ctrl_status[0] & (KB_LCTRL_MASK|KB_RCTRL_MASK|KB_LALT_MASK|KB_RALT_MASK|KB_LSHIFT_MASK|KB_RSHIFT_MASK|KB_F12_MASK));
		}

		if (index == GLUK_REG_E)
		{
			//return keyboard statuses
			tmp &= ~(KB_LWIN_MASK_1|KB_RWIN_MASK_1|KB_MENU_MASK_1);
			tmp |= (kb_ctrl_status[1] & (KB_LWIN_MASK_1|KB_RWIN_MASK_1|KB_MENU_MASK_1));
		}
	}
	else
	{
		if (index >= 0xF0)
		{
			if ((gluk_regs[GLUK_REG_C] & GLUK_C_EEPROM_FLAG)!=0)
			{
				//read from eeprom
				tmp = read_eeprom(index);
			}
			else
			{
				//read version
				tmp = GetVersionByte(index);
			}
		}
		else
		{
			//other from nvram
			//- on PCF8583 nvram started from #10
			//- on 512vi1[DS12887] nvram started from #0E
			tmp = rtc_read((index/* & 0x3F*/)+2);
		}
	}

#ifdef LOGENABLE
	{
		char log_gs[] = "GR[..]..\r\n";
		u8 b = index;
		log_gs[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
		log_gs[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
		b = tmp;
		log_gs[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
		log_gs[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
		to_log(log_gs);
	}
#endif
	return tmp;
}

void gluk_set_reg(u8 index, u8 data)
{
#ifdef LOGENABLE
	char log_gs[] = "GS[..]..\r\n";
	u8 b = index;
	log_gs[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_gs[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=data;
	log_gs[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_gs[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_gs);
#endif

	if (index < sizeof(gluk_regs)/sizeof(gluk_regs[0]))
	{
		if (index < 10)
		{
			//write to clock registers
			if ((gluk_regs[GLUK_REG_B] & GLUK_B_DATA_MODE) == 0)
			{
				//array of registers must be in Hex, but data in BCD if HEX-bit not set in reg B
				data = bcd_to_hex(data);
			}
			gluk_regs[index] = data;

			//write to nvram if need
			switch (index)
			{
				case GLUK_REG_SEC:
					if (data <= 59) rtc_write(2, hex_to_bcd(data/*gluk_regs[GLUK_REG_SEC]*/));
					break;
				case GLUK_REG_MIN:
					if (data <= 59) rtc_write(3, hex_to_bcd(data/*gluk_regs[GLUK_REG_MIN]*/));
					break;
				case GLUK_REG_HOUR:
					if (data <= 23) rtc_write(4, 0x3F & hex_to_bcd(data/*gluk_regs[GLUK_REG_HOUR]*/));
					break;
				case GLUK_REG_MONTH:
				case GLUK_REG_DAY_WEEK:
					if ((gluk_regs[GLUK_REG_DAY_WEEK]-1 <= 6) && 
						(gluk_regs[GLUK_REG_MONTH] > 0) && 
						(gluk_regs[GLUK_REG_MONTH] <= 12))
					{
						//DS12788 dayweek 1..7 => PC8583 dayweek 0..6
						rtc_write(6, ((gluk_regs[GLUK_REG_DAY_WEEK]-1)<<5)+(0x1F & hex_to_bcd(gluk_regs[GLUK_REG_MONTH])));
					}
					break;
				case GLUK_REG_YEAR:
					rtc_write(RTC_YEAR_ADD_REG, gluk_regs[GLUK_REG_YEAR]);
				case GLUK_REG_DAY_MONTH:
					rtc_write(5, (gluk_regs[GLUK_REG_YEAR]<<6)+(0x3F & hex_to_bcd(gluk_regs[GLUK_REG_DAY_MONTH])));
					break;
			}
		}
		else
		{
			switch (index)
			{
				case GLUK_REG_A:
					//EEPROM address
					gluk_regs[GLUK_REG_A]=data;
					break;

				case GLUK_REG_B:
					//BCD or Hex mode set
					gluk_regs[GLUK_REG_B]=(data & GLUK_B_DATA_MODE)|GLUK_B_INIT_VALUE;
					break;

				case GLUK_REG_C:
					if ((data & GLUK_C_CLEAR_LOG_FLAG) != 0)
					{
						//clear PS2 keyboard log
						ps2keyboard_reset_log();
					}
					if ((data & GLUK_C_CAPS_LED_FLAG) != (gluk_regs[GLUK_REG_C] & GLUK_C_CAPS_LED_FLAG))
					{
						//switch state of CAPS LED on PS2 keyboard
						gluk_regs[GLUK_REG_C] = gluk_regs[GLUK_REG_C]^GLUK_C_CAPS_LED_FLAG;
						modes_register = modes_register^MODE_CAPSLED;
						//set led on keyboard
						ps2keyboard_send_cmd(PS2KEYBOARD_CMD_SETLED);
					}
					if ((data & GLUK_C_EEPROM_FLAG) != (gluk_regs[GLUK_REG_C] & GLUK_C_EEPROM_FLAG))
					{
						//switch EEPROM mode
						gluk_regs[GLUK_REG_C] = gluk_regs[GLUK_REG_C]^GLUK_C_EEPROM_FLAG;
					}
					break;
			}
		}
	}
	else
	{
		if (index >= 0xF0)
		{
			if (gluk_regs[GLUK_REG_C] & GLUK_C_EEPROM_FLAG)
			{
				//write to eeprom
				write_eeprom(index, data);
			}
			else
			{
				//set version data type
				SetVersionType(index, data);
			}
		}
		else
		{
			//write to nvram
			//- on PCF8583 nvram started from #10
			//- on 512vi1[DS12887] nvram started from #0E
			rtc_write((index/* & 0x3F*/)+2, data);
		}
	}
}
