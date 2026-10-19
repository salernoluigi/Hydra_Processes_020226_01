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
 * presso_support.h
 *
 *  Created on: Oct 16, 2026
 *      Author: fil
 */

#ifndef SUPPORT_PRESSO_SUPPORT_H_
#define SUPPORT_PRESSO_SUPPORT_H_

extern	void check_presso_usd(I2C_24xx_DriverStruct_t *i2c_24xx_Drv);
extern	void read_version_from_ee(I2C_24xx_DriverStruct_t *i2c_24xx_Drv,uint8_t ee_event);
extern	uint32_t rewrite_ee_presso_program_files(I2C_24xx_DriverStruct_t *i2c_24xx_Drv,uint8_t file_index);

#endif /* SUPPORT_PRESSO_SUPPORT_H_ */
