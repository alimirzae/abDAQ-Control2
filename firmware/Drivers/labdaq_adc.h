/**
 ******************************************************************************
 * @file    labdaq_adc.h
 * @brief   High-Speed ADC Driver with DMA for LabDAQ-Control
 ******************************************************************************
 */

#ifndef INC_LABDAQ_ADC_H_
#define INC_LABDAQ_ADC_H_

#include "labdaq_config.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LABDAQ_ADC_OK = 0,
    LABDAQ_ADC_ERROR_INIT,
    LABDAQ_ADC_ERROR_DMA,
    LABDAQ_ADC_TIMEOUT
} labdaq_adc_status_t;

typedef struct {
    ADC_HandleTypeDef   hadc;
    DMA_HandleTypeDef   hdma_adc;
    TIM_HandleTypeDef   htim_trigger;
    uint32_t            sampling_rate_hz;
    volatile bool       dma_half_complete;
    volatile bool       dma_full_complete;
    float               vref_mv;
    float               calib_offset_mv;
    float               calib_gain;
} labdaq_adc_t;

/**
 * @brief  Initialize ADC1 peripheral, GPIO pin, DMA2 Stream0, and trigger timer
 * @param  dev Pointer to ADC device handle
 * @param  sampling_rate_hz Desired sampling rate in Hz
 * @return labdaq_adc_status_t
 */
labdaq_adc_status_t LABDAQ_ADC_Init(labdaq_adc_t *dev, uint32_t sampling_rate_hz);

/**
 * @brief  Start continuous DMA acquisition into specified buffer
 * @param  dev Pointer to ADC device handle
 * @param  buffer Destination memory buffer
 * @param  length Total buffer length in 16-bit words
 * @return labdaq_adc_status_t
 */
labdaq_adc_status_t LABDAQ_ADC_StartDMA(labdaq_adc_t *dev, uint16_t *buffer, uint32_t length);

/**
 * @brief  Stop ADC and DMA conversion
 * @param  dev Pointer to ADC device handle
 * @return labdaq_adc_status_t
 */
labdaq_adc_status_t LABDAQ_ADC_Stop(labdaq_adc_t *dev);

/**
 * @brief  Perform a single synchronous software conversion
 * @param  dev Pointer to ADC device handle
 * @return 12-bit raw ADC code
 */
uint16_t LABDAQ_ADC_ReadSingleRaw(labdaq_adc_t *dev);

/**
 * @brief  Convert 12-bit raw ADC code to millivolts with gain/offset calibration
 * @param  dev Pointer to ADC device handle
 * @param  raw 12-bit raw ADC reading
 * @return Calibrated voltage in millivolts (0.0 to 3300.0 mV)
 */
float LABDAQ_ADC_RawToMilliVolts(const labdaq_adc_t *dev, uint16_t raw);

/**
 * @brief  Reconfigure sampling timer frequency dynamically
 * @param  dev Pointer to ADC device handle
 * @param  rate_hz New target sampling frequency in Hz
 */
void LABDAQ_ADC_SetSamplingRate(labdaq_adc_t *dev, uint32_t rate_hz);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_ADC_H_ */
