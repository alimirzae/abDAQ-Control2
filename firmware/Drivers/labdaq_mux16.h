/**
 ******************************************************************************
 * @file    labdaq_mux16.h
 * @brief   16-Channel Analog Multiplexer (CD74HC4067) Driver for STM32F407
 *          Controls S0-S3 channel select lines and /EN via Port E and level shifter.
 ******************************************************************************
 */

#ifndef INC_LABDAQ_MUX16_H_
#define INC_LABDAQ_MUX16_H_

#include "labdaq_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LABDAQ_MUX_OK = 0,
    LABDAQ_MUX_ERR_INVALID_CHANNEL,
    LABDAQ_MUX_ERR_NOT_INIT
} labdaq_mux_status_t;

typedef struct {
    uint8_t  current_channel;
    bool     enabled;
    uint32_t settling_delay_us;
} labdaq_mux_t;

/**
 * @brief  Initialize GPIO pins for 16-channel MUX control
 * @param  mux Pointer to MUX handle structure
 * @return labdaq_mux_status_t
 */
labdaq_mux_status_t LABDAQ_MUX16_Init(labdaq_mux_t *mux);

/**
 * @brief  Select an active channel on the 16:1 analog multiplexer (0 to 15)
 * @param  mux Pointer to MUX handle
 * @param  channel Channel index (0..15)
 * @return labdaq_mux_status_t
 */
labdaq_mux_status_t LABDAQ_MUX16_SelectChannel(labdaq_mux_t *mux, uint8_t channel);

/**
 * @brief  Enable or disable the analog multiplexer output
 * @param  mux Pointer to MUX handle
 * @param  enable true to enable (/EN low), false to disable (/EN high)
 */
void LABDAQ_MUX16_SetEnable(labdaq_mux_t *mux, bool enable);

/**
 * @brief  Switch to next channel sequentially in round-robin fashion
 * @param  mux Pointer to MUX handle
 * @return New selected channel (0..15)
 */
uint8_t LABDAQ_MUX16_NextChannel(labdaq_mux_t *mux);

/**
 * @brief  Hardware microsecond delay used during channel switching
 * @param  us Microseconds to wait
 */
void LABDAQ_MUX16_DelayUs(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_MUX16_H_ */
