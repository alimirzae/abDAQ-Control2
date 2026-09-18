/**
 * @file    main.h
 * @brief   Include forwarder for Core/Src/ -> Core/Inc/main.h
 *          Enables seamless compilation when arm-none-eabi-gcc is invoked on Core/Src/*.c
 *          without explicit -I../Core/Inc flag.
 */
#ifndef CORE_SRC_MAIN_FORWARDER_H_
#define CORE_SRC_MAIN_FORWARDER_H_

#if __has_include("../Inc/main.h")
#include "../Inc/main.h"
#elif __has_include("../../Core/Inc/main.h")
#include "../../Core/Inc/main.h"
#elif __has_include("Core/Inc/main.h")
#include "Core/Inc/main.h"
#endif

#endif /* CORE_SRC_MAIN_FORWARDER_H_ */
