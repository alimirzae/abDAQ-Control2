/**
 ******************************************************************************
 * @file    labdaq_mux16.c
 * @brief   16-Channel Analog Multiplexer (CD74HC4067) Implementation
 ******************************************************************************
 */

#include "labdaq_mux16.h"
#include "stm32f4xx_hal.h"

/* Fast bit-bang macro for Port E lower 8 high pins (PE8..PE11) */
#define MUX_PIN_MASK (LABDAQ_MUX_S0_PIN | LABDAQ_MUX_S1_PIN | LABDAQ_MUX_S2_PIN | LABDAQ_MUX_S3_PIN)

labdaq_mux_status_t LABDAQ_MUX16_Init(labdaq_mux_t *mux)
{
    if (!mux) return LABDAQ_MUX_ERR_NOT_INIT;

    /* Enable GPIOE clock */
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure S0, S1, S2, S3 and /EN pins as fast Push-Pull outputs */
    GPIO_InitStruct.Pin = LABDAQ_MUX_S0_PIN | LABDAQ_MUX_S1_PIN | 
                          LABDAQ_MUX_S2_PIN | LABDAQ_MUX_S3_PIN | 
                          LABDAQ_MUX_EN_PIN | LABDAQ_SYNC_OUT_PIN |
                          LABDAQ_ACTUATOR_PIN | LABDAQ_STATUS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(LABDAQ_MUX_PORT, &GPIO_InitStruct);

    /* Default to channel 0, enabled */
    mux->current_channel = 0;
    mux->enabled = true;
    mux->settling_delay_us = LABDAQ_MUX_SETTLE_US;

    LABDAQ_MUX16_SelectChannel(mux, 0);
    LABDAQ_MUX16_SetEnable(mux, true);

    return LABDAQ_MUX_OK;
}

labdaq_mux_status_t LABDAQ_MUX16_SelectChannel(labdaq_mux_t *mux, uint8_t channel)
{
    if (!mux) return LABDAQ_MUX_ERR_NOT_INIT;
    if (channel >= LABDAQ_NUM_CHANNELS) return LABDAQ_MUX_ERR_INVALID_CHANNEL;

    /* Write 4-bit channel code to PE8..PE11 */
    /* Atomic BSRR register write for minimal jitter */
    uint32_t set_mask = 0;
    uint32_t reset_mask = 0;

    if (channel & 0x01) set_mask |= LABDAQ_MUX_S0_PIN;
    else reset_mask |= LABDAQ_MUX_S0_PIN;

    if (channel & 0x02) set_mask |= LABDAQ_MUX_S1_PIN;
    else reset_mask |= LABDAQ_MUX_S1_PIN;

    if (channel & 0x04) set_mask |= LABDAQ_MUX_S2_PIN;
    else reset_mask |= LABDAQ_MUX_S2_PIN;

    if (channel & 0x08) set_mask |= LABDAQ_MUX_S3_PIN;
    else reset_mask |= LABDAQ_MUX_S3_PIN;

    LABDAQ_MUX_PORT->BSRR = (reset_mask << 16) | set_mask;
    mux->current_channel = channel;

    /* Settling delay for analog switch internal resistance and trace capacitance */
    if (mux->settling_delay_us > 0) {
        LABDAQ_MUX16_DelayUs(mux->settling_delay_us);
    }

    return LABDAQ_MUX_OK;
}

void LABDAQ_MUX16_SetEnable(labdaq_mux_t *mux, bool enable)
{
    if (!mux) return;
    mux->enabled = enable;

    /* CD74HC4067 /EN is Active Low: Low = Connected, High = High-Z disconnected */
    if (enable) {
        HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_MUX_EN_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_MUX_EN_PIN, GPIO_PIN_SET);
    }
}

uint8_t LABDAQ_MUX16_NextChannel(labdaq_mux_t *mux)
{
    if (!mux) return 0;
    uint8_t next = (mux->current_channel + 1) % LABDAQ_NUM_CHANNELS;
    LABDAQ_MUX16_SelectChannel(mux, next);
    return next;
}

void LABDAQ_MUX16_DelayUs(uint32_t us)
{
    /* Microsecond delay calibrated for STM32F407 running at 168 MHz (SYSCLK) */
    /* 1 loop iteration is approx 4 CPU cycles -> 168 / 4 = 42 iterations per us */
    uint32_t count = us * 42;
    while (count--) {
        __NOP();
    }
}
