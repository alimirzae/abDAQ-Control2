/**
 ******************************************************************************
 * @file    labdaq_sampler.h
 * @brief   Deterministic Multi-Channel DAQ Sequencer & Cyclic Test Controller
 ******************************************************************************
 */

#ifndef INC_LABDAQ_SAMPLER_H_
#define INC_LABDAQ_SAMPLER_H_

#if __has_include("labdaq_config.h")
#include "labdaq_config.h"
#elif __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#endif

#include "labdaq_mux16.h"
#include "labdaq_adc.h"
#include "labdaq_buffer.h"
#include "labdaq_filter.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEST_STATE_IDLE = 0,
    TEST_STATE_READY,
    TEST_STATE_RUNNING,
    TEST_STATE_PAUSED,
    TEST_STATE_COMPLETED,
    TEST_STATE_FAULT
} labdaq_test_state_t;

typedef struct {
    float    frequency_hz;
    uint32_t target_cycles;
    uint32_t current_cycle;
    float    amplitude;
    float    dc_offset;
    bool     actuator_state;
    uint32_t half_period_ms;
    uint32_t last_toggle_tick;
} labdaq_cyclic_generator_t;

typedef struct {
    labdaq_mux_t              mux;
    labdaq_adc_t              adc;
    labdaq_buffer_manager_t   buffer_mgr;
    labdaq_filter_engine_t    filter_engine;
    labdaq_cyclic_generator_t cyclic_gen;
    labdaq_test_state_t       test_state;
    uint32_t                  sampling_rate_hz;
    uint32_t                  frame_sequence;
    uint32_t                  start_timestamp_ms;
    uint16_t                  latest_raw_frame[LABDAQ_NUM_CHANNELS];
    float                     latest_filtered_frame[LABDAQ_NUM_CHANNELS];
    float                     latest_voltage_frame[LABDAQ_NUM_CHANNELS];
    bool                      streaming_active;
} labdaq_system_t;

/**
 * @brief  Initialize full DAQ system (MUX, ADC, Timers, Buffers, Filters)
 * @param  sys Pointer to master system handle
 * @return true if success, false otherwise
 */
bool LABDAQ_System_Init(labdaq_system_t *sys);

/**
 * @brief  Start high-speed continuous acquisition
 * @param  sys Pointer to system handle
 */
void LABDAQ_System_StartAcquisition(labdaq_system_t *sys);

/**
 * @brief  Stop acquisition
 * @param  sys Pointer to system handle
 */
void LABDAQ_System_StopAcquisition(labdaq_system_t *sys);

/**
 * @brief  Start cyclic mechanical/electrical fatigue test
 * @param  sys Pointer to system handle
 * @param  frequency_hz Test frequency (e.g. 1.0 Hz)
 * @param  target_cycles Target cycle count (e.g. 50000)
 */
void LABDAQ_CyclicTest_Start(labdaq_system_t *sys, float frequency_hz, uint32_t target_cycles);

/**
 * @brief  Pause cyclic test
 * @param  sys Pointer to system handle
 */
void LABDAQ_CyclicTest_Pause(labdaq_system_t *sys);

/**
 * @brief  Resume cyclic test
 * @param  sys Pointer to system handle
 */
void LABDAQ_CyclicTest_Resume(labdaq_system_t *sys);

/**
 * @brief  Emergency abort cyclic test and disable actuator
 * @param  sys Pointer to system handle
 */
void LABDAQ_CyclicTest_Abort(labdaq_system_t *sys);

/**
 * @brief  Execute single round-robin 16-channel acquisition step
 * @param  sys Pointer to system handle
 */
void LABDAQ_System_StepAcquisition(labdaq_system_t *sys);

/**
 * @brief  Main periodic task called from main loop (1 kHz or timer interrupt)
 * @param  sys Pointer to system handle
 */
void LABDAQ_System_Task(labdaq_system_t *sys);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_SAMPLER_H_ */
