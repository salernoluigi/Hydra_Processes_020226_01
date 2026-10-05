/* 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Project : A_os
*/
/*
 * hydra_process_2_EE_USB.c
 *
 *  Created on: Oct 5, 2026
 *      Author: fil
 */

#include "main.h"
#include "A_os_includes.h"
#ifndef SAMPLE_PROCESSES_ENABLED
#include "hydra.h"
#include "Functions/presso.h"
#include "Support/counters.h"
#include "../A_os/kernel/A_exported_functions.h"

#include "ff.h"
extern	SD_HandleTypeDef hsd1;
extern	FRESULT list_directory(const char *path);
extern	uint32_t find_files_by_extension(const char *path,const char *extension);
extern					Hydra_Counters_TypeDef	Hydra_Counters;

char					BoardNameVersion[EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE];

Presso_ee_TypeDef		Presso_on_sd;
Presso_sdcard_TypeDef	Presso_sdcard[EE_PRESSO_NUM_PROGRAM];
HYDRA_USB_TypeDef		HYDRA_USB;

__attribute__ ((aligned (256)))	Presso_ee_TypeDef		Presso_programs[EE_PRESSO_NUM_PROGRAM];


uint8_t					read_done=0;

#define	XMODEM_AREA_LEN		32768
uint8_t	xmodem_area[XMODEM_AREA_LEN];

uint8_t	usb_rx_buffer[XMODEM_LINE_LEN];
uint8_t	usb_tx_buffer[XMODEM_LINE_LEN];

USB_DriverStruct_t	USB_Drv =
{
		.data = usb_rx_buffer,
		.data_index = 0,
		.requested_len = XMODEM_LINE_LEN,
		.usb_interface_class = USB_CDC_CLASS,
		.timeout = 50,
		.wakeup_id = WAKEUP_FROM_USB_DEVICE_IRQ,
};

void ee_callback(uint32_t parameter)
{
}

I2C_24xx_DriverStruct_t	i2c_24xx_Drv =
{
	.bus = &hi2c1,
	.i2c_scl_port = GPIOB,
	.i2c_scl_bit = 6,
	.device_address = I2C_24XX_ADDRESS,
	.device_address_size = I2C_MEMADD_SIZE_16BIT,
	.device_size = 65536,
	.flags = I2C_FLAGS_USES_READ_DMA | I2C_FLAGS_USES_WRITE_DMA | I2C_FLAGS_WAKEUP_ON_READ | I2C_FLAGS_WAKEUP_ON_WRITE | I2C_FLAGS_WAIT_ON_WRITE_COMPLETE | I2C_FLAGS_WAIT_ON_READ_COMPLETE,
	.wakeup_id = WAKEUP_FROM_I2C1_IRQ,
	.i2c_callback = ee_callback,
};

SDCARD_DriverStruct_t HydraSDCARD =
{
	.hsd = &hsd1,
	.sd_detect_port = SDMMC1_CD_GPIO_Port,
	.sd_detect_bit = SDMMC1_CD_Pin,
};

FATFS fs;
FIL file;
char path[] = "";
FRESULT res;
uint8_t buffer[2048];
UINT br;
char	outconfig[32];
char	csv_ee_line[64];

char		filesinfo[128];
uint32_t	file_number=0;

uint8_t		xmodem_rx_usb_enable;
uint8_t		xmodem_rx_usb_enable_poll;
uint8_t		tim_downscale=0;

// Helper function to check if a string ends with a specific extension (case-insensitive)
int has_extension(const char *filename, const char *ext)
{
    size_t len = strlen(filename);
    size_t ext_len = strlen(ext);

    if (len < ext_len)
    	return 0;

    // Compare the end of the filename with the target extension
    return (strcasecmp(filename + len - ext_len, ext) == 0);
}

uint32_t find_files_by_extension(const char *path,const char *extension)
{
FRESULT res;
DIR dir;
static FILINFO fno; // Static to keep stack footprint low

    res = f_opendir(&dir, path);
    if (res != FR_OK)
        return 1;

    file_number=0;
    for (;;)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
        	break;

        if (!(fno.fattrib & AM_DIR))
        {
            if (has_extension(fno.fname, extension))
            {
                sprintf(Presso_sdcard[file_number].program_name,"%s/%s", path,fno.fname);
                Presso_sdcard[file_number].len = fno.fsize;
                file_number++;
                if ( file_number >= EE_PRESSO_NUM_PROGRAM)
                	break;
            }
        }
    }
    f_closedir(&dir);
    return file_number;
}

static uint32_t find_csv_cr_subst(uint8_t *data_ptr)
{
uint32_t i;
	for(i=0;i<1024;i++)
	{
		if ( data_ptr[i] == '\n' )
		{
			csv_ee_line[i] = 0;
			return i+1;
		}
		else if ( data_ptr[i] == ',' )
			csv_ee_line[i] = ' ';
		else
			csv_ee_line[i] = data_ptr[i];

	}
	return 0;
}

uint16_t convert_gpio(void)
{
uint8_t		i;
uint16_t	gpioval = 0;

	for(i=0;i<16;i++)
	{
		if ( outconfig[i] == '1' )
			gpioval |= 1<<i;
	}
	return gpioval;
}

char	pname[EE_PROG_NAME_SIZE];

uint32_t decode_sd_csv(uint8_t *data_ptr,uint32_t data_len)
{
uint32_t 	cr_index = 0;
uint32_t 	char_processed = 0;
uint32_t 	line_index = 0;
int 		pnum,tmp1_int,tmp2_int;

	bzero(pname,EE_PROG_NAME_SIZE);
	char_processed = 0;
	while(1)
	{
		switch(*data_ptr)
		{
		case 'S' :
			cr_index = find_csv_cr_subst(data_ptr);
			if ( cr_index == 0 )
				return 0;
			pnum = sscanf((char *)csv_ee_line,"%c %s %d %d",
					&Presso_on_sd.program_valid_flag,
					pname,
					&tmp1_int,
					&tmp2_int);
			if ( pnum == 4 )
			{
				Presso_on_sd.program_number_of_lines = tmp1_int & 0xff;
				Presso_on_sd.program_step_time = tmp2_int & 0xff;
				if ( Presso_on_sd.program_valid_flag != EE_PROG_VALID_FLAG)
					return 0;
				strcat(Presso_on_sd.program_name , pname);
				if ( char_processed > sizeof(Presso_ee_TypeDef))
					return 0;
				char_processed +=cr_index;
				data_ptr += cr_index;
			}
			else
				return 0;
			break;
		case 'L' :
			cr_index = find_csv_cr_subst(data_ptr);
			if ( cr_index == 0 )
				return 0;
			pnum = sscanf(&csv_ee_line[2],"%d %d %d %d %d %d %d %d %s",
					(int *)&Presso_on_sd.Presso_ee_line[line_index].line_number,
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[0],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[1],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[2],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[3],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[4],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].heater_values[5],
					(int *)&Presso_on_sd.Presso_ee_line[line_index].sense_pressure,
					outconfig
					);
			if ( pnum == 9 )
			{
				Presso_on_sd.Presso_ee_line[line_index].gpio = convert_gpio();
				char_processed +=cr_index;
				line_index++;
				if ( line_index > EE_MAX_LINE_NUMBER )
					return 0;
				data_ptr += cr_index;
			}
			else
				return 0;
			break;
		case 'E' :
			cr_index = find_csv_cr_subst(data_ptr);
			if ( cr_index == 0 )
				return 0;
			char_processed +=cr_index;
			return char_processed;
			break;
		default:
			return 0;
		}
	}
	return 0;
}

uint32_t check_rewrite_programs(void)
{
	res = f_open(&file, "/PRESSO/UPD.UPD", FA_READ);
	if ( res )
		return 0;
	f_close(&file);
	return 1;
}

uint32_t rewrite_ee_presso_program_files(uint8_t file_index)
{
uint32_t	ee_address;
	ee_address = EE_PRESSO_PROGSTART+(file_index*EE_PRESSO_PROGRAM_SIZE);
	HYDRA_Struct.ee_sd_flags &= ~HYDRA_I2CMEM_WRITE_DONE;
	return i2c_24xx_write(&i2c_24xx_Drv,ee_address,(uint8_t *)&Presso_on_sd, EE_PRESSO_PROGRAM_SIZE);
}

uint32_t sdload(void)
{
uint32_t	i,nfiles;
uint32_t 	char_processed = 0;

	res = f_mount(&fs, "", 1);
	if ( res )
		return 0;
	if ( check_rewrite_programs() == 0 )
		return 0;
	nfiles = find_files_by_extension("/PRESSO",".csv");
	for(i=0;i<nfiles;i++)
	{
		if ( i>= EE_PRESSO_NUM_PROGRAM)
			return 0;
		if(strlen(Presso_sdcard[i].program_name) != 0)
		{
			res = f_open(&file, Presso_sdcard[i].program_name, FA_READ);
			if (res != FR_OK)
				return nfiles;
			bzero(buffer, sizeof(buffer));
			bzero((uint8_t *)&Presso_on_sd, sizeof(Presso_ee_TypeDef));
			res = f_read(&file, buffer, sizeof(buffer), &br);
			f_close(&file);

			char_processed=decode_sd_csv(buffer,br);
			if ( char_processed ==0 )
				return 0;
			rewrite_ee_presso_program_files(i);
		}
	}
	return nfiles;
}

uint8_t		boardname_restored = 0,boardname_checked = 0;
uint32_t	eeret_val;

void read_version(uint8_t ee_event)
{
	if ( ee_event == 1)
	{
		if ( boardname_restored == 0)
		{
			boardname_restored = 1;
			boardname_checked = 0;
			eeret_val = i2c_24xx_read(&i2c_24xx_Drv,EE_BOARD_NAMEVERSION_ADDRESS,(uint8_t *)BoardNameVersion,EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
		}
	}
	if ( ee_event == 5)
	{
		if (( boardname_restored == 1) && ( boardname_checked == 0))
		{
			if ( strcmp(BoardNameVersion,BOARD_NAMEVERSION))
			{
				bzero(BoardNameVersion,EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
				sprintf((char *)BoardNameVersion,BOARD_NAMEVERSION);
				eeret_val = i2c_24xx_write(&i2c_24xx_Drv,EE_BOARD_NAMEVERSION_ADDRESS,(uint8_t *)BoardNameVersion, EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
			}
			boardname_checked = 1;
			memcpy((uint8_t *)&Hydra_Counters,&BoardNameVersion[EE_BOARD_NAMEVERSION_SIZE],sizeof(Hydra_Counters_TypeDef));
		}
	}
}

void check_usd(void)
{
	if ( HAL_GPIO_ReadPin(SDMMC1_CD_GPIO_Port, SDMMC1_CD_Pin) == 0)
	{
		if ( (HYDRA_Struct.ee_sd_flags & HYDRA_SD_PRESENT) == 0 )
		{
			HYDRA_Struct.ee_sd_flags |= HYDRA_SD_PRESENT;
			if ( (HYDRA_Struct.ee_sd_flags & HYDRA_SD_LOADED) == 0 )
			{
				sdload();
				HYDRA_Struct.ee_sd_flags |= HYDRA_SD_LOADED;
				if ( (HYDRA_Struct.ee_sd_flags & (HYDRA_I2CMEM_READ_IN_PROGRESS | HYDRA_I2CMEM_LOADED) ) == 0 )
				{
					HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_READ_IN_PROGRESS;
					i2c_24xx_read(&i2c_24xx_Drv,EE_PRESSO_PROGSTART,(uint8_t *)&Presso_programs,EE_PRESSO_PROGRAM_SIZE*(EE_PRESSO_NUM_PROGRAM));
				}
			}
		}
	}
	else
	{
		HYDRA_Struct.ee_sd_flags &= ~HYDRA_SD_PRESENT;
		HYDRA_Struct.ee_sd_flags &= ~HYDRA_SD_LOADED;
	}
}

void hydra_process_2_EE_USB_init(uint32_t process_id)
{
	usb_device_driver_register(&USB_Drv);
}

uint32_t	size_struct;
void hydra_process_2_EE_USB(uint32_t process_id)
{
uint32_t	wakeup,flags;
uint8_t		cntr = 0;

	sdcard_register(&HydraSDCARD);
	if ( i2c_24xx_register(&i2c_24xx_Drv) == 0 )
		HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_PRESENT;
	create_timer(TIMER_ID_0,100,TIMERFLAGS_FOREVER | TIMERFLAGS_ENABLED);
	bzero((char *)&Presso_programs[0],sizeof(Presso_ee_TypeDef)*EE_PRESSO_NUM_PROGRAM);
	bzero(BoardNameVersion,EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
	size_struct = sizeof(Presso_ee_TypeDef);

	while(1)
	{
		wait_event(EVENT_TIMER | EVENT_I2C1_IRQ | EVENT_USB_DEVICE_IRQ);
		get_wakeup_flags(&wakeup,&flags);
		if (( wakeup & WAKEUP_FROM_TIMER) == WAKEUP_FROM_TIMER)
		{
			cntr++;
			read_version(cntr);
			if ( cntr == 20)
			{
				cntr = 10;
				check_usd();
			}
			if ( xmodem_rx_usb_enable == 1 )
			{
				if ( xmodem_rx_usb_enable_poll	 == 1 )
				{
					tim_downscale ++;
					if ( tim_downscale > 10 )
					{
						xmodem_data_process((uint32_t *)&USB_Drv,xmodem_rx_usb_enable_poll,XMODEM_IF_USB,usb_rx_buffer);
						tim_downscale = 0;
					}
				}
			}
		}
		if (( wakeup & WAKEUP_FROM_I2C1_IRQ) == WAKEUP_FROM_I2C1_IRQ)
		{
			if (( HYDRA_Struct.ee_sd_flags & HYDRA_I2CMEM_PRESENT ) == HYDRA_I2CMEM_PRESENT)
			{
				if (( flags & WAKEUP_FLAGS_I2C_RX) == WAKEUP_FLAGS_I2C_RX)
				{
					if ( (HYDRA_Struct.ee_sd_flags & HYDRA_I2CMEM_READ_IN_PROGRESS) == HYDRA_I2CMEM_READ_IN_PROGRESS )
					{
						if ( (HYDRA_Struct.ee_sd_flags & HYDRA_I2CMEM_LOADED) == 0 )
						{
							HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_LOADED;
							HYDRA_Struct.ee_sd_flags &= ~HYDRA_I2CMEM_READ_IN_PROGRESS;
						}
					}
				}
				if (( flags & WAKEUP_FLAGS_I2C_TX) == WAKEUP_FLAGS_I2C_TX)
				{
					HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_WRITE_DONE;
				}
			}
		}
		if (( wakeup & WAKEUP_FROM_USB_DEVICE_IRQ) == WAKEUP_FROM_USB_DEVICE_IRQ)
		{
			if (( usb_rx_buffer[0] == '<') && ( usb_rx_buffer[1] == 'h'))
			{
				xmodem_rx_usb_enable = 1;
				xmodem_rx_usb_enable_poll = 1;
			}
			else
			{
				if (xmodem_data_process((uint32_t *)&USB_Drv,xmodem_rx_usb_enable_poll,XMODEM_IF_USB,usb_rx_buffer) == X_EOT)
				{
					xmodem_rx_usb_enable = 0;
					xmodem_rx_usb_enable_poll = 0;
				}
				else
				{
					xmodem_rx_usb_enable = 1;
					xmodem_rx_usb_enable_poll = 0;
					parse_USB_packet(usb_rx_buffer,usb_get_rx_len(&USB_Drv));
				}
			}
		}
	}
}
#endif //#ifndef SAMPLE_PROCESSES_ENABLED

