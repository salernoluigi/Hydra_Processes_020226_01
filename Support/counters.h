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
 * counters.h
 *
 *  Created on: Aug 7, 2026
 *      Author: MARFIX
 */

#ifndef HYDRA_COUNTERS_H_
#define HYDRA_COUNTERS_H_

/*
 * Defined in ../hydra.h :
#define	GLOBAL_STOPPED	0
#define	GLOBAL_OP		1
#define	AIRPEN_OP		2
#define	HYDRAPEN_OP		3
#define	JETPEEL_OP		4
#define	LINFOCUP_OP		5
#define	MOUSSE_OP		6
#define	PRESSO_OP		7
#define	VORTEX_OP		8
*/

typedef struct
{
	uint32_t			op_time[9];
	uint8_t				time_to_update;
}Hydra_Counters_TypeDef;

#define	WRITE_AFTER_120_SEC		120
#endif /* HYDRA_COUNTERS_H_ */
