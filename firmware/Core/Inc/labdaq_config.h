/**
 * @file    labdaq_config.h
 * @brief   Include forwarder for Core/Inc/ -> Config/labdaq_config.h
 */
#ifndef CORE_INC_LABDAQ_CONFIG_H_
#define CORE_INC_LABDAQ_CONFIG_H_

#if __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#elif __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("Config/labdaq_config.h")
#include "Config/labdaq_config.h"
#endif

#endif /* CORE_INC_LABDAQ_CONFIG_H_ */
