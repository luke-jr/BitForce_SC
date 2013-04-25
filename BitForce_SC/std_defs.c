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
extern volatile char GLOBAL_CRITICAL_TEMPERATURE = FALSE;

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