/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : dataloader.c
  * @brief          : Audio samples dataloader
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dataloader.h"
#include <aiTestUtility.h>


/* Private define ------------------------------------------------------------*/
#define BASE_WAVE_NAME		"sample.wav"
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
static float32_t aWindowFuncBuffer[FRAME_LEN];
static float32_t aMelFilterCoefs[NUM_MEL_COEFS];
static uint32_t aMelFilterStartIndices[NUM_MELS];
static uint32_t aMelFilterStopIndices[NUM_MELS];
static float32_t aSpectrScratchBuffer[FFT_LEN];

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Load Wav file Metadata.
  * @param  None
  * @retval None
  */
void ReadWavFile(void)
{
	UINT bytesread = 0;
	volatile UINT byteswriten = 0;
	volatile uint32_t frame_index = 0;
	uint32_t AudioRemSize = 0;
	int16_t aInWord;
	float32_t aInFrame[FRAME_LEN];
	float32_t aColBuffer[NUM_MELS];
	float32_t aSpectrogram[NUM_MELS * NUM_COLS];
	const uint32_t num_frames = 1 + (SIGNAL_LEN - FRAME_LEN) / HOP_LEN;

	char* wavefilename = BASE_WAVE_NAME;

	if(f_open(&File, wavefilename, FA_READ) != FR_OK)
	{
		//TODO Handle Error
	}
	else
	{
		f_read(&File, &waveformat, sizeof(waveformat), &bytesread);
	}

	AudioRemSize = waveformat.FileSize - bytesread;

	Preprocessing_Init();

	while(AudioRemSize != 0)
	{
		bytesread = 0;
		f_read(&File, &Audio_Buffer[0], AUDIO_BUFFER_SIZE, &bytesread);
		bytesread /= 2;
		f_lseek(&File, bytesread);
		if(AudioRemSize > AUDIO_BUFFER_SIZE)
		{
			AudioRemSize -= bytesread;
		}
		else
		{
			AudioRemSize = 0;
		}

		for(uint32_t i = 0; i < bytesread/2; i++)
		{
		    aInWord = Audio_Buffer[i * 2] | (Audio_Buffer[i * 2 + 1] << 8);
		    if (aInWord >= 0x8000) aInWord -= 0x10000;
		    aInFrame[i] = (float32_t) aInWord / (1 << 15);
		}

		MelSpectrogramColumn(&S_MelSpectr, aInFrame, aColBuffer);

		for (uint32_t i = 0; i < NUM_MELS; i++)
		{
			aSpectrogram[i * num_frames + frame_index] = aColBuffer[i];
		}

		frame_index++;
		byteswriten += bytesread/2;
	}
	f_close(&File);

	PowerTodB(aSpectrogram);
	for(uint32_t i = 0;i < (NUM_COLS * NUM_MELS); i++)
	{
		LC_PRINT("%.4f, ", aSpectrogram[i]);
	}
}


void Preprocessing_Init(void)
{

  /* Init window function */
  if (Window_Init(aWindowFuncBuffer, FRAME_LEN, WINDOW_HANN) != 0)
  {
    while(1);
  }
  /* Init RFFT */
  arm_rfft_fast_init_f32(&S_Rfft, FFT_LEN);

  /* Init Spectrogram */
  S_Spectr.pRfft    = &S_Rfft;
  S_Spectr.Type     = SPECTRUM_TYPE_POWER;
  S_Spectr.pWindow  = aWindowFuncBuffer;
  S_Spectr.SampRate = waveformat.SampleRate;
  S_Spectr.FrameLen = FRAME_LEN;
  S_Spectr.FFTLen   = FFT_LEN;
  S_Spectr.pScratch = aSpectrScratchBuffer;

  /* Init Mel filter */
  S_MelFilter.pStartIndices = aMelFilterStartIndices;
  S_MelFilter.pStopIndices  = aMelFilterStopIndices;
  S_MelFilter.pCoefficients = aMelFilterCoefs;
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

void AudioPreprocessing_Run(int16_t *pInSignal, float32_t *pOut, uint32_t signal_len)
{
  float32_t aInFrame[FRAME_LEN];
  float32_t aColBuffer[NUM_MELS];

  const uint32_t num_frames = 1 + (signal_len - FRAME_LEN) / HOP_LEN;

  for (uint32_t frame_index = 0; frame_index < num_frames; frame_index++)
  {
    buf_to_float_normed(&pInSignal[HOP_LEN * frame_index], aInFrame, FRAME_LEN);
    MelSpectrogramColumn(&S_MelSpectr, aInFrame, aColBuffer);
    /* Reshape column into pOut */
    for (uint32_t i = 0; i < NUM_MELS; i++)
    {
      pOut[i * num_frames + frame_index] = aColBuffer[i];
    }
  }
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
