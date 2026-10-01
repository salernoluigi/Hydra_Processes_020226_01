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
 * counters.c
 *
 *  Created on: Aug 7, 2026
 *      Author: MARFIX
 */
#include "main.h"
#include "../A_os_includes.h"
#ifndef	SAMPLE_PROCESSES_ENABLED
#include "../hydra.h"
#include "counters.h"

Hydra_Counters_TypeDef	Hydra_Counters;
uint8_t					counter_buffer_tx[EE_COUNTERS_SIZE];

uint32_t	update_counters(uint8_t op)
{
	if ( op == 0 )
		return 0;
	Hydra_Counters.time_to_update ++;
	Hydra_Counters.op_time[GLOBAL_OP] ++;
	Hydra_Counters.op_time[op] ++;
	return Hydra_Counters.time_to_update;
}

uint32_t	store_counters(void)
{
	Hydra_Counters.time_to_update = 0;
	bzero(counter_buffer_tx,EE_COUNTERS_SIZE);
	i2c_24xx_write(&i2c_24xx_Drv,EE_COUNTERS_START,counter_buffer_tx, EE_COUNTERS_SIZE);
	return 0;
}

#endif //#ifndef	SAMPLE_PROCESSES_ENABLED

