/**
 ******************************************************************************
 * @file    labdaq_adc.c
 * @brief   High-Speed ADC Driver with DMA for STM32F407 (ADC1 + TIM2 TRGO)
 ******************************************************************************
 */

#include "labdaq_adc.h"

labdaq_adc_status_t LABDAQ_ADC_Init(labdaq_adc_t *dev, uint32_t sampling_rate_hz)
{
    if (!dev) return LABDAQ_ADC_ERROR_INIT;

    dev->sampling_rate_hz = sampling_rate_hz;
    dev->vref_mv = LABDAQ_ADC_VREF_MV;
    dev->calib_offset_mv = 0.0f;
    dev->calib_gain = 1.0f;
    dev->dma_half_complete = false;
    dev->dma_full_complete = false;

    /* 1. Clocks Enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* 2. Configure Analog Pin PA4 (ADC1 Channel 4) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_ADC_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LABDAQ_ADC_PORT, &GPIO_InitStruct);

    /* 3. Configure DMA2 Stream 0 Channel 0 for ADC1 */
    dev->hdma_adc.Instance = DMA2_Stream0;
    dev->hdma_adc.Init.Channel = DMA_CHANNEL_0;
    dev->hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dev->hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    dev->hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    dev->hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dev->hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dev->hdma_adc.Init.Mode = DMA_CIRCULAR;
    dev->hdma_adc.Init.Priority = DMA_PRIORITY_HIGH;
    dev->hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&dev->hdma_adc) != HAL_OK) {
        return LABDAQ_ADC_ERROR_DMA;
    }

    __HAL_LINKDMA(&dev->hadc, DMA_Handle, dev->hdma_adc);

    /* Enable DMA2 Stream0 Interrupt */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* 4. Configure ADC1 */
    dev->hadc.Instance = ADC1;
    dev->hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4; /* 84MHz APB2 / 4 = 21MHz ADC clock */
    dev->hadc.Init.Resolution = ADC_RESOLUTION_12B;
    dev->hadc.Init.ScanConvMode = DISABLE;
    dev->hadc.Init.ContinuousConvMode = DISABLE; /* Driven by TIM2 trigger */
    dev->hadc.Init.DiscontinuousConvMode = DISABLE;
    dev->hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    dev->hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    dev->hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    dev->hadc.Init.NbrOfConversion = 1;
    dev->hadc.Init.DMAContinuousRequests = ENABLE;
    dev->hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&dev->hadc) != HAL_OK) {
        return LABDAQ_ADC_ERROR_INIT;
    }

    /* Configure ADC regular channel */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = LABDAQ_ADC_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    if (HAL_ADC_ConfigChannel(&dev->hadc, &sConfig) != HAL_OK) {
        return LABDAQ_ADC_ERROR_INIT;
    }

    /* 5. Configure Timer 2 for hardware trigger */
    LABDAQ_ADC_SetSamplingRate(dev, sampling_rate_hz);

    return LABDAQ_ADC_OK;
}

void LABDAQ_ADC_SetSamplingRate(labdaq_adc_t *dev, uint32_t rate_hz)
{
    if (!dev) return;
    if (rate_hz < LABDAQ_MIN_SAMPLE_RATE) rate_hz = LABDAQ_MIN_SAMPLE_RATE;
    if (rate_hz > LABDAQ_MAX_SAMPLE_RATE * LABDAQ_NUM_CHANNELS) {
        rate_hz = LABDAQ_MAX_SAMPLE_RATE * LABDAQ_NUM_CHANNELS;
    }

    dev->sampling_rate_hz = rate_hz;

    /* Timer 2 runs on APB1 timer clock = 84 MHz */
    uint32_t timer_clk = 84000000;
    uint32_t prescaler = 0;
    uint32_t period = (timer_clk / rate_hz) - 1;

    /* If period exceeds 16-bit register (TIM2 is 32-bit on STM32F407) */
    dev->htim_trigger.Instance = TIM2;
    dev->htim_trigger.Init.Prescaler = prescaler;
    dev->htim_trigger.Init.CounterMode = TIM_COUNTERMODE_UP;
    dev->htim_trigger.Init.Period = period;
    dev->htim_trigger.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    dev->htim_trigger.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&dev->htim_trigger);

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&dev->htim_trigger, &sMasterConfig);
}

labdaq_adc_status_t LABDAQ_ADC_StartDMA(labdaq_adc_t *dev, uint16_t *buffer, uint32_t length)
{
    if (!dev || !buffer || length == 0) return LABDAQ_ADC_ERROR_DMA;

    dev->dma_half_complete = false;
    dev->dma_full_complete = false;

    if (HAL_ADC_Start_DMA(&dev->hadc, (uint32_t *)buffer, length) != HAL_OK) {
        return LABDAQ_ADC_ERROR_DMA;
    }

    /* Start Timer 2 trigger */
    HAL_TIM_Base_Start(&dev->htim_trigger);

    return LABDAQ_ADC_OK;
}

labdaq_adc_status_t LABDAQ_ADC_Stop(labdaq_adc_t *dev)
{
    if (!dev) return LABDAQ_ADC_ERROR_INIT;

    HAL_TIM_Base_Stop(&dev->htim_trigger);
    HAL_ADC_Stop_DMA(&dev->hadc);

    return LABDAQ_ADC_OK;
}

uint16_t LABDAQ_ADC_ReadSingleRaw(labdaq_adc_t *dev)
{
    if (!dev) return 0;

    HAL_ADC_Start(&dev->hadc);
    if (HAL_ADC_PollForConversion(&dev->hadc, 10) == HAL_OK) {
        return (uint16_t)HAL_ADC_GetValue(&dev->hadc);
    }
    return 0;
}

float LABDAQ_ADC_RawToMilliVolts(const labdaq_adc_t *dev, uint16_t raw)
{
    if (!dev) return 0.0f;
    float mv = ((float)raw / 4095.0f) * dev->vref_mv;
    mv = (mv * dev->calib_gain) + dev->calib_offset_mv;
    if (mv < 0.0f) mv = 0.0f;
    if (mv > dev->vref_mv) mv = dev->vref_mv;
    return mv;
}
