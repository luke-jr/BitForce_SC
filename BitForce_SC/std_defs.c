/*
 * std_defs.c
 *
 * Created: 21/11/2012 20:56:46
 *  Author: NASSER
 */ 

#include "std_defs.h"
#include "Generic_Module.h"
#include "AVR32_OptimizedTemplates.h"
#include <avr32/io.h>

// Set our initial value for the critical temperature
const unsigned int __ASIC_FREQUENCY_WORDS [10] = {0x0, 0xFFFF, 0xFFFD, 0xFFF5, 0xFFD5, 0xFF55, 0xFD55, 0xF555, 0xD555, 0x5555};
const unsigned int __ASIC_FREQUENCY_VALUES[10] = {189, 233, 240, 246, 253, 260, 266, 274, 283, 291}; // TO BE DETERMINED
	
#if (TOTAL_CHIPS_INSTALLED == 8)
	unsigned int  GLOBAL_CHIP_FREQUENCY_INFO[TOTAL_CHIPS_INSTALLED] =  {0,0,0,0,0,0,0,0};
	unsigned char GLOBAL_CHIP_PROCESSOR_COUNT[TOTAL_CHIPS_INSTALLED] = {0,0,0,0,0,0,0,0};
#elif (TOTAL_CHIPS_INSTALLED == 16)
	unsigned int  GLOBAL_CHIP_FREQUENCY_INFO[TOTAL_CHIPS_INSTALLED] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char GLOBAL_CHIP_PROCESSOR_COUNT[TOTAL_CHIPS_INSTALLED] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#else	
	unsigned int  GLOBAL_CHIP_FREQUENCY_INFO[TOTAL_CHIPS_INSTALLED];
	unsigned char GLOBAL_CHIP_PROCESSOR_COUNT[TOTAL_CHIPS_INSTALLED];
#endif	

volatile void Sleep(unsigned int iSleepPeriod)
{
	volatile unsigned int iActualCounter = MACRO_GetTickCountRet;
	while (MACRO_GetTickCountRet - iActualCounter < iSleepPeriod) WATCHDOG_RESET;
}

void System_Request_Pulse_Blink()
{
	if (GLOBAL_PULSE_BLINK_REQUEST == 0) 
	{
		GLOBAL_PULSE_BLINK_REQUEST = MACRO_GetTickCountRet;
	}		
}