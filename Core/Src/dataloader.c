/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : dataloader.c
  * @brief          : Audio samples dataloader
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dataloader.h"

/* Private define ------------------------------------------------------------*/
#define BASE_WAVE_NAME			"sample.wav"
#define AUDIO_BUFFER_SIZE		1024

/* Private variables ---------------------------------------------------------*/
FIL File;
FRESULT res;
WAVE_FormatTypeDef waveformat;
uint8_t Audio_Buffer[AUDIO_BUFFER_SIZE];
static __IO uint32_t AudioRemSize = 0;


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Load Wav file Metadata.
  * @param  None
  * @retval None
  */
void ReadWavFile(void)
{
	UINT bytesread = 0;

	char* wavefilename = BASE_WAVE_NAME;

	if(f_open(&File, wavefilename, FA_READ) != FR_OK)
	{
		//TODO Handle Error
	}
	else
	{
		f_read(&File, &waveformat, sizeof(waveformat), &bytesread);
	}

	AudioRemSize = waveformat.FileSize;

	while(AudioRemSize != 0)
	{
		bytesread = 0;
		f_read(&File, &Audio_Buffer[0], AUDIO_BUFFER_SIZE, &bytesread);
		AudioRemSize -= bytesread;
	}
	f_close(&File);
}
