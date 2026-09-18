/**
 ******************************************************************************
 * @file    labdaq_filter.c
 * @brief   Real-time Digital Filtering and Signal Conditioning Engine
 ******************************************************************************
 */

#include "labdaq_filter.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void LABDAQ_Filter_Init(labdaq_filter_engine_t *engine)
{
    if (!engine) return;
    memset(engine, 0, sizeof(labdaq_filter_engine_t));
    engine->enabled = true;

    for (uint8_t ch = 0; ch < LABDAQ_NUM_CHANNELS; ch++) {
        labdaq_channel_filter_t *cf = &engine->channels[ch];
        cf->type = FILTER_TYPE_MAV;
        cf->mav_window_size = LABDAQ_DEFAULT_MAV_WINDOW;
        cf->ema_alpha = LABDAQ_DEFAULT_EMA_ALPHA;
        cf->min_val = 4095.0f;
        cf->max_val = 0.0f;
        cf->mean_val = 0.0f;
        cf->rms_val = 0.0f;

        /* Default 2nd-order Butterworth coefficients at 1kHz sample rate, 50Hz cutoff */
        LABDAQ_Filter_SetButterworthLPF(engine, ch, 50.0f, 1000.0f);
    }
}

void LABDAQ_Filter_ConfigureChannel(labdaq_filter_engine_t *engine, 
                                    uint8_t channel, 
                                    labdaq_filter_type_t type, 
                                    float param)
{
    if (!engine || channel >= LABDAQ_NUM_CHANNELS) return;
    labdaq_channel_filter_t *cf = &engine->channels[channel];
    cf->type = type;

    if (type == FILTER_TYPE_MAV) {
        uint8_t win = (uint8_t)param;
        if (win < 1) win = 1;
        if (win > LABDAQ_FILTER_MAX_WINDOW) win = LABDAQ_FILTER_MAX_WINDOW;
        cf->mav_window_size = win;
        cf->mav_sum = 0;
        cf->mav_index = 0;
        memset(cf->mav_history, 0, sizeof(cf->mav_history));
    } else if (type == FILTER_TYPE_EMA) {
        if (param < 0.01f) param = 0.01f;
        if (param > 1.0f) param = 1.0f;
        cf->ema_alpha = param;
    }
}

void LABDAQ_Filter_SetButterworthLPF(labdaq_filter_engine_t *engine,
                                    uint8_t channel,
                                    float cutoff_hz,
                                    float sample_rate_hz)
{
    if (!engine || channel >= LABDAQ_NUM_CHANNELS) return;
    labdaq_channel_filter_t *cf = &engine->channels[channel];

    /* Calculate 2nd-order lowpass Butterworth filter coefficients via bilinear transform */
    float ita = 1.0f / tanf((float)M_PI * cutoff_hz / sample_rate_hz);
    float q = sqrtf(2.0f);
    float denom = 1.0f + q * ita + ita * ita;

    cf->iir_b0 = 1.0f / denom;
    cf->iir_b1 = 2.0f / denom;
    cf->iir_b2 = 1.0f / denom;
    cf->iir_a1 = 2.0f * (ita * ita - 1.0f) / denom;
    cf->iir_a2 = -(1.0f - q * ita + ita * ita) / denom;

    cf->iir_w1 = 0.0f;
    cf->iir_w2 = 0.0f;
}

static uint16_t median3(uint16_t a, uint16_t b, uint16_t c)
{
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
}

float LABDAQ_Filter_ProcessSample(labdaq_filter_engine_t *engine, 
                                  uint8_t channel, 
                                  uint16_t raw_input)
{
    if (!engine || channel >= LABDAQ_NUM_CHANNELS) return (float)raw_input;
    labdaq_channel_filter_t *cf = &engine->channels[channel];

    if (!engine->enabled || cf->type == FILTER_TYPE_BYPASS) {
        return (float)raw_input;
    }

    float output = 0.0f;

    switch (cf->type) {
        case FILTER_TYPE_MAV: {
            cf->mav_sum -= cf->mav_history[cf->mav_index];
            cf->mav_history[cf->mav_index] = raw_input;
            cf->mav_sum += raw_input;
            cf->mav_index = (cf->mav_index + 1) % cf->mav_window_size;
            output = (float)cf->mav_sum / (float)cf->mav_window_size;
            break;
        }

        case FILTER_TYPE_EMA: {
            if (cf->sample_count == 0) {
                cf->ema_last_val = (float)raw_input;
            } else {
                cf->ema_last_val = (cf->ema_alpha * (float)raw_input) + 
                                   ((1.0f - cf->ema_alpha) * cf->ema_last_val);
            }
            output = cf->ema_last_val;
            break;
        }

        case FILTER_TYPE_IIR_LPF: {
            /* Direct Form II Transposed */
            float x = (float)raw_input;
            float w0 = x - (cf->iir_a1 * cf->iir_w1) - (cf->iir_a2 * cf->iir_w2);
            output = (cf->iir_b0 * w0) + (cf->iir_b1 * cf->iir_w1) + (cf->iir_b2 * cf->iir_w2);
            cf->iir_w2 = cf->iir_w1;
            cf->iir_w1 = w0;
            break;
        }

        case FILTER_TYPE_MEDIAN: {
            cf->prev_raw[2] = cf->prev_raw[1];
            cf->prev_raw[1] = cf->prev_raw[0];
            cf->prev_raw[0] = raw_input;
            output = (float)median3(cf->prev_raw[0], cf->prev_raw[1], cf->prev_raw[2]);
            break;
        }

        default:
            output = (float)raw_input;
            break;
    }

    /* Update live channel statistics */
    if (output < cf->min_val) cf->min_val = output;
    if (output > cf->max_val) cf->max_val = output;

    cf->sample_count++;
    /* Incremental running mean */
    cf->mean_val += (output - cf->mean_val) / (float)cf->sample_count;

    return output;
}

void LABDAQ_Filter_ProcessFrame(labdaq_filter_engine_t *engine,
                                const uint16_t *raw_frame,
                                float *filtered_out)
{
    if (!engine || !raw_frame || !filtered_out) return;

    for (uint8_t ch = 0; ch < LABDAQ_NUM_CHANNELS; ch++) {
        filtered_out[ch] = LABDAQ_Filter_ProcessSample(engine, ch, raw_frame[ch]);
    }
}

void LABDAQ_Filter_ResetStats(labdaq_filter_engine_t *engine)
{
    if (!engine) return;
    for (uint8_t ch = 0; ch < LABDAQ_NUM_CHANNELS; ch++) {
        engine->channels[ch].min_val = 4095.0f;
        engine->channels[ch].max_val = 0.0f;
        engine->channels[ch].mean_val = 0.0f;
        engine->channels[ch].sample_count = 0;
    }
}
