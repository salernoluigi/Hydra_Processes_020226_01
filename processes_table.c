/*
 * processes_table.c
 *
 *  Created on: Sep 13, 2023
 *      Author: fil
 */
#include "main.h"
#include "A_os_includes.h"

#ifndef	SAMPLE_PROCESSES_ENABLED

extern	void hydra_process_1_main(uint32_t process_id);	//This is process1
extern	void hydra_process_1_main_init(uint32_t process_id);

extern	void hydra_process_2_USB(uint32_t process_id);	//This is process2
extern	void hydra_process_2_USB_init(uint32_t process_id);

extern	void hydra_process_3_EE(uint32_t process_id);	//This is process3
extern	void hydra_process_3_EE_init(uint32_t process_id);	//This is process3
extern	void process_4(uint32_t process_id);	//This is process4 of the application

extern	void process1_empty(uint32_t process_id);	//This is process1
extern	void process1_test(uint32_t process_id);	//This is process1

extern	void process1_empty_init(uint32_t process_id);

extern	void process2_empty(uint32_t process_id);	//This is process2
extern	void process2_empty_init(uint32_t process_id);


USRprcs_t	UserProcesses[USR_PROCESS_NUMBER] =
{
		{
				.user_init = hydra_process_1_main_init,
				.user_process = hydra_process_1_main,
				.stack_size = 4096,
		},
		{
				.user_init = hydra_process_2_USB_init,
				.user_process = hydra_process_2_USB,
				.stack_size = 4096,
		},
		{
				.user_init = hydra_process_3_EE_init,
				.user_process = hydra_process_3_EE,
				.stack_size = 4096,
		},
		{
				.user_process = process_4,
				.stack_size = 256,
		}
};
#endif // #ifndef	SAMPLE_PROCESSES_ENABLED

