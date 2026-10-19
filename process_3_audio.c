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
 * process_3_audio.c
 *
 *  Created on: Oct 15, 2026
 *      Author: fil
 */

#include "main.h"
#include "A_os_includes.h"

#ifndef	SAMPLE_PROCESSES_ENABLED
#include "hydra.h"

//#define PRC3_AUDIO	1
#ifdef PRC3_AUDIO

#include "main.h"
#include "ff.h"

// 2048 samples = 4096 bytes total.
// Each half-buffer is 1024 samples (2048 bytes), perfectly aligning with four 512-byte SD card sectors.
#define AUDIO_BUF_SIZE 2048
#define SAMPLE_FREQUENCY 44100

// Dynamic WAV header structure
typedef struct {
    char     riff[4];        // "RIFF"
    uint32_t fileSize;
    char     wave[4];        // "WAVE"
    char     fmt[4];         // "fmt "
    uint32_t fmtLen;
    uint16_t audioFormat;    // 1 = PCM (Uncompressed)
    uint16_t numChannels;    // 1 = Mono, 2 = Stereo
    uint32_t sampleRate;     // Sampling frequency (e.g., 16000, 44100)
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;  // 8 or 16-bit
    char     data[4];        // "data"
    uint32_t dataLen;
} WAV_HeaderTypeDef;

// Global Audio Variables
FIL wavFile;
WAV_HeaderTypeDef wavHeader;
UINT bytesRead;
uint8_t isPlaying = 0;

__attribute__((aligned(4))) uint16_t AudioBuffer[AUDIO_BUF_SIZE] ;

__attribute__ ((aligned (32)))	DAC_DriverStruct_t DAC_Drv =
{
	.dac = &hdac1,
	.dac_timer = &htim7,
	.dac_sample_frequency = SAMPLE_FREQUENCY,
	.dac_buffer = (int16_t *)AudioBuffer,
	.channel = DAC_CHANNEL_1,
	.alignment = DAC_ALIGN_12B_R,
	.len = AUDIO_BUF_SIZE,
	.flags = DAC_FLAGS_WAKEUP,
};

void Set_WAV_Sample_Rate(uint32_t sampleRate) {
    // Check your clock tree: TIM7 clock is typically APB1 Clock * 2
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq() * 2;

    // Calculate Auto-Reload Register (ARR) assuming a Prescaler (PSC) of 0
    uint32_t arr = (timer_clk / sampleRate) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim7, arr);
    __HAL_TIM_SET_COUNTER(&htim7, 0);
}

void Refill_Mono_Buffer(uint16_t *buffer_ptr, uint16_t num_samples) {
    // Read raw data from SD card. A void pointer is sent directly to avoid cast mismatching.
    // Length to read: number of samples * 2 bytes per sample.
    FRESULT res = f_read(&wavFile, (uint8_t *)buffer_ptr, num_samples * 2, &bytesRead);

    if (res != FR_OK) {
        // Handle read error (e.g., abort playback or flash an error LED)
        return;
    }

    // Cast pointer *after* reading to guarantee safe structural array indexing
    int16_t *signed_ptr = (int16_t*)buffer_ptr;

    for (int i = 0; i < num_samples; i++) {
        int16_t signed_sample = signed_ptr[i];

        // 1. Convert PCM signed (-32768 to 32767) to unsigned (0 to 65535)
        // 2. Downsample 16-bit scale to 12-bit DAC scale by shifting right 4 bits
        buffer_ptr[i] = (uint16_t)(signed_sample + 32768) >> 4;
    }

    // End of File (EOF) Handling
    if (bytesRead < (num_samples * 2)) {
        HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
        HAL_TIM_Base_Stop(&htim7);
        f_close(&wavFile);
        isPlaying = 0;
    }
}

void Start_Mono_Playback(const char* filename) {
    // Ensure we don't interrupt an active track unsafely
    if (isPlaying) {
        HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
        HAL_TIM_Base_Stop(&htim7);
        f_close(&wavFile);
        isPlaying = 0;
    }

    if (f_open(&wavFile, filename, FA_READ) == FR_OK) {
        // Read 44-byte standard header layout
        f_read(&wavFile, &wavHeader, sizeof(WAV_HeaderTypeDef), &bytesRead);

        // Safety Verification: Ensure file is uncompressed PCM and purely Mono
        if (wavHeader.audioFormat != 1 || wavHeader.numChannels != 1) {
            f_close(&wavFile);
            return; // Exit if formatting does not match constraints
        }

        // Update TIM7 speed to file frequency (e.g., 16000, 44100)
        Set_WAV_Sample_Rate(wavHeader.sampleRate);

        // Prime the double buffer cleanly via array slicing logic
        Refill_Mono_Buffer(&AudioBuffer[0], AUDIO_BUF_SIZE / 2);
        Refill_Mono_Buffer(&AudioBuffer[AUDIO_BUF_SIZE / 2], AUDIO_BUF_SIZE / 2);

        isPlaying = 1;

        // Activate internal hardware timers
        HAL_TIM_Base_Start(&htim7);

        // Start Circular DMA stream on DAC Channel 1 (12-bit Right Aligned mapping)
        HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*)AudioBuffer, AUDIO_BUF_SIZE, DAC_ALIGN_12B_R);
    }
}


void process_3_init(uint32_t process_id)
{
	dac_register(&DAC_Drv);
}

char wav_filename[32];
FATFS SDFatFSA;           // Global FatFs file system object
char SDPathA[4];          // SD logical drive path (e.g., "0:/")
uint8_t sdMounted = 0;   // Status flag

void process_3_audio(uint32_t process_id)
{
uint32_t	wakeup,flags;
uint8_t		aud_cntr = 0 , aud_stop=0;

	create_timer(TIMER_ID_0,100,TIMERFLAGS_FOREVER | TIMERFLAGS_ENABLED);
	dac_init(&DAC_Drv);
	dac_start(&DAC_Drv);
    FRESULT res = f_mount(&SDFatFSA, (TCHAR const*)SDPathA, 1);
    if (res == FR_OK) {
        sdMounted = 1;
        // Optional: blink a success LED or print confirmation
    } else {
        sdMounted = 0;
        // Error handling: 'res' will tell you why (e.g., 3 = FR_NOT_READY)
        // Light up a debug LED here so you know mounting failed!
    }
	HAL_TIM_Base_Stop(&htim7);
	sprintf(wav_filename,"/ALARM/s1k44100.wav");

	while(1)
	{
		wait_event(EVENT_TIMER | EVENT_DAC_IRQ);
		get_wakeup_flags(&wakeup,&flags);
		if (( wakeup & WAKEUP_FROM_TIMER) == WAKEUP_FROM_TIMER)
		{
			aud_cntr++;
			if ( aud_cntr == 20 )
			{
				HAL_GPIO_WritePin(PA_SHDN_GPIO_Port, PA_SHDN_Pin, GPIO_PIN_RESET);
				Start_Mono_Playback(wav_filename);
			}
			if ( aud_cntr >= 22 )
			{
				aud_cntr = 21;
			}
			if ( aud_stop == 1)
			{
				HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
				f_close(&wavFile);
				aud_stop = 0;
			}
		}
		if (( wakeup & WAKEUP_FROM_DAC_IRQ) == WAKEUP_FROM_DAC_IRQ)
		{
		    if (isPlaying)
			{
				if ((DAC_Drv.status & DAC_STATUS_HALF) == DAC_STATUS_HALF)
			        Refill_Mono_Buffer(&AudioBuffer[0], AUDIO_BUF_SIZE / 2);
				else  if ((DAC_Drv.status & DAC_STATUS_FULL) == DAC_STATUS_FULL)
			        Refill_Mono_Buffer(&AudioBuffer[AUDIO_BUF_SIZE / 2], AUDIO_BUF_SIZE / 2);
			}
		}
	}
}

#else //#ifdef PRC3_AUDIO
void process_3_init(uint32_t process_id)
{

}
void process_3_audio(uint32_t process_id)
{
	wait_event(HW_SLEEP_FOREVER);
}
#endif

#endif // #ifndef	SAMPLE_PROCESSES_ENABLED
