/*
 * MCU_Initialization.c
 *
 * Created: 08/10/2012 21:47:25
 *  Author: NASSER GHOSEIRI
 */ 

#include "AVR32X\AVR32_Module.h"
#include "PIC32\PIC32_Module.h"
#include "STM32\STM32_Module.h"
#include "std_defs.h"

/// ************************ MCU Initialization
void init_mcu(void)
{
	#if defined(__COMPILING_FOR_AVR32__)
		__AVR32_LowLevelInitialize();
	#elif defined(__COMPILING_FOR_PIC32__)
		__PIC32_LowLevelInitialize();
	#elif defined(__COMPILING_FOR_STM32__)
		__STM32_LowLevelInitialize();
	#endif
}