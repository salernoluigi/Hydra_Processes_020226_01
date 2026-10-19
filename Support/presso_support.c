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
 * presso_support.c
 *
 *  Created on: Oct 16, 2026
 *      Author: fil
 */

#include "main.h"
#include "A_os_includes.h"
#ifndef	SAMPLE_PROCESSES_ENABLED

#include "../hydra.h"
#include "presso_support.h"
#include "ff.h"

uint8_t		boardname_restored = 0,boardname_checked = 0;
uint32_t	eeret_val;
extern	char					BoardNameVersion[EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE];
extern	Presso_ee_TypeDef		Presso_on_sd;
extern	Presso_ee_TypeDef		Presso_programs[EE_PRESSO_NUM_PROGRAM];
extern	Presso_sdcard_TypeDef	Presso_sdcard[EE_PRESSO_NUM_PROGRAM];

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

void read_version_from_ee(I2C_24xx_DriverStruct_t *i2c_24xx_Drv,uint8_t ee_event)
{
	if ( ee_event == 1)
	{
		if ( boardname_restored == 0)
		{
			bzero(BoardNameVersion,EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
			boardname_restored = 1;
			boardname_checked = 0;
			eeret_val = i2c_24xx_read(i2c_24xx_Drv,EE_BOARD_NAMEVERSION_ADDRESS,(uint8_t *)BoardNameVersion,EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
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
				eeret_val = i2c_24xx_write(i2c_24xx_Drv,EE_BOARD_NAMEVERSION_ADDRESS,(uint8_t *)BoardNameVersion, EE_BOARD_NAMEVERSION_SIZE+EE_COUNTERS_SIZE);
			}
			boardname_checked = 1;
			memcpy((uint8_t *)&Hydra_Counters,&BoardNameVersion[EE_BOARD_NAMEVERSION_SIZE],sizeof(Hydra_Counters_TypeDef));
		}
	}
}

uint32_t rewrite_ee_presso_program_files(I2C_24xx_DriverStruct_t *i2c_24xx_Drv,uint8_t file_index)
{
uint32_t	ee_address;
	ee_address = EE_PRESSO_PROGSTART+(file_index*EE_PRESSO_PROGRAM_SIZE);
	HYDRA_Struct.ee_sd_flags &= ~HYDRA_I2CMEM_WRITE_DONE;
	return i2c_24xx_write(i2c_24xx_Drv,ee_address,(uint8_t *)&Presso_on_sd, EE_PRESSO_PROGRAM_SIZE);
}

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


uint32_t sdload_presso_files(I2C_24xx_DriverStruct_t *i2c_24xx_Drv)
{
uint32_t	i,nfiles;
uint32_t 	char_processed = 0;

	/*
	if ( check_rewrite_programs() == 0 )
		return 0;
		*/
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
			rewrite_ee_presso_program_files(i2c_24xx_Drv,i);
		}
	}
	return nfiles;
}

void check_presso_usd(I2C_24xx_DriverStruct_t *i2c_24xx_Drv)
{
	if ( HAL_GPIO_ReadPin(SDMMC1_CD_GPIO_Port, SDMMC1_CD_Pin) == 0)
	{
		if ( (HYDRA_Struct.ee_sd_flags & HYDRA_SD_PRESENT) == 0 )
		{
			res = f_mount(&fs, "", 1);
			if ( res )
				return;
			HYDRA_Struct.ee_sd_flags |= HYDRA_SD_PRESENT;
			if ( (HYDRA_Struct.ee_sd_flags & HYDRA_SD_LOADED) == 0 )
			{
				sdload_presso_files(i2c_24xx_Drv);
				HYDRA_Struct.ee_sd_flags |= HYDRA_SD_LOADED;
				if ( (HYDRA_Struct.ee_sd_flags & (HYDRA_I2CMEM_READ_IN_PROGRESS | HYDRA_I2CMEM_LOADED) ) == 0 )
				{
					HYDRA_Struct.ee_sd_flags |= HYDRA_I2CMEM_READ_IN_PROGRESS;
					bzero((char *)&Presso_programs[0],sizeof(Presso_ee_TypeDef)*EE_PRESSO_NUM_PROGRAM);
					i2c_24xx_read(i2c_24xx_Drv,EE_PRESSO_PROGSTART,(uint8_t *)&Presso_programs,EE_PRESSO_PROGRAM_SIZE*(EE_PRESSO_NUM_PROGRAM));
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

#endif //#ifndef	SAMPLE_PROCESSES_ENABLED
