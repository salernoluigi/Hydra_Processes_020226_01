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
 * presso.h
 *
 *  Created on: Sep 28, 2026
 *      Author: fil
 */

#ifndef HYDRA_PRESSO_H_
#define HYDRA_PRESSO_H_

typedef struct
{
	uint8_t				current_step;
	uint8_t				number_of_steps;
	uint8_t				state;
	uint16_t			step_time;
}Presso_Sequencer_TypeDef;
/*state*/
#define	SEQUENCER_STATE_IDLE		0x00
#define	SEQUENCER_STATE_OPENING		0x01
#define	SEQUENCER_STATE_RUNNING		0x02
#define	SEQUENCER_STATE_FINISHED	0x04
#define	SEQUENCER_STATE_PAUSE		0x40
#define	SEQUENCER_STOP_AT_END		0x80

#define	PRESSO_GPIO_NR				10
#define	PRESSO_HEAT_NR				6

typedef struct
{
	GPIO_TypeDef 		*port;
	uint16_t 			bit;
}Presso_GPIO_TypeDef;

typedef struct
{
	GPIO_TypeDef 				*port;
	uint16_t 					bit;
	Pwm_Control_DriverStruct_t	Pwm_TIMx_Control;
	uint32_t 					Pwm_TIMx_Control_Channel;
}Presso_HEAT_TypeDef;

#define	PRESSO_OUT0_PORT		PIN_TIM1_CH1_GPIO_Port
#define	PRESSO_OUT0_PIN			PIN_TIM1_CH1_Pin
#define	PRESSO_OUT1_PORT		PIN_TIM1_CH2_GPIO_Port
#define	PRESSO_OUT1_PIN			PIN_TIM1_CH2_Pin
#define	PRESSO_OUT2_PORT		PIN_TIM1_CH3_GPIO_Port
#define	PRESSO_OUT2_PIN			PIN_TIM1_CH3_Pin
#define	PRESSO_OUT3_PORT		PIN_TIM1_CH4_GPIO_Port
#define	PRESSO_OUT3_PIN			PIN_TIM1_CH4_Pin
#define	PRESSO_OUT4_PORT		PIN_TIM3_CH1_GPIO_Port
#define	PRESSO_OUT4_PIN			PIN_TIM3_CH1_Pin
#define	PRESSO_OUT5_PORT		PIN_TIM3_CH2_GPIO_Port
#define	PRESSO_OUT5_PIN			PIN_TIM3_CH2_Pin
#define	PRESSO_OUT6_PORT		PIN_TIM3_CH3_GPIO_Port
#define	PRESSO_OUT6_PIN			PIN_TIM3_CH3_Pin
#define	PRESSO_OUT7_PORT		PIN_TIM3_CH4_GPIO_Port
#define	PRESSO_OUT7_PIN			PIN_TIM3_CH4_Pin
#define	PRESSO_OUT8_PORT		PIN_TIM4_CH1_GPIO_Port
#define	PRESSO_OUT8_PIN			PIN_TIM4_CH1_Pin
#define	PRESSO_OUT9_PORT		PIN_TIM4_CH2_GPIO_Port
#define	PRESSO_OUT9_PIN			PIN_TIM4_CH2_Pin

#define	PRESSO_HEAT0_PORT		PIN_TIM4_CH3_GPIO_Port
#define	PRESSO_HEAT0_PIN		PIN_TIM4_CH3_Pin
#define	PRESSO_HEAT1_PORT		PIN_TIM4_CH4_GPIO_Port
#define	PRESSO_HEAT1_PIN		PIN_TIM4_CH4_Pin
#define	PRESSO_HEAT2_PORT		PIN_TIM5_CH1_GPIO_Port
#define	PRESSO_HEAT2_PIN		PIN_TIM5_CH1_Pin
#define	PRESSO_HEAT3_PORT		PIN_TIM5_CH2_GPIO_Port
#define	PRESSO_HEAT3_PIN		PIN_TIM5_CH2_Pin
#define	PRESSO_HEAT4_PORT		PIN_TIM5_CH3_GPIO_Port
#define	PRESSO_HEAT4_PIN		PIN_TIM5_CH3_Pin
#define	PRESSO_HEAT5_PORT		PIN_TIM5_CH4_GPIO_Port
#define	PRESSO_HEAT5_PIN		PIN_TIM5_CH4_Pin

#define	PRESSO_FLASH_SIZE	65536

typedef struct
{
	uint8_t			line_number;
	uint8_t			heater_values[6];
	uint8_t			sense_pressure;
	uint16_t		gpio;
}Presso_ee_line_TypeDef;


#define	EE_PROG_NAME_SIZE		9
#define	EE_MAX_LINE_NUMBER		24
#define	PRESSO_PADDING			(sizeof(char)*(1+EE_PROG_NAME_SIZE)+sizeof(uint8_t)+sizeof(uint8_t)+(sizeof(Presso_ee_line_TypeDef)*EE_MAX_LINE_NUMBER))
typedef struct
{
	char					program_valid_flag;						// 1 S
	char					program_name[EE_PROG_NAME_SIZE];		// 9 BIO
	uint8_t					program_number_of_lines;				// 1 20
	uint8_t					program_step_time;						// 1 2
	Presso_ee_line_TypeDef	Presso_ee_line[EE_MAX_LINE_NUMBER];		// 16*EE_MAX_LINE_NUMBER
	uint8_t                 reserved_padding[256-PRESSO_PADDING];
}Presso_ee_TypeDef;
#define	EE_PROG_VALID_FLAG			'S'

typedef struct
{
	char			program_name[26];
	uint32_t		len;
	uint8_t			program_number;
}Presso_sdcard_TypeDef;


extern	uint32_t presso_start(uint32_t level);
extern	void presso_sequencer_halt(void);
extern	void presso_sequencer_sm(void);



#endif /* HYDRA_PRESSO_H_ */
