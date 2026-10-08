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
 * usb_support.c
 *
 *  Created on: Oct 5, 2026
 *      Author: fil
 */
#include "main.h"
#include "A_os_includes.h"
#ifndef	SAMPLE_PROCESSES_ENABLED

#include "../hydra.h"
#include "counters.h"

extern	HYDRA_USB_TypeDef		HYDRA_USB;

static uint8_t pack_USB_packet(uint8_t *rx_buf,uint8_t len)
{
uint32_t	i;

	for(i=0;i<len;i++)
	{
		i &= (USB_BUF_LEN-1);
		if (( HYDRA_USB.usb_flags & USB_FLAGS_HEADEROK) == 0 )
		{
			if ( rx_buf[i] == '<')
			{
				HYDRA_USB.usb_packed_rx_buf[0] = rx_buf[i];
				HYDRA_USB.usb_rx_buf_index = 1;
				HYDRA_USB.usb_flags |= USB_FLAGS_HEADEROK;
			}
		}
		else
		{
			HYDRA_USB.usb_packed_rx_buf[HYDRA_USB.usb_rx_buf_index ] = rx_buf[i];
			if ( HYDRA_USB.usb_packed_rx_buf[HYDRA_USB.usb_rx_buf_index] == '>')
			{
				HYDRA_USB.usb_flags |= USB_FLAGS_PKTCOMPLETE;
				HYDRA_USB.usb_flags &= ~USB_FLAGS_HEADEROK;
				HYDRA_USB.usb_rx_buf_len = HYDRA_USB.usb_rx_buf_index;
				HYDRA_USB.usb_rx_buf_index = 0;
				return	HYDRA_USB.usb_rx_buf_len;
			}
			HYDRA_USB.usb_rx_buf_index++;
		}
	}
	return 0;
}


static uint8_t decode_USB_packet(void)
{
uint16_t	pnum;
char p0;
int	p1,p2,p3;

	pnum = sscanf((char * )HYDRA_USB.usb_packed_rx_buf,"<%c%d%d%d", &p0, &p1,&p2,&p3);
	switch(pnum)
	{
	case	4:
		HYDRA_USB.command_from_usb = p0;
		HYDRA_USB.parameter1_from_usb = p1;
		HYDRA_USB.parameter2_from_usb = p2;
		HYDRA_USB.parameter3_from_usb = p3;
		break;
	case	3:
		HYDRA_USB.command_from_usb = p0;
		HYDRA_USB.parameter1_from_usb = p1;
		HYDRA_USB.parameter2_from_usb = p2;
		break;
	case	2:
		HYDRA_USB.command_from_usb = p0;
		HYDRA_USB.parameter1_from_usb = p1;
		break;
	case	1:
		HYDRA_USB.command_from_usb = p0;
		break;
	default:
		break;
	}
	return pnum;
}
/*
 * return values:
 * 0  : command accepted
 * 1  : no valid command
 * >1 : returns lenght of the xmodem set received
 */
uint32_t parse_USB_packet(uint8_t* Buf,uint8_t len)
{
uint32_t 	ret_val = 1;
uint16_t	pnum;

	if ( pack_USB_packet(Buf,len) == 0 )
		return 1;
	pnum = decode_USB_packet();
	if ( pnum == 0)
		return 1;
	HYDRA_USB.usb_tx_buf_len = 0;
	if (( HYDRA_USB.command_from_usb & 0x20) == 0)	// Monitor commands
	{
		switch(HYDRA_USB.command_from_usb)
		{
		case HYDRA_GETVERINFO:
			sprintf((char *)HYDRA_USB.usb_tx_buf,"%s %s\n\r",BOARD_NAMEVERSION,A_OS_VERSION);
			HYDRA_USB.usb_tx_buf_len = strlen((char *)HYDRA_USB.usb_tx_buf);
			ret_val = 0;
			break;
		case HYDRA_GETCOUNTERS:
			sprintf((char *)HYDRA_USB.usb_tx_buf,"%d %d %d %d %d %d %d %d\n\r",
					(int )Hydra_Counters.op_time[GLOBAL_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[AIRPEN_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[HYDRAPEN_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[JETPEEL_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[LINFOCUP_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[MOUSSE_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[PRESSO_OP]/COUNTERS_UNIT,
					(int )Hydra_Counters.op_time[VORTEX_OP]/COUNTERS_UNIT
				   );
			HYDRA_USB.usb_tx_buf_len = strlen((char *)HYDRA_USB.usb_tx_buf);
			ret_val = 0;
			break;
		case HYDRA_ACTIVE:
			sprintf((char *)HYDRA_USB.usb_tx_buf,"Active : %d\n\r",
							(int )HYDRA_Struct.running_function
						   );
					HYDRA_USB.usb_tx_buf_len = strlen((char *)HYDRA_USB.usb_tx_buf);
					ret_val = 0;
					break;
		case HYDRA_GOXMODEM :
			ret_val = HYDRA_USB.usb_xmodem_size = HYDRA_USB.parameter1_from_usb;
			break;
		}
	}
	else	// op commands
	{
		switch(HYDRA_USB.command_from_usb)
		{
		case HYDRA_OP_PRESSO:
			if ( pnum == 3)
			{
				HYDRA_Struct.presso_program = HYDRA_USB.parameter2_from_usb;
				if ( HYDRA_USB.parameter1_from_usb )
					sprintf((char *)HYDRA_USB.usb_tx_buf,"Presso program %d started\n\r",(int )HYDRA_Struct.presso_program);
				else
					sprintf((char *)HYDRA_USB.usb_tx_buf,"Presso program %d stopped\n\r",(int )HYDRA_Struct.presso_program);
				HYDRA_USB.usb_tx_buf_len = strlen((char *)HYDRA_USB.usb_tx_buf);
				presso_start( HYDRA_USB.parameter1_from_usb );
				ret_val = 0;
			}
			else
			{
				sprintf((char *)HYDRA_USB.usb_tx_buf,"Presso program wrong parameter number %d : <p 0|1[stop/start] 1[program number]>\n\r",(int )pnum);
				HYDRA_USB.usb_tx_buf_len = strlen((char *)HYDRA_USB.usb_tx_buf);
				ret_val = 0;
			}
			break;
		}

	}
	if ( ret_val == 0 )
	{
		if ( HYDRA_USB.usb_tx_buf_len )
		{
			usb_send(&USB_Drv,HYDRA_USB.usb_tx_buf,HYDRA_USB.usb_tx_buf_len);
		}
	}
	return ret_val;
}
#endif //#ifndef	SAMPLE_PROCESSES_ENABLED


