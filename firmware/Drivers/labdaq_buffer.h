/**
 ******************************************************************************
 * @file    labdaq_buffer.h
 * @brief   Circular Ping-Pong Buffer & DMA Double-Buffer Manager for LabDAQ
 ******************************************************************************
 */

#ifndef INC_LABDAQ_BUFFER_H_
#define INC_LABDAQ_BUFFER_H_

#if __has_include("labdaq_config.h")
#include "labdaq_config.h"
#elif __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUFFER_BLOCK_NONE = 0,
    BUFFER_BLOCK_HALF,  /* First half is ready to process */
    BUFFER_BLOCK_FULL   /* Second half is ready to process */
} labdaq_buffer_ready_t;

typedef struct {
    uint32_t timestamp_us;
    uint32_t sequence_id;
    uint16_t channels[LABDAQ_NUM_CHANNELS];
} labdaq_frame_t;

typedef struct {
    uint16_t              raw_dma_buffer[LABDAQ_RAW_BUFFER_SIZE];
    uint32_t              total_samples_collected;
    uint32_t              total_frames_processed;
    uint32_t              buffer_overruns;
    volatile uint8_t      active_ready_block;
    bool                  is_locked;
} labdaq_buffer_manager_t;

/**
 * @brief  Initialize circular buffer structures and zero memory
 * @param  mgr Pointer to buffer manager
 */
void LABDAQ_Buffer_Init(labdaq_buffer_manager_t *mgr);

/**
 * @brief  Called by DMA Half-Transfer complete interrupt
 * @param  mgr Pointer to buffer manager
 */
void LABDAQ_Buffer_OnHalfTransfer(labdaq_buffer_manager_t *mgr);

/**
 * @brief  Called by DMA Full-Transfer complete interrupt
 * @param  mgr Pointer to buffer manager
 */
void LABDAQ_Buffer_OnFullTransfer(labdaq_buffer_manager_t *mgr);

/**
 * @brief  Check if a buffer block is available for processing
 * @param  mgr Pointer to buffer manager
 * @return BUFFER_BLOCK_HALF, BUFFER_BLOCK_FULL, or BUFFER_BLOCK_NONE
 */
labdaq_buffer_ready_t LABDAQ_Buffer_CheckReady(labdaq_buffer_manager_t *mgr);

/**
 * @brief  Get pointer to the completed raw data block for streaming or filtering
 * @param  mgr Pointer to buffer manager
 * @param  block BUFFER_BLOCK_HALF or BUFFER_BLOCK_FULL
 * @param  num_samples Pointer to store sample count
 * @return Pointer to contiguous 16-bit sample array
 */
const uint16_t* LABDAQ_Buffer_GetBlockData(labdaq_buffer_manager_t *mgr, 
                                           labdaq_buffer_ready_t block, 
                                           uint32_t *num_samples);

/**
 * @brief  Acknowledge and mark the block as consumed
 * @param  mgr Pointer to buffer manager
 */
void LABDAQ_Buffer_ReleaseBlock(labdaq_buffer_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_BUFFER_H_ */
