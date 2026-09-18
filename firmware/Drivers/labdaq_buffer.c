/**
 ******************************************************************************
 * @file    labdaq_buffer.c
 * @brief   Circular Ping-Pong Buffer & DMA Double-Buffer Implementation
 ******************************************************************************
 */

#include "labdaq_buffer.h"

void LABDAQ_Buffer_Init(labdaq_buffer_manager_t *mgr)
{
    if (!mgr) return;
    memset(mgr->raw_dma_buffer, 0, sizeof(mgr->raw_dma_buffer));
    mgr->total_samples_collected = 0;
    mgr->total_frames_processed = 0;
    mgr->buffer_overruns = 0;
    mgr->active_ready_block = BUFFER_BLOCK_NONE;
    mgr->is_locked = false;
}

void LABDAQ_Buffer_OnHalfTransfer(labdaq_buffer_manager_t *mgr)
{
    if (!mgr) return;

    /* Detect overrun if previous block hasn't been consumed yet */
    if (mgr->active_ready_block != BUFFER_BLOCK_NONE) {
        mgr->buffer_overruns++;
    }

    mgr->active_ready_block = BUFFER_BLOCK_HALF;
    mgr->total_samples_collected += (LABDAQ_RAW_BUFFER_SIZE / 2);
}

void LABDAQ_Buffer_OnFullTransfer(labdaq_buffer_manager_t *mgr)
{
    if (!mgr) return;

    /* Detect overrun if previous block hasn't been consumed yet */
    if (mgr->active_ready_block != BUFFER_BLOCK_NONE) {
        mgr->buffer_overruns++;
    }

    mgr->active_ready_block = BUFFER_BLOCK_FULL;
    mgr->total_samples_collected += (LABDAQ_RAW_BUFFER_SIZE / 2);
}

labdaq_buffer_ready_t LABDAQ_Buffer_CheckReady(labdaq_buffer_manager_t *mgr)
{
    if (!mgr) return BUFFER_BLOCK_NONE;
    return (labdaq_buffer_ready_t)mgr->active_ready_block;
}

const uint16_t* LABDAQ_Buffer_GetBlockData(labdaq_buffer_manager_t *mgr, 
                                           labdaq_buffer_ready_t block, 
                                           uint32_t *num_samples)
{
    if (!mgr || !num_samples) return NULL;

    *num_samples = LABDAQ_RAW_BUFFER_SIZE / 2;

    if (block == BUFFER_BLOCK_HALF) {
        return &mgr->raw_dma_buffer[0];
    } else if (block == BUFFER_BLOCK_FULL) {
        return &mgr->raw_dma_buffer[LABDAQ_RAW_BUFFER_SIZE / 2];
    }

    *num_samples = 0;
    return NULL;
}

void LABDAQ_Buffer_ReleaseBlock(labdaq_buffer_manager_t *mgr)
{
    if (!mgr) return;
    mgr->active_ready_block = BUFFER_BLOCK_NONE;
    mgr->total_frames_processed += LABDAQ_BUFFER_BLOCK_FRAMES;
}
