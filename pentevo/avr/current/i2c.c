
#include <avr/io.h>
#include <util/twi.h>
#include "mytypes.h"
#include "i2c.h"

u8 tw_send_start()
{
	//start transmit
	TWCR =_BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

	//wait for flag
	while (!(TWCR & _BV(TWINT)));

#ifdef LOGENABLE
	char log_reset_type[] = "TWS..[..]..\r\n";
	u8 b = TWSR;
	log_reset_type[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_reset_type[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=TWCR;
	log_reset_type[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_reset_type[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=TWDR;
	log_reset_type[9] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_reset_type[10] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_reset_type);
#endif

   return TW_STATUS;
}

//stop transmit
void tw_send_stop()
{
	TWCR = _BV(TWINT)|_BV(TWEN)|_BV(TWSTO);
}

u8 tw_send_addr(u8 addr)
{
	//set address
	TWDR = addr;

	//enable transmit
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for end transmit
	while ((TWCR & _BV(TWINT))==0);

#ifdef LOGENABLE
	char log_tw[] = "TWA..[..]..\r\n";
	u8 b = TWSR;
	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=TWCR;
	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[9] = ((addr >> 4) <= 9)?'0'+(addr >> 4):'A'+(addr >> 4)-10;
	log_tw[10] = ((addr & 0x0F) <= 9)?'0'+(addr & 0x0F):'A'+(addr & 0x0F)-10;
	to_log(log_tw);
#endif

  return TW_STATUS;
}

u8 tw_send_data(u8 data)
{
	//set data
	TWDR = data;

	//enable transmit
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for end transmit
	while ((TWCR & _BV(TWINT))==0);

#ifdef LOGENABLE
	char log_tw[] = "TWW..[..]..\r\n";
	u8 b = TWSR;
	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=TWCR;
	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[9] = ((data >> 4) <= 9)?'0'+(data >> 4):'A'+(data >> 4)-10;
	log_tw[10] = ((data & 0x0F) <= 9)?'0'+(data & 0x0F):'A'+(data & 0x0F)-10;
	to_log(log_tw);
#endif

  return TW_STATUS;
}

u8 tw_read_data(u8 *data)
{
	//enable
	TWCR = _BV(TWINT)|_BV(TWEN);

	//wait for flag set
	while ((TWCR & _BV(TWINT))==0);

#ifdef LOGENABLE
	char log_tw[] = "TWR..[..]..\r\n";
	u8 b = TWSR;
	log_tw[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b=TWCR;
	log_tw[6] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_tw[7] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	log_tw[9] = ((TWDR >> 4) <= 9)?'0'+(TWDR >> 4):'A'+(TWDR >> 4)-10;
	log_tw[10] = ((TWDR & 0x0F) <= 9)?'0'+(TWDR & 0x0F):'A'+(TWDR & 0x0F)-10;
	to_log(log_tw);
#endif

  //get data
	*data = TWDR;
   return TW_STATUS;
}
