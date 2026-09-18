/**
 ******************************************************************************
 * @file    labdaq_sampler.c
 * @brief   Deterministic Multi-Channel DAQ Sequencer & Cyclic Test Controller
 ******************************************************************************
 */

#include "labdaq_sampler.h"
#include <math.h>

bool LABDAQ_System_Init(labdaq_system_t *sys)
{
    if (!sys) return false;

    sys->sampling_rate_hz = LABDAQ_DEFAULT_SAMPLE_RATE;
    sys->frame_sequence = 0;
    sys->test_state = TEST_STATE_IDLE;
    sys->streaming_active = false;

    /* Initialize Cyclic Test Generator */
    sys->cyclic_gen.frequency_hz = LABDAQ_CYCLIC_DEFAULT_FREQ;
    sys->cyclic_gen.target_cycles = LABDAQ_CYCLIC_DEFAULT_CYCLES;
    sys->cyclic_gen.current_cycle = 0;
    sys->cyclic_gen.amplitude = 1.0f;
    sys->cyclic_gen.dc_offset = 0.0f;
    sys->cyclic_gen.actuator_state = false;
    sys->cyclic_gen.half_period_ms = 500;
    sys->cyclic_gen.last_toggle_tick = 0;

    /* 1. Initialize MUX */
    if (LABDAQ_MUX16_Init(&sys->mux) != LABDAQ_MUX_OK) {
        return false;
    }

    /* 2. Initialize ADC */
    if (LABDAQ_ADC_Init(&sys->adc, sys->sampling_rate_hz * LABDAQ_NUM_CHANNELS) != LABDAQ_ADC_OK) {
        return false;
    }

    /* 3. Initialize Buffer Manager */
    LABDAQ_Buffer_Init(&sys->buffer_mgr);

    /* 4. Initialize Filter Engine */
    LABDAQ_Filter_Init(&sys->filter_engine);

    /* Clear output pins */
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_RESET);

    sys->test_state = TEST_STATE_READY;
    return true;
}

void LABDAQ_System_StartAcquisition(labdaq_system_t *sys)
{
    if (!sys) return;
    sys->streaming_active = true;
    sys->start_timestamp_ms = HAL_GetTick();

    /* Start DMA continuous circular mode */
    LABDAQ_ADC_StartDMA(&sys->adc, sys->buffer_mgr.raw_dma_buffer, LABDAQ_RAW_BUFFER_SIZE);

    /* Turn on status LED and status GPIO */
    HAL_GPIO_WritePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_SET);
}

void LABDAQ_System_StopAcquisition(labdaq_system_t *sys)
{
    if (!sys) return;
    sys->streaming_active = false;
    LABDAQ_ADC_Stop(&sys->adc);

    HAL_GPIO_WritePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_RESET);
}

void LABDAQ_CyclicTest_Start(labdaq_system_t *sys, float frequency_hz, uint32_t target_cycles)
{
    if (!sys) return;

    if (frequency_hz < 0.01f) frequency_hz = 0.01f;
    if (frequency_hz > LABDAQ_CYCLIC_MAX_FREQ) frequency_hz = LABDAQ_CYCLIC_MAX_FREQ;

    sys->cyclic_gen.frequency_hz = frequency_hz;
    sys->cyclic_gen.target_cycles = target_cycles;
    sys->cyclic_gen.current_cycle = 0;
    sys->cyclic_gen.half_period_ms = (uint32_t)(500.0f / frequency_hz);
    if (sys->cyclic_gen.half_period_ms == 0) sys->cyclic_gen.half_period_ms = 1;
    sys->cyclic_gen.last_toggle_tick = HAL_GetTick();

    sys->test_state = TEST_STATE_RUNNING;

    /* Assert status pin */
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_SET);

    /* Start DAQ logging alongside cyclic test if not already running */
    if (!sys->streaming_active) {
        LABDAQ_System_StartAcquisition(sys);
    }
}

void LABDAQ_CyclicTest_Pause(labdaq_system_t *sys)
{
    if (!sys || sys->test_state != TEST_STATE_RUNNING) return;
    sys->test_state = TEST_STATE_PAUSED;
    /* De-energize actuator during pause for safety */
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, GPIO_PIN_RESET);
}

void LABDAQ_CyclicTest_Resume(labdaq_system_t *sys)
{
    if (!sys || sys->test_state != TEST_STATE_PAUSED) return;
    sys->test_state = TEST_STATE_RUNNING;
    sys->cyclic_gen.last_toggle_tick = HAL_GetTick();
}

void LABDAQ_CyclicTest_Abort(labdaq_system_t *sys)
{
    if (!sys) return;
    sys->test_state = TEST_STATE_IDLE;
    /* Safely shut off actuator and sync pulses */
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_RESET);
}

void LABDAQ_System_StepAcquisition(labdaq_system_t *sys)
{
    if (!sys) return;

    /* Sweep all 16 MUX channels synchronously */
    for (uint8_t ch = 0; ch < LABDAQ_NUM_CHANNELS; ch++) {
        LABDAQ_MUX16_SelectChannel(&sys->mux, ch);
        uint16_t raw = LABDAQ_ADC_ReadSingleRaw(&sys->adc);
        sys->latest_raw_frame[ch] = raw;

        /* Apply real-time digital filtering */
        float filtered = LABDAQ_Filter_ProcessSample(&sys->filter_engine, ch, raw);
        sys->latest_filtered_frame[ch] = filtered;

        /* Convert to millivolts */
        sys->latest_voltage_frame[ch] = LABDAQ_ADC_RawToMilliVolts(&sys->adc, (uint16_t)filtered);
    }

    sys->frame_sequence++;
}

void LABDAQ_System_Task(labdaq_system_t *sys)
{
    if (!sys) return;

    uint32_t now = HAL_GetTick();

    /* 1. Handle Cyclic Test Actuator Pulse & Cycle Counting */
    if (sys->test_state == TEST_STATE_RUNNING) {
        if ((now - sys->cyclic_gen.last_toggle_tick) >= sys->cyclic_gen.half_period_ms) {
            sys->cyclic_gen.last_toggle_tick = now;
            sys->cyclic_gen.actuator_state = !sys->cyclic_gen.actuator_state;

            /* Drive Actuator Pin via Level Shifter (PE14 / B7) */
            HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, 
                              sys->cyclic_gen.actuator_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

            /* Pulse SYNC_OUT pin on cycle start */
            if (sys->cyclic_gen.actuator_state) {
                HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_SET);
                sys->cyclic_gen.current_cycle++;

                /* Check target completion */
                if (sys->cyclic_gen.target_cycles > 0 && 
                    sys->cyclic_gen.current_cycle >= sys->cyclic_gen.target_cycles) {
                    sys->test_state = TEST_STATE_COMPLETED;
                    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_RESET);
                }
            } else {
                HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_RESET);
            }
        }
    }

    /* 2. Check DMA Ping-Pong Buffer for Completed Blocks */
    labdaq_buffer_ready_t ready_block = LABDAQ_Buffer_CheckReady(&sys->buffer_mgr);
    if (ready_block != BUFFER_BLOCK_NONE) {
        uint32_t sample_count = 0;
        const uint16_t *data = LABDAQ_Buffer_GetBlockData(&sys->buffer_mgr, ready_block, &sample_count);

        if (data && sample_count > 0) {
            /* Process newest samples into latest_raw_frame */
            uint32_t last_frame_idx = sample_count - LABDAQ_NUM_CHANNELS;
            for (uint8_t ch = 0; ch < LABDAQ_NUM_CHANNELS; ch++) {
                sys->latest_raw_frame[ch] = data[last_frame_idx + ch];
                float filtered = LABDAQ_Filter_ProcessSample(&sys->filter_engine, ch, sys->latest_raw_frame[ch]);
                sys->latest_filtered_frame[ch] = filtered;
                sys->latest_voltage_frame[ch] = LABDAQ_ADC_RawToMilliVolts(&sys->adc, (uint16_t)filtered);
            }
        }

        /* Mark block as processed */
        LABDAQ_Buffer_ReleaseBlock(&sys->buffer_mgr);
    }
}
