/**
 ******************************************************************************
 * @file    labdaq_filter.h
 * @brief   Real-time Digital Filtering and Signal Conditioning Engine
 *          Supports MAV, EMA, IIR Lowpass, Spike Suppression & Statistics.
 ******************************************************************************
 */

#ifndef INC_LABDAQ_FILTER_H_
#define INC_LABDAQ_FILTER_H_

#include "labdaq_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FILTER_TYPE_BYPASS = 0,
    FILTER_TYPE_MAV,       /* Moving Average */
    FILTER_TYPE_EMA,       /* Exponential Moving Average */
    FILTER_TYPE_IIR_LPF,   /* 2nd-Order Butterworth Low-Pass Filter */
    FILTER_TYPE_MEDIAN     /* 3-Point Spike Rejection */
} labdaq_filter_type_t;

/* Per-channel filter state */
typedef struct {
    labdaq_filter_type_t type;

    /* Moving average window */
    uint16_t  mav_history[LABDAQ_FILTER_MAX_WINDOW];
    uint32_t  mav_sum;
    uint8_t   mav_index;
    uint8_t   mav_window_size;

    /* EMA state */
    float     ema_alpha;
    float     ema_last_val;

    /* 2nd-order IIR Direct Form II state */
    float     iir_b0, iir_b1, iir_b2;
    float     iir_a1, iir_a2;
    float     iir_w1, iir_w2;

    /* Median / Spike rejection */
    uint16_t  prev_raw[3];

    /* Live Channel Statistics */
    float     min_val;
    float     max_val;
    float     mean_val;
    float     rms_val;
    uint32_t  sample_count;
} labdaq_channel_filter_t;

typedef struct {
    labdaq_channel_filter_t channels[LABDAQ_NUM_CHANNELS];
    bool enabled;
} labdaq_filter_engine_t;

/**
 * @brief  Initialize filtering engine for all 16 channels
 * @param  engine Pointer to filter engine
 */
void LABDAQ_Filter_Init(labdaq_filter_engine_t *engine);

/**
 * @brief  Configure filter type and parameters for a specific channel
 * @param  engine Pointer to filter engine
 * @param  channel Channel index (0..15)
 * @param  type Filter type
 * @param  param Primary parameter (e.g. window size or alpha)
 */
void LABDAQ_Filter_ConfigureChannel(labdaq_filter_engine_t *engine, 
                                    uint8_t channel, 
                                    labdaq_filter_type_t type, 
                                    float param);

/**
 * @brief  Configure Butterworth 2nd-order lowpass filter coefficients
 * @param  engine Pointer to filter engine
 * @param  channel Channel index (0..15)
 * @param  cutoff_hz Cutoff frequency in Hz
 * @param  sample_rate_hz Sampling rate in Hz
 */
void LABDAQ_Filter_SetButterworthLPF(labdaq_filter_engine_t *engine,
                                    uint8_t channel,
                                    float cutoff_hz,
                                    float sample_rate_hz);

/**
 * @brief  Process a single raw sample through channel filter pipeline
 * @param  engine Pointer to filter engine
 * @param  channel Channel index (0..15)
 * @param  raw_input Raw 12-bit ADC reading
 * @return Filtered output value (float)
 */
float LABDAQ_Filter_ProcessSample(labdaq_filter_engine_t *engine, 
                                  uint8_t channel, 
                                  uint16_t raw_input);

/**
 * @brief  Process an entire frame of 16 channels in place or to output buffer
 * @param  engine Pointer to filter engine
 * @param  raw_frame Input 16-channel array
 * @param  filtered_out Output 16-channel float array
 */
void LABDAQ_Filter_ProcessFrame(labdaq_filter_engine_t *engine,
                                const uint16_t *raw_frame,
                                float *filtered_out);

/**
 * @brief  Reset statistics accumulators (Min, Max, RMS) for all channels
 * @param  engine Pointer to filter engine
 */
void LABDAQ_Filter_ResetStats(labdaq_filter_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_FILTER_H_ */
