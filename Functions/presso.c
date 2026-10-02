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
 * presso.c
 *
 *  Created on: Sep 28, 2026
 *      Author: fil
 */
#include "main.h"
#include "../A_os_includes.h"
#ifndef	SAMPLE_PROCESSES_ENABLED
#include "../hydra.h"
#include "presso.h"

Presso_Sequencer_TypeDef	Presso_Sequencer;
extern	Presso_ee_TypeDef	Presso_programs[EE_PRESSO_NUM_PROGRAM];
Presso_ee_TypeDef	*Presso_programs_var;

Presso_GPIO_TypeDef			Presso_GPIO[PRESSO_GPIO_NR] =
{
		{
				.port = PRESSO_OUT0_PORT,
				.bit = PRESSO_OUT0_PIN,
		},
		{
				.port = PRESSO_OUT1_PORT,
				.bit = PRESSO_OUT1_PIN,
		},
		{
				.port = PRESSO_OUT2_PORT,
				.bit = PRESSO_OUT2_PIN,
		},
		{
				.port = PRESSO_OUT3_PORT,
				.bit = PRESSO_OUT3_PIN,
		},
		{
				.port = PRESSO_OUT4_PORT,
				.bit = PRESSO_OUT4_PIN,
		},
		{
				.port = PRESSO_OUT5_PORT,
				.bit = PRESSO_OUT5_PIN,
		},
		{
				.port = PRESSO_OUT6_PORT,
				.bit = PRESSO_OUT6_PIN,
		},
		{
				.port = PRESSO_OUT7_PORT,
				.bit = PRESSO_OUT7_PIN,
		},
		{
				.port = PRESSO_OUT8_PORT,
				.bit = PRESSO_OUT8_PIN,
		},
		{
				.port = PRESSO_OUT9_PORT,
				.bit = PRESSO_OUT9_PIN,
		},
};

Presso_HEAT_TypeDef Presso_HEAT[6] =
{
		{
				.port = PRESSO_HEAT0_PORT,
				.bit = PRESSO_HEAT0_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_3,
		},
		{
				.port = PRESSO_HEAT1_PORT,
				.bit = PRESSO_HEAT1_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_4,
		},
		{
				.port = PRESSO_HEAT2_PORT,
				.bit = PRESSO_HEAT2_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_1,
		},
		{
				.port = PRESSO_HEAT3_PORT,
				.bit = PRESSO_HEAT3_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_2,
		},
		{
				.port = PRESSO_HEAT4_PORT,
				.bit = PRESSO_HEAT4_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_3,
		},
		{
				.port = PRESSO_HEAT5_PORT,
				.bit = PRESSO_HEAT5_PIN,
				.Pwm_TIMx_Control_Channel = TIM_CHANNEL_4,
		},
};

void presso_sequencer_set_gpio(uint16_t outconfig)
{
uint8_t i;
	for(i=0;i<PRESSO_GPIO_NR;i++)
	{
		if ( outconfig & (1 << i ) )
			HAL_GPIO_WritePin(Presso_GPIO[i].port, Presso_GPIO[i].bit, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(Presso_GPIO[i].port, Presso_GPIO[i].bit, GPIO_PIN_RESET);
	}
}

void presso_sequencer_set_heater(uint16_t heater_nr , uint16_t heater_pw)
{
	pwm_set_width(&Presso_HEAT[heater_nr].Pwm_TIMx_Control,heater_pw*25,Presso_HEAT[heater_nr].Pwm_TIMx_Control_Channel);
}


static uint32_t presso_timeout_callback(uint32_t	val0,uint32_t	val1)
{
uint8_t i;

	presso_sequencer_set_gpio(0);
	store_counters();
	HYDRA_Struct.running_function = 0;

	for(i=0;i<PRESSO_HEAT_NR;i++)
		pwm_stop(&Presso_HEAT[i].Pwm_TIMx_Control,Presso_HEAT[i].Pwm_TIMx_Control_Channel);

	HYDRA_Struct.global_timer_status = GLOBAL_TIMER_STOP;
	global_timer_stop();
	for(i=0;i<PRESSO_GPIO_NR;i++)
		set_gpio_mode(Presso_GPIO[i].port, Presso_GPIO[i].bit,MODE_AF);
	for(i=0;i<PRESSO_HEAT_NR;i++)
		set_gpio_mode(Presso_HEAT[i].port, Presso_HEAT[i].bit,MODE_AF);

	HYDRA_Struct.global_timer_status = GLOBAL_TIMER_STOP;
	task_delay(50);
	send_numeric_dwin_packet(&Uart3_LCD_Drv,0x0682,AIRPEN_VP,0);
	task_delay(50);
	HYDRA_Struct.pump_status = 0;
	return 0;
}

static uint32_t presso_cleanup_function(uint32_t	val0,uint32_t	val1)
{
	presso_timeout_callback(0,0);
	return 0;
}

uint32_t presso_start(uint32_t level)
{
uint8_t i;
	if ( level )
	{
		HYDRA_Struct.running_function = PRESSO_OP;
		HYDRA_Struct.global_timer_status = GLOBAL_TIMER_RUNNING;
		Presso_programs_var = &Presso_programs[HYDRA_Struct.presso_program];

		Presso_Sequencer.current_step = 0;
		Presso_Sequencer.step_time = Presso_programs[HYDRA_Struct.presso_program].program_step_time; // called each 100 mSec
		Presso_Sequencer.number_of_steps = Presso_programs[HYDRA_Struct.presso_program].program_number_of_lines;
		HYDRA_Struct.global_timer_elapsed_callback = presso_timeout_callback;
		HYDRA_Struct.cleanup_function = presso_cleanup_function;

		for(i=0;i<PRESSO_HEAT_NR;i++)
		{
			if ( i < 2 )
				Presso_HEAT[i].Pwm_TIMx_Control = Pwm_TIM4_Control;
			else
				Presso_HEAT[i].Pwm_TIMx_Control = Pwm_TIM5_Control;
			pwm_stop(&Presso_HEAT[i].Pwm_TIMx_Control,Presso_HEAT[i].Pwm_TIMx_Control_Channel);
			pwm_set_width(&Presso_HEAT[i].Pwm_TIMx_Control,0,Presso_HEAT[i].Pwm_TIMx_Control_Channel);
			pwm_start(&Presso_HEAT[i].Pwm_TIMx_Control,Presso_HEAT[i].Pwm_TIMx_Control_Channel);
		}
		for(i=0;i<PRESSO_GPIO_NR;i++)
			set_gpio_mode(Presso_GPIO[i].port, Presso_GPIO[i].bit,MODE_OUTPUT);
		for(i=0;i<PRESSO_HEAT_NR;i++)
			set_gpio_mode(Presso_HEAT[i].port, Presso_HEAT[i].bit,MODE_AF);

		return Presso_Sequencer.step_time | Presso_Sequencer.number_of_steps; // if returns 0 the ee is uninitialized
	}
	else
	{
		Presso_Sequencer.state |= SEQUENCER_STOP_AT_END;
	}
	return 0;
}

void presso_sequencer_sm(void)
{
uint16_t value,heater,i;
	if ( HYDRA_Struct.running_function != PRESSO_OP )
		return;
	if ( Presso_Sequencer.step_time )
		Presso_Sequencer.step_time--;
	if ( Presso_Sequencer.step_time == 0 )
	{
		update_counters(PRESSO_OP);
		Presso_Sequencer.step_time = Presso_programs[HYDRA_Struct.presso_program].program_step_time;
		Presso_Sequencer.current_step ++;
		if ( Presso_Sequencer.current_step >= Presso_Sequencer.number_of_steps)
		{
			if (( Presso_Sequencer.state & SEQUENCER_STOP_AT_END) == SEQUENCER_STOP_AT_END)
			{
				Presso_Sequencer.current_step = 0;
				Presso_Sequencer.state = SEQUENCER_STATE_IDLE;
				presso_timeout_callback(0,0);
				return;
			}
			Presso_Sequencer.current_step = 0;
		}
		value = Presso_programs[HYDRA_Struct.presso_program].Presso_ee_line[Presso_Sequencer.current_step].gpio;
		presso_sequencer_set_gpio(value);
		for(i=0;i<PRESSO_HEAT_NR;i++)
		{
			heater = Presso_programs[HYDRA_Struct.presso_program].Presso_ee_line[Presso_Sequencer.current_step].heater_values[i];
			presso_sequencer_set_heater(i,heater);
		}

	}
}


#endif //#ifndef	SAMPLE_PROCESSES_ENABLED

