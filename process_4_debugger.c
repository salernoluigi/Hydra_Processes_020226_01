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
 * process_4_debugger.c
 *
 *  Created on: Oct 1, 2026
 *      Author: fil
 */

#include "main.h"
#include "A_os_includes.h"
#ifndef	SAMPLE_PROCESSES_ENABLED

#include "hydra.h"

#define DEBUG_FUNCTIONS 1

#ifdef DEBUG_FUNCTIONS
enum StateMachine {
  DBG_IDLE,
  DBG_PRESSO1_ON,
  DBG_PRESSO1_OFF,
  DBG_PRESSO2_ON,
  DBG_PRESSO2_OFF,
  DBG_PRESSO3_ON,
  DBG_PRESSO3_OFF,
};
uint8_t	sm = DBG_IDLE;

#define	CNTR_IDLE	2
#define	CNTR_RUN	10

void process_4_debugger(uint32_t process_id)
{
uint32_t	wakeup,flags;
uint8_t		cntr_stop=CNTR_IDLE;

	create_timer(TIMER_ID_0,1000,TIMERFLAGS_FOREVER | TIMERFLAGS_ENABLED);

	while(1)
	{
		wait_event(EVENT_TIMER);
		get_wakeup_flags(&wakeup,&flags);
		if (( wakeup & WAKEUP_FROM_TIMER) == WAKEUP_FROM_TIMER)
		{
			switch(sm)
			{
			case DBG_IDLE:
				sm = DBG_PRESSO1_ON;
				cntr_stop=CNTR_IDLE;
				break;
			case DBG_PRESSO1_ON:
				cntr_stop--;
				if ( cntr_stop == 0 )
				{
					HYDRA_Struct.presso_program = 0;
					presso_start(1);
					cntr_stop=CNTR_RUN;
					sm = DBG_PRESSO1_OFF;
				}
				break;
			case DBG_PRESSO1_OFF:
				if (  cntr_stop )
					cntr_stop--;
				if ( cntr_stop == 1 )
					presso_start(0);
				if ( cntr_stop == 0 )
				{
					if (HYDRA_Struct.running_function == 0 )
					{
						cntr_stop=CNTR_IDLE;
						sm = DBG_PRESSO2_ON;
					}
				}
				break;
			case DBG_PRESSO2_ON:
				if (  cntr_stop )
					cntr_stop--;
				if ( cntr_stop == 0 )
				{
					HYDRA_Struct.presso_program = 1;
					presso_start(1);
					cntr_stop=CNTR_RUN;
					sm = DBG_PRESSO2_OFF;
				}
				break;
			case DBG_PRESSO2_OFF:
				if (  cntr_stop )
					cntr_stop--;
				if ( cntr_stop == 1 )
					presso_start(0);
				if ( cntr_stop == 0 )
				{
					if (HYDRA_Struct.running_function == 0 )
					{
						cntr_stop=CNTR_IDLE;
						sm = DBG_PRESSO3_ON;
					}
				}
				break;
			case DBG_PRESSO3_ON:
				if (  cntr_stop )
					cntr_stop--;
				if ( cntr_stop == 0 )
				{
					HYDRA_Struct.presso_program = 2;
					presso_start(1);
					cntr_stop=CNTR_RUN;
					sm = DBG_PRESSO3_OFF;
				}
				break;
			case DBG_PRESSO3_OFF:
				if (  cntr_stop )
					cntr_stop--;
				if ( cntr_stop == 1 )
					presso_start(0);
				if ( cntr_stop == 0 )
				{
					if (HYDRA_Struct.running_function == 0 )
					{
						cntr_stop=CNTR_IDLE;
						sm = DBG_PRESSO1_ON;
					}
				}
				break;
			}
		}
	}
}
#else
void process_4_debugger(uint32_t process_id)
{
	wait_event(HW_SLEEP_FOREVER);
}
#endif //#ifdef DEBUG_FUNCTIONS
#endif // #ifndef	SAMPLE_PROCESSES_ENABLED

