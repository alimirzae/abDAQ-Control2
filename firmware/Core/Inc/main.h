/**
 ******************************************************************************
 * @file    main.h
 * @brief   Header for main.c file for LabDAQ-Control STM32F407 application
 ******************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "labdaq_config.h"
#include "labdaq_sampler.h"
#include "labdaq_comm.h"
#include "labdaq_net.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void SystemClock_Config(void);

extern labdaq_system_t g_labdaq;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
