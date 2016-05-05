/*

Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Author : FTDI 

Revision History: 
0.1 - date 2013.04.24 - Initial Version
0.2 - date 2013.08.19 - few inclusions
1.0 - date 2015.03.16 - Update with new APIs for FT81X

File Description: 
    This file defines the generic APIs of host access layer for the FT800 or EVE compatible silicon. 
    Application shall access FT800 or EVE resources over these APIs,regardless of I2C or SPI protocol.   
    I2C and SPI is selected by compiler switch "FT_I2C_MODE"  and "FT_SPI_MODE". In addition, there are 
    some helper functions defined for FT800 coprocessor engine as well as host commands.
 
 */
#ifndef FT_GPU_HAL_H
#define FT_GPU_HAL_H



typedef enum {
	FT_GPU_I2C_MODE = 0,
	FT_GPU_SPI_MODE,

	FT_GPU_MODE_COUNT,
	FT_GPU_MODE_UNKNOWN = FT_GPU_MODE_COUNT
}FT_GPU_HAL_MODE_E;

typedef enum {
	FT_GPU_HAL_OPENED,
	FT_GPU_HAL_READING,
	FT_GPU_HAL_WRITING,
	FT_GPU_HAL_CLOSED,

	FT_GPU_HAL_STATUS_COUNT,
	FT_GPU_HAL_STATUS_ERROR = FT_GPU_HAL_STATUS_COUNT
}FT_GPU_HAL_STATUS_E;

typedef struct {
	union {
		ft_uint8_t spi_cs_pin_no;		//spi chip select number of ft8xx chip
		ft_uint8_t i2c_addr;			//i2c address of ft8xx chip
	};
	union {
		ft_uint16_t spi_clockrate_khz;  //In KHz
		ft_uint16_t i2c_clockrate_khz;  //In KHz
	};
	ft_uint8_t channel_no;				//mpsse channel number
	ft_uint8_t pdn_pin_no;				//ft8xx power down pin number
}Ft_Gpu_Hal_Config_t;

typedef struct {
	ft_uint8_t reserved;
}Ft_Gpu_App_Context_t;

typedef struct {
	/* Total number channels for libmpsse */
	ft_uint32_t TotalChannelNum;
}Ft_Gpu_HalInit_t;

typedef enum {
	FT_GPU_READ = 0,
	FT_GPU_WRITE,
}FT_GPU_TRANSFERDIR_T;


typedef struct {	
	ft_uint32_t length; //IN and OUT
	ft_uint32_t address;
	ft_uint8_t  *buffer;
}Ft_Gpu_App_Transfer_t;

typedef struct {
	Ft_Gpu_App_Context_t    app_header;
	Ft_Gpu_Hal_Config_t     hal_config;

    ft_uint16_t 			ft_cmd_fifo_wp; //coprocessor fifo write pointer
    ft_uint16_t 			ft_dl_buff_wp;  //display command memory write pointer

	FT_GPU_HAL_STATUS_E 	status;        //OUT
	ft_void_t*          	hal_handle;        //IN/OUT
	/* Additions specific to ft81x */
	ft_uint8_t				spichannel;			//variable to contain single/dual/quad channels
	ft_uint8_t				spinumdummy;		//number of dummy bytes as 1 or 2 for spi read
}Ft_Gpu_Hal_Context_t;

/* FIFO buffer management */
typedef struct Ft_Fifo_t{
	ft_uint32_t 		fifo_buff; 	//fifo buffer address
	ft_int32_t 			fifo_len; 	//fifo length
	ft_int32_t 			fifo_wp; 	//fifo write pointer - maintained by host
	ft_int32_t 			fifo_rp;  	//fifo read point - maintained by device

	/* FT800 series specific registers */
	ft_uint32_t 		HW_Read_Reg;	//hardware fifo read register
	ft_uint32_t 		HW_Write_Reg;	//hardware fifo write register
}Ft_Fifo_t;

/*The basic APIs Level 1*/
ft_bool_t              Ft_Gpu_Hal_Init(Ft_Gpu_HalInit_t *halinit);
ft_bool_t              Ft_Gpu_Hal_Open(Ft_Gpu_Hal_Context_t *host);

/*The APIs for reading/writing transfer continuously only with small buffer system*/
ft_void_t               Ft_Gpu_Hal_StartTransfer(Ft_Gpu_Hal_Context_t *host,FT_GPU_TRANSFERDIR_T rw,ft_uint32_t addr);
ft_uint8_t              Ft_Gpu_Hal_Transfer8(Ft_Gpu_Hal_Context_t *host,ft_uint8_t value);
ft_uint16_t             Ft_Gpu_Hal_Transfer16(Ft_Gpu_Hal_Context_t *host,ft_uint16_t value);
ft_uint32_t             Ft_Gpu_Hal_Transfer32(Ft_Gpu_Hal_Context_t *host,ft_uint32_t value);
ft_void_t               Ft_Gpu_Hal_EndTransfer(Ft_Gpu_Hal_Context_t *host);

/*Close and deinit apis*/
ft_void_t              Ft_Gpu_Hal_Close(Ft_Gpu_Hal_Context_t *host);
ft_void_t              Ft_Gpu_Hal_DeInit();

/*Helper function APIs Read*/
ft_uint8_t  Ft_Gpu_Hal_Rd8(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr);
ft_uint16_t Ft_Gpu_Hal_Rd16(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr);
ft_uint32_t Ft_Gpu_Hal_Rd32(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr);

/*Helper function APIs Write*/
ft_void_t Ft_Gpu_Hal_Wr8(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr, ft_uint8_t v);
ft_void_t Ft_Gpu_Hal_Wr16(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr, ft_uint16_t v);
ft_void_t Ft_Gpu_Hal_Wr32(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr, ft_uint32_t v);

/*******************************************************************************/
/*******************************************************************************/
/*APIs for coprocessor Fifo read/write and space management*/
ft_void_t Ft_Gpu_Hal_Updatecmdfifo(Ft_Gpu_Hal_Context_t *host,ft_uint32_t count);
ft_void_t Ft_Gpu_Hal_WrCmd32(Ft_Gpu_Hal_Context_t *host,ft_uint32_t cmd);
ft_void_t Ft_Gpu_Hal_WrCmdBuf(Ft_Gpu_Hal_Context_t *host,ft_uint8_t *buffer,ft_uint32_t count);
ft_void_t Ft_Gpu_Hal_WaitCmdfifo_empty(Ft_Gpu_Hal_Context_t *host);
ft_void_t Ft_Gpu_Hal_ResetCmdFifo(Ft_Gpu_Hal_Context_t *host);
ft_void_t Ft_Gpu_Hal_CheckCmdBuffer(Ft_Gpu_Hal_Context_t *host,ft_uint32_t count);

ft_void_t Ft_Gpu_Hal_ResetDLBuffer(Ft_Gpu_Hal_Context_t *host);

ft_void_t  Ft_Gpu_Hal_StartCmdTransfer(Ft_Gpu_Hal_Context_t *host,FT_GPU_TRANSFERDIR_T rw, ft_uint16_t count);

ft_void_t Ft_Gpu_Hal_Powercycle(Ft_Gpu_Hal_Context_t *host,ft_bool_t up);


/*******************************************************************************/
/* APIs related to fifo buffer management */
/* API to update the read pointer into the structure */
ft_void_t Ft_Fifo_Init(Ft_Fifo_t *pFifo,ft_uint32_t StartAddress,ft_uint32_t Length,ft_uint32_t HWReadRegAddress,ft_uint32_t HWWriteRegAddress);//Init all the parameters of fifo buffer
ft_void_t Ft_Fifo_Update(Ft_Gpu_Hal_Context_t *host,Ft_Fifo_t *pFifo);//update both the read and write pointers
ft_uint32_t Ft_Fifo_Write(Ft_Gpu_Hal_Context_t *host,Ft_Fifo_t *pFifo,ft_uint8_t *buffer,ft_uint32_t NumbytetoWrite);//just write and update the write register
ft_void_t Ft_Fifo_Write32(Ft_Gpu_Hal_Context_t *host,Ft_Fifo_t *pFifo,ft_uint32_t WriteWord);//just write one word and update the write register
ft_void_t Ft_Fifo_WriteWait(Ft_Gpu_Hal_Context_t *host,Ft_Fifo_t *pFifo,ft_uint8_t *buffer,ft_uint32_t Numbyte);//write and wait for the fifo to be empty
ft_uint32_t Ft_Fifo_GetFreeSpace(Ft_Gpu_Hal_Context_t *host,Ft_Fifo_t *pFifo);//get the free space in the fifo - make sure the return value is maximum of (LENGTH - 4)


/*******************************************************************************/
/*******************************************************************************/
/*APIs for Host Commands*/
typedef enum {
	FT_GPU_INTERNAL_OSC = 0x48, //default
	FT_GPU_EXTERNAL_OSC = 0x44,
}FT_GPU_PLL_SOURCE_T;


typedef enum {
	FT_GPU_PLL_48M = 0x62,  //default
	FT_GPU_PLL_36M = 0x61,
	FT_GPU_PLL_24M = 0x64,
}FT_GPU_PLL_FREQ_T;

typedef enum {
	FT_GPU_ACTIVE_M =  0x00,  
	FT_GPU_STANDBY_M = 0x41,//default
	FT_GPU_SLEEP_M =   0x42,
	FT_GPU_POWERDOWN_M = 0x50,
}FT_GPU_POWER_MODE_T;


#ifdef FT_81X_ENABLE
	typedef enum {
		FT_GPU_SYSCLK_DEFAULT = 0x00,  //default 60mhz
		FT_GPU_SYSCLK_72M = 0x06, 
		FT_GPU_SYSCLK_60M = 0x05,  
		FT_GPU_SYSCLK_48M = 0x04,  
		FT_GPU_SYSCLK_36M = 0x03,
		FT_GPU_SYSCLK_24M = 0x02,
	}FT_GPU_81X_PLL_FREQ_T;

	typedef enum{
		FT_GPU_MAIN_ROM = 0x80,  //main graphicas ROM used 
		FT_GPU_RCOSATAN_ROM = 0x40,  //line slope table used for 
		FT_GPU_SAMPLE_ROM = 0x20,  //JA samples
		FT_GPU_JABOOT_ROM = 0x10, //JA microcode
		FT_GPU_J1BOOT_ROM = 0x08, //J1 microcode
		FT_GPU_ADC = 0x01,  //
		FT_GPU_POWER_ON_ROM_AND_ADC = 0x00,  //specify this element to power on all ROMs and ADCs
	}FT_GPU_81X_ROM_AND_ADC_T;

	typedef enum{
		FT_GPU_5MA = 0x00,  //default current
		FT_GPU_10MA = 0x01,
		FT_GPU_15MA = 0x02,
		FT_GPU_20MA = 0x03,
	}FT_GPU_81X_GPIO_DRIVE_STRENGTH_T;

	typedef enum{
		FT_GPU_GPIO0 = 0x00,
		FT_GPU_GPIO1 = 0x04,
		FT_GPU_GPIO2 = 0x08,
		FT_GPU_GPIO3 = 0x0C,
		FT_GPU_GPIO4 = 0x10,
		FT_GPU_DISP = 0x20,
		FT_GPU_DE = 0x24,
		FT_GPU_VSYNC_HSYNC = 0x28,
		FT_GPU_PCLK = 0x2C,
		FT_GPU_BACKLIGHT = 0x30,
		FT_GPU_R_G_B = 0x34,
		FT_GPU_AUDIO_L = 0x38,
		FT_GPU_INT_N = 0x3C,
		FT_GPU_TOUCHWAKE = 0x40,
		FT_GPU_SCL = 0x44,
		FT_GPU_SDA = 0x48,
		FT_GPU_SPI_MISO_MOSI_IO2_IO3 = 0x4C,
	}FT_GPU_81X_GPIO_GROUP_T;

	#define FT_GPU_81X_RESET_ACTIVE 0x000268

	#define FT_GPU_81X_RESET_REMOVAL 0x002068
#endif

#define FT_GPU_CORE_RESET  (0x68)

/* Enums for number of SPI dummy bytes and number of channels */
typedef enum {
	FT_GPU_SPI_SINGLE_CHANNEL = 0,
	FT_GPU_SPI_DUAL_CHANNEL = 1,
	FT_GPU_SPI_QUAD_CHANNEL = 2,
}FT_GPU_SPI_NUMCHANNELS_T;
typedef enum {
	FT_GPU_SPI_ONEDUMMY = 1,
	FT_GPU_SPI_TWODUMMY = 2,
}FT_GPU_SPI_NUMDUMMYBYTES;

#define FT_SPI_ONE_DUMMY_BYTE	(0x00)
#define FT_SPI_TWO_DUMMY_BYTE	(0x04)
#define FT_SPI_SINGLE_CHANNEL	(0x00)
#define FT_SPI_DUAL_CHANNEL		(0x01)
#define FT_SPI_QUAD_CHANNEL		(0x02)

ft_int32_t hal_strlen(const ft_char8_t *s);
ft_void_t Ft_Gpu_Hal_Sleep(ft_uint32_t ms);
ft_void_t Ft_Gpu_ClockSelect(Ft_Gpu_Hal_Context_t *host,FT_GPU_PLL_SOURCE_T pllsource);
ft_void_t Ft_Gpu_PLL_FreqSelect(Ft_Gpu_Hal_Context_t *host,FT_GPU_PLL_FREQ_T freq);
ft_void_t Ft_Gpu_PowerModeSwitch(Ft_Gpu_Hal_Context_t *host,FT_GPU_POWER_MODE_T pwrmode);
ft_void_t Ft_Gpu_CoreReset(Ft_Gpu_Hal_Context_t *host);
ft_void_t Ft_Gpu_Hal_StartTransfer(Ft_Gpu_Hal_Context_t *host,FT_GPU_TRANSFERDIR_T rw,ft_uint32_t addr);
ft_void_t Ft_Gpu_Hal_WrMem(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr, const ft_uint8_t *buffer, ft_uint32_t length);
ft_void_t Ft_Gpu_Hal_WrMemFromFlash(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr,const ft_prog_uchar8_t *buffer, ft_uint32_t length);
ft_void_t Ft_Gpu_Hal_WrCmdBufFromFlash(Ft_Gpu_Hal_Context_t *host,FT_PROGMEM ft_prog_uchar8_t *buffer,ft_uint32_t count);
ft_void_t Ft_Gpu_Hal_RdMem(Ft_Gpu_Hal_Context_t *host,ft_uint32_t addr, ft_uint8_t *buffer, ft_uint32_t length);
ft_void_t Ft_Gpu_Hal_WaitLogo_Finish(Ft_Gpu_Hal_Context_t *host);
ft_uint8_t Ft_Gpu_Hal_TransferString(Ft_Gpu_Hal_Context_t *host,const ft_char8_t *string);
ft_void_t Ft_Gpu_HostCommand(Ft_Gpu_Hal_Context_t *host,ft_uint8_t cmd);
ft_void_t Ft_Gpu_HostCommand_Ext3(Ft_Gpu_Hal_Context_t *host,ft_uint32_t cmd);
ft_int32_t Ft_Gpu_Hal_Dec2Ascii(ft_char8_t *pSrc,ft_int32_t value);
ft_uint8_t Ft_Gpu_Hal_WaitCmdfifo_empty_status(Ft_Gpu_Hal_Context_t *host);
ft_void_t Ft_Gpu_Hal_WrCmdBuf_nowait(Ft_Gpu_Hal_Context_t *host,ft_uint8_t *buffer,ft_uint32_t count);
ft_uint16_t Ft_Gpu_Cmdfifo_Freespace(Ft_Gpu_Hal_Context_t *host);
ft_int16_t Ft_Gpu_Hal_SetSPI(Ft_Gpu_Hal_Context_t *host,FT_GPU_SPI_NUMCHANNELS_T numchnls,FT_GPU_SPI_NUMDUMMYBYTES numdummy);

#ifdef FT_81X_ENABLE
ft_void_t Ft_Gpu_81X_SelectSysCLK(Ft_Gpu_Hal_Context_t *host, FT_GPU_81X_PLL_FREQ_T freq);
ft_void_t Ft_GPU_81X_PowerOffComponents(Ft_Gpu_Hal_Context_t *host, ft_uint8_t val);
ft_void_t Ft_GPU_81X_PadDriveStrength(Ft_Gpu_Hal_Context_t *host, FT_GPU_81X_GPIO_DRIVE_STRENGTH_T strength, FT_GPU_81X_GPIO_GROUP_T group);
ft_void_t Ft_Gpu_81X_ResetActive(Ft_Gpu_Hal_Context_t *host);
ft_void_t Ft_Gpu_81X_ResetRemoval(Ft_Gpu_Hal_Context_t *host);
#endif

ft_uint32_t Ft_Gpu_CurrentFrequency(Ft_Gpu_Hal_Context_t *host);
ft_int32_t Ft_Gpu_ClockTrimming(Ft_Gpu_Hal_Context_t *host,ft_int32_t LowFreq);

#endif  /* FT_GPU_HAL_H */
