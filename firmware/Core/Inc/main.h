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

#if __has_include("labdaq_config.h")
#include "labdaq_config.h"
#elif __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#endif

#if __has_include("labdaq_sampler.h")
#include "labdaq_sampler.h"
#elif __has_include("../Drivers/labdaq_sampler.h")
#include "../Drivers/labdaq_sampler.h"
#elif __has_include("../../Drivers/labdaq_sampler.h")
#include "../../Drivers/labdaq_sampler.h"
#endif

#if __has_include("labdaq_comm.h")
#include "labdaq_comm.h"
#elif __has_include("../Drivers/labdaq_comm.h")
#include "../Drivers/labdaq_comm.h"
#elif __has_include("../../Drivers/labdaq_comm.h")
#include "../../Drivers/labdaq_comm.h"
#endif

#if __has_include("labdaq_net.h")
#include "labdaq_net.h"
#elif __has_include("../Drivers/labdaq_net.h")
#include "../Drivers/labdaq_net.h"
#elif __has_include("../../Drivers/labdaq_net.h")
#include "../../Drivers/labdaq_net.h"
#endif

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void SystemClock_Config(void);

extern labdaq_system_t g_labdaq;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
