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

#define	xmodem_rx_data_area		0x30000000
#define	xmodem_rx_data_len		0x2ffff

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

#define AUDIO_BUF_SIZE 4096
uint16_t AudioBuffer[AUDIO_BUF_SIZE] ;

__attribute__ ((aligned (32)))	DAC_DriverStruct_t DAC_Drv =
{
	.dac = &hdac1,
	.dac_timer = &htim7,
	.sample_rate = DEFAULT_SAMPLE_FREQUENCY,
	.dac_buffer = AudioBuffer,
	.volume = 0.5F,
	.channel = DAC_CHANNEL_1,
	.dac_buf_size = AUDIO_BUF_SIZE,
	.flags = DAC_FLAGS_WAKEUP,
};

uint8_t		xmodem_rx_usb_enable;
uint8_t		xmodem_rx_usb_enable_poll;
uint8_t		tim_downscale=0;

void hydra_process_2_EE_USB_init(uint32_t process_id)
{
	usb_device_driver_register(&USB_Drv);
	dac_register(&DAC_Drv);
}

#define	PRC2_TICK	100
#define	XMO_TOUT	200
uint8_t	audio_enabled = 0;
extern	char SDPath[4];   /* SD logical drive path */
extern	FATFS SDFatFS;    /* File system object for SD logical drive */
uint8_t isPlaying= 0 , sdMounted;
FIL wav_file;
char wav_filename[32];

void hydra_process_2_EE_USB(uint32_t process_id)
{
uint32_t	wakeup,flags;
uint8_t		cntr = 0,xmo_res,xmo_timeout = 30;

	if ( i2c_24xx_register(&i2c_24xx_Drv) == 0 )
		HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_PRESENT;
	create_timer(TIMER_ID_0,PRC2_TICK,TIMERFLAGS_FOREVER | TIMERFLAGS_ENABLED);
	sprintf(wav_filename,"/ALARM/s2k44100.wav");

	if ( f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) == 0 )
    	sdMounted = 1;

	xmodem_rx_usb_enable = 0;
	xmodem_rx_usb_enable_poll = 0;
	xmodem_rx_init((uint8_t *)xmodem_rx_data_area,xmodem_rx_data_len);

	dac_init(&DAC_Drv);

	while(1)
	{
		wait_event(EVENT_TIMER | EVENT_I2C1_IRQ | EVENT_USB_DEVICE_IRQ  | EVENT_DAC_IRQ);
		get_wakeup_flags(&wakeup,&flags);
		if (( wakeup & WAKEUP_FROM_TIMER) == WAKEUP_FROM_TIMER)
		{
			cntr++;
			read_version_from_ee(&i2c_24xx_Drv,cntr);
			if ( cntr == 10)
				check_presso_usd(&i2c_24xx_Drv);
			if ( cntr == 19)
			{
				if ( sdMounted )
					if ( WAV_Player_Init(&DAC_Drv,wav_filename) == 0 )
						isPlaying = 1;
			}
			if ( cntr == 20)
			{
				audio_enabled = 1;
				cntr = 19;
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
				xmo_timeout--;
				if ( xmo_timeout == 0 )
				{
					xmodem_rx_usb_enable = 0;
					xmodem_rx_usb_enable_poll = 0;
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
			if ( xmodem_rx_usb_enable == 0 )
			{
				if ( parse_USB_packet(usb_rx_buffer,usb_get_rx_len(&USB_Drv)) > 1 )
				{
					xmodem_rx_usb_enable = 1;
					xmodem_rx_usb_enable_poll = 1;
					xmo_timeout = XMO_TOUT;
				}
			}
			else
			{
				xmo_res = xmodem_data_process((uint32_t *)&USB_Drv,xmodem_rx_usb_enable_poll,XMODEM_IF_USB,usb_rx_buffer);
				if ( xmo_res == X_EOT)
				{
					xmodem_rx_usb_enable = 0;
					bzero(usb_rx_buffer,XMODEM_LINE_LEN);
				}
				else
					xmodem_rx_usb_enable_poll = 0;
				xmo_timeout = XMO_TOUT;
			}
		}
		if (( wakeup & WAKEUP_FROM_DAC_IRQ) == WAKEUP_FROM_DAC_IRQ)
		{
		    if (isPlaying)
		    {
		        WAV_Player_Process(&DAC_Drv);
		    }
		}
	}
}
#endif //#ifndef SAMPLE_PROCESSES_ENABLED

