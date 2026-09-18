/**
 * @file    labdaq_config.h
 * @brief   Include forwarder for Drivers/ -> Config/labdaq_config.h
 *          Enables seamless compilation even if IDE include path (-I../Config) is omitted.
 */
#ifndef DRIVERS_LABDAQ_CONFIG_H_
#define DRIVERS_LABDAQ_CONFIG_H_

#if __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#elif __has_include("Config/labdaq_config.h")
#include "Config/labdaq_config.h"
#endif

#endif /* DRIVERS_LABDAQ_CONFIG_H_ */
