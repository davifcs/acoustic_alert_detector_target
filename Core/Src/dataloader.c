/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : dataloader.c
  * @brief          : Audio samples dataloader
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dataloader.h"
#include "app_x-cube-ai.h"
#include <aiTestUtility.h>

/* Private define ------------------------------------------------------------*/
#define BASE_WAVE_NAME		"sample"
#define FILE_EXTENSION		".wav"
#define SIGNAL_LEN			14700
#define FFT_LEN				1024
#define FRAME_LEN 			FFT_LEN
#define AUDIO_BUFFER_SIZE	2048
#define HOP_LEN				512U
#define NUM_MELS			30U
#define NUM_COLS			27U
#define NUM_MEL_COEFS   	968U

/* Private variables ---------------------------------------------------------*/
FIL File;
FRESULT res;
WAVE_FormatTypeDef waveformat;
static uint8_t Audio_Buffer[AUDIO_BUFFER_SIZE];
static __IO uint32_t AudioRemSize = 0;

static arm_rfft_fast_instance_f32 S_Rfft;
static MelFilterTypeDef           S_MelFilter;
static SpectrogramTypeDef         S_Spectr;
static MelSpectrogramTypeDef      S_MelSpectr;
static float32_t aColBuffer[NUM_MELS];
static float32_t pSpectrogram[NUM_COLS * NUM_MELS];
float32_t aSpectrScratchBuffer[FFT_LEN];


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Load Wav file Metadata.
  * @param  None
  * @retval None
  */
float32_t* ReadWavFile(uint16_t file_index)
{
	UINT bytesread = 0;
	UINT header = 0;
	volatile UINT byteswriten = 0;
	volatile uint32_t frame_index = 0;
	uint32_t AudioRemSize = 0;
	int16_t aInWord;
	float32_t aInFrame[FRAME_LEN];
	const uint32_t num_frames = 1 + (SIGNAL_LEN - FRAME_LEN) / HOP_LEN;
	uint16_t index_len = 0;
	uint16_t n = file_index;

	do{n /= 10; ++ index_len;} while (n != 0);
	char wavefilename[sizeof(BASE_WAVE_NAME) + index_len];
	sprintf(wavefilename, "%s%u%s", BASE_WAVE_NAME, file_index, FILE_EXTENSION);

	if(f_open(&File, wavefilename, FA_READ) != FR_OK)
	{
		for(;;)
		{
			//TODO Handle Error
		}
	}
	else
	{
		f_read(&File, &waveformat, sizeof(waveformat), &bytesread);
	}

	header = bytesread;
	AudioRemSize = waveformat.FileSize - header;

	Preprocessing_Init();

	while(AudioRemSize >= AUDIO_BUFFER_SIZE)
	{
		bytesread = 0;
		f_lseek(&File, header + byteswriten);
		f_read(&File, &Audio_Buffer[0], AUDIO_BUFFER_SIZE, &bytesread);
		AudioRemSize -= bytesread/2;

		for(uint32_t i = 0; i < bytesread/2; i++)
		{
		    aInWord = Audio_Buffer[i * 2] | (Audio_Buffer[i * 2 + 1] << 8);
		    if (aInWord >= 0x8000) aInWord -= 0x10000;
		    aInFrame[i] = (float32_t) aInWord / (1 << 15);
		}

		MelSpectrogramColumn(&S_MelSpectr, aInFrame, aColBuffer);

		for (uint32_t i = 0; i < NUM_MELS; i++)
		{
			pSpectrogram[i * num_frames + frame_index] = aColBuffer[i];
		}

		frame_index++;
		byteswriten += bytesread/2;
	}
	f_close(&File);
	PowerTodB(pSpectrogram);
	return pSpectrogram;
}


void Preprocessing_Init(void)
{
  /* Init RFFT */
  arm_rfft_fast_init_f32(&S_Rfft, FFT_LEN);

   /* Init Spectrogram */
   S_Spectr.pRfft    = &S_Rfft;
   S_Spectr.Type     = SPECTRUM_TYPE_POWER;
   S_Spectr.pWindow  = (float32_t *) hannWin_1024;;
   S_Spectr.SampRate = waveformat.SampleRate;
   S_Spectr.FrameLen = FRAME_LEN;
   S_Spectr.FFTLen   = FFT_LEN;
   S_Spectr.pScratch = aSpectrScratchBuffer;

   /* Init Mel filter */
   S_MelFilter.pStartIndices = (uint32_t *) melFiltersStartIndices_1024_30;
   S_MelFilter.pStopIndices  = (uint32_t *) melFiltersStopIndices_1024_30;
   S_MelFilter.pCoefficients = (float32_t *) melFilterLut_1024_30;
   S_MelFilter.NumMels   = NUM_MELS;
   S_MelFilter.FFTLen    = FFT_LEN;
   S_MelFilter.SampRate  = waveformat.SampleRate;
   S_MelFilter.FMin      = 0.0;
   S_MelFilter.FMax      = S_MelFilter.SampRate / 2.0;
   S_MelFilter.Formula   = MEL_SLANEY;
   S_MelFilter.Normalize = 1;
   S_MelFilter.Mel2F     = 1;
   MelFilterbank_Init(&S_MelFilter);

  /* Init MelSpectrogram */
  S_MelSpectr.SpectrogramConf = &S_Spectr;
  S_MelSpectr.MelFilter       = &S_MelFilter;
}

void PowerTodB(float32_t *pSpectrogram)
{
  float32_t max_mel_energy = 0.0f;
  uint32_t i;

  /* Find MelEnergy Scaling factor */
  for (i = 0; i < NUM_MELS * NUM_COLS; i++) {
    max_mel_energy = (max_mel_energy > pSpectrogram[i]) ? max_mel_energy : pSpectrogram[i];
  }

  /* Scale Mel Energies */
  for (i = 0; i < NUM_MELS * NUM_COLS; i++) {
    pSpectrogram[i] /= max_mel_energy;
  }

  /* Convert power spectrogram to decibel */
  for (i = 0; i < NUM_MELS * NUM_COLS; i++) {
    pSpectrogram[i] = 10.0f * log10f(pSpectrogram[i]);
  }

  /* Threshold output to -80.0 dB */
  for (i = 0; i < NUM_MELS * NUM_COLS; i++) {
    pSpectrogram[i] = (pSpectrogram[i] < -80.0f) ? (-80.0f) : (pSpectrogram[i]);
  }
}
