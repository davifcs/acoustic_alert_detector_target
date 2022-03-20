/**
  ******************************************************************************
  * @file    dataloader.h
  * @author  MCD Application Team
  * @brief   Header for dataloader.c module.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_DATALOADER_H_
#define INC_DATALOADER_H_

/* Includes ------------------------------------------------------------------*/
#include "fatfs.h"
#include "feature_extraction.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  BUFFER_OFFSET_NONE = 0,
  BUFFER_OFFSET_HALF,
  BUFFER_OFFSET_FULL,
}BUFFER_StateTypeDef;


typedef struct
{
  uint32_t   ChunkID;       /* 0 */
  uint32_t   FileSize;      /* 4 */
  uint32_t   FileFormat;    /* 8 */
  uint32_t   SubChunk1ID;   /* 12 */
  uint32_t   SubChunk1Size; /* 16*/
  uint16_t   AudioFormat;   /* 20 */
  uint16_t   NbrChannels;   /* 22 */
  uint32_t   SampleRate;    /* 24 */

  uint32_t   ByteRate;      /* 28 */
  uint16_t   BlockAlign;    /* 32 */
  uint16_t   BitPerSample;  /* 34 */
  uint32_t   SubChunk2ID;   /* 36 */
  uint32_t   SubChunk2Size; /* 40 */

}WAVE_FormatTypeDef;


/* Exported functions ------------------------------------------------------- */
void ReadWavFile(void);
void Preprocessing_Init(void);
void AudioPreprocessing_Run(int16_t *pInSignal, float32_t *pOut, uint32_t signal_len);
void PowerTodB(float32_t *pSpectrogram);


#endif /* INC_DATALOADER_H_ */
