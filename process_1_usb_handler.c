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
 * process_1_usb_handler.c
 *
 *  Created on: Feb 1, 2025
 *      Author: fil
 */

#include "main.h"
#include "A_os_includes.h"
#include "membrane_global_includes.h"

extern	uint32_t	usb_driver_handle;

uint8_t pack_USB_packet(uint8_t *rx_buf,uint8_t len)
{
uint32_t	i;

	for(i=0;i<len;i++)
	{
		i &= (USB_BUF_LEN-1);
		if (( MembraneUSB.usb_flags & USB_FLAGS_HEADEROK) == 0 )
		{
			if ( rx_buf[i] == '<')
			{
				MembraneUSB.usb_packed_rx_buf[0] = rx_buf[i];
				MembraneUSB.usb_rx_buf_index = 1;
				MembraneUSB.usb_flags |= USB_FLAGS_HEADEROK;
			}
		}
		else
		{
			MembraneUSB.usb_packed_rx_buf[MembraneUSB.usb_rx_buf_index ] = rx_buf[i];
			if ( MembraneUSB.usb_packed_rx_buf[MembraneUSB.usb_rx_buf_index] == '>')
			{
				MembraneUSB.usb_flags |= USB_FLAGS_PKTCOMPLETE;
				MembraneUSB.usb_flags &= ~USB_FLAGS_HEADEROK;
				MembraneUSB.usb_rx_buf_len = MembraneUSB.usb_rx_buf_index;
				MembraneUSB.usb_rx_buf_index = 0;
				return	MembraneUSB.usb_rx_buf_len;
			}
			MembraneUSB.usb_rx_buf_index++;
		}
	}
	return 0;
}

uint8_t decode_USB_packet(uint8_t* Buf)
{
uint16_t	pnum;
char p0;
int	p1,p2,p3;

	pnum = sscanf((char * )Buf,"<%c%d%d%d", &p0, &p1,&p2,&p3);
	switch(pnum)
	{
	case	4:
		MembraneUSB.command_from_usb = p0;
		MembraneUSB.parameter1_from_usb = p1;
		MembraneUSB.parameter2_from_usb = p2;
		MembraneUSB.parameter3_from_usb = p3;
		break;
	case	3:
		MembraneUSB.command_from_usb = p0;
		MembraneUSB.parameter1_from_usb = p1;
		MembraneUSB.parameter2_from_usb = p2;
		break;
	case	2:
		MembraneUSB.command_from_usb = p0;
		MembraneUSB.parameter1_from_usb = p1;
		break;
	case	1:
		MembraneUSB.command_from_usb = p0;
		break;
	default:
		break;
	}
	return pnum;
}

extern	uint8_t	app_name[16];
extern	uint8_t	app_version[16];
extern	uint8_t	a_version[32];

extern	uint8_t	file_md5_chars[32];
extern	uint8_t		interval;

uint32_t usb_txed=0;
uint8_t parse_USB_packet(uint8_t* Buf)
{
uint8_t 	ret_val = PARSE_ERROR;

	MembraneUSB.usb_tx_buf_len = 0;
	switch(MembraneUSB.command_from_usb)
	{
	/* Data & info commands to sensors */
	case SENSORS_GETACQ_COMMAND:
		get_sensor_data(MembraneUSB.parameter1_from_usb,MembraneUSB.parameter2_from_usb);
		ret_val = PARSE_GETACQ;
		break;
	case SENSORS_GETSENSVERINFO:
		get_sensor_info(MembraneUSB.parameter1_from_usb,MembraneUSB.parameter2_from_usb);
		ret_val = PARSE_OK;
		break;
	case CONCENTRATOR_VERSION:
		sprintf((char *)MembraneUSB.usb_tx_buf,"%s %s %s",app_name , app_version, a_version);
		MembraneUSB.usb_tx_buf_len = strlen((char *)MembraneUSB.usb_tx_buf);
		ret_val = PARSE_OK;
		break;
	}
	if (( ret_val == PARSE_OK ) || ( ret_val == PARSE_GETACQ ))
	{
		if ( MembraneUSB.usb_tx_buf_len )
		{
			usb_txed++;
			usb_send(&USB_Drv,MembraneUSB.usb_tx_buf,MembraneUSB.usb_tx_buf_len);
		}
	}
	return ret_val;
}

void send_sensor_version_packet(uint8_t address,uint8_t type , char *packet)
{
	bzero(MembraneUSB.usb_tx_buf,USB_BUF_LEN);
	sprintf((char *)MembraneUSB.usb_tx_buf,"Sensor %d type %d %s",address,type,packet);
	MembraneUSB.usb_tx_buf_len = strlen((char *)MembraneUSB.usb_tx_buf);
	usb_send(&USB_Drv,MembraneUSB.usb_tx_buf,MembraneUSB.usb_tx_buf_len-1);
}

void send_sensor_status_packet(uint8_t address,uint8_t type , char *packet)
{
	bzero(MembraneUSB.usb_tx_buf,USB_BUF_LEN);
	sprintf((char *)MembraneUSB.usb_tx_buf,"Sensor %d type %d status %s",address,type,packet);
	MembraneUSB.usb_tx_buf_len = strlen((char *)MembraneUSB.usb_tx_buf);
	usb_send(&USB_Drv,MembraneUSB.usb_tx_buf,MembraneUSB.usb_tx_buf_len-1);
}

void send_sensor_update_progress(uint8_t address, uint32_t pkt_num)
{
	bzero(MembraneUSB.usb_tx_buf,USB_BUF_LEN);
	sprintf((char *)MembraneUSB.usb_tx_buf,"%d",(int )pkt_num);
	MembraneUSB.usb_tx_buf_len = strlen((char *)MembraneUSB.usb_tx_buf);
	usb_send(&USB_Drv,MembraneUSB.usb_tx_buf,MembraneUSB.usb_tx_buf_len);
}

void send_sensor_flash_download_status(char *packet)
{
	bzero(MembraneUSB.usb_tx_buf,USB_BUF_LEN);
	sprintf((char *)MembraneUSB.usb_tx_buf,"%s",packet);
	MembraneUSB.usb_tx_buf_len = strlen((char *)MembraneUSB.usb_tx_buf);
	usb_send(&USB_Drv,MembraneUSB.usb_tx_buf,MembraneUSB.usb_tx_buf_len);
}
